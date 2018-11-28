/*
* BigInt Base
* (C) 1999-2011,2012,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/bigint.h>
#include <botan/internal/mp_core.h>
#include <botan/internal/rounding.h>
#include <botan/internal/bit_ops.h>
#include <botan/internal/ct_utils.h>

namespace Botan {

BigInt::BigInt(const word words[], size_t length)
   {
   m_data.set_words(words, length);
   }

/*
* Construct a BigInt from a regular number
*/
BigInt::BigInt(uint64_t n)
   {
   if(n > 0)
      {
#if BOTAN_MP_WORD_BITS == 32
      m_data.set_word_at(0, static_cast<word>(n));
      m_data.set_word_at(1, static_cast<word>(n >> 32));
#else
      m_data.set_word_at(0, n);
#endif
      }

   }

/*
* Construct a BigInt of the specified size
*/
BigInt::BigInt(Sign s, size_t size)
   {
   m_data.set_size(size);
   m_signedness = s;
   }

/*
* Construct a BigInt from a string
*/
BigInt::BigInt(const std::string& str)
   {
   Base base = Decimal;
   size_t markers = 0;
   bool negative = false;

   if(str.length() > 0 && str[0] == '-')
      {
      markers += 1;
      negative = true;
      }

   if(str.length() > markers + 2 && str[markers    ] == '0' &&
                                    str[markers + 1] == 'x')
      {
      markers += 2;
      base = Hexadecimal;
      }

   *this = decode(cast_char_ptr_to_uint8(str.data()) + markers,
                  str.length() - markers, base);

   if(negative) set_sign(Negative);
   else         set_sign(Positive);
   }

BigInt::BigInt(const uint8_t input[], size_t length)
   {
   binary_decode(input, length);
   }

/*
* Construct a BigInt from an encoded BigInt
*/
BigInt::BigInt(const uint8_t input[], size_t length, Base base)
   {
   *this = decode(input, length, base);
   }

BigInt::BigInt(const uint8_t buf[], size_t length, size_t max_bits)
   {
   const size_t max_bytes = std::min(length, (max_bits + 7) / 8);
   binary_decode(buf, max_bytes);

   const size_t b = this->bits();
   if(b > max_bits)
      {
      *this >>= (b - max_bits);
      }
   }

/*
* Construct a BigInt from an encoded BigInt
*/
BigInt::BigInt(RandomNumberGenerator& rng, size_t bits, bool set_high_bit)
   {
   randomize(rng, bits, set_high_bit);
   }

int32_t BigInt::cmp_word(word other) const
   {
   if(is_negative())
      return -1; // other is positive ...

   const size_t sw = this->sig_words();
   if(sw > 1)
      return 1; // must be larger since other is just one word ...

   return bigint_cmp(this->data(), sw, &other, 1);
   }

/*
* Comparison Function
*/
int32_t BigInt::cmp(const BigInt& other, bool check_signs) const
   {
   if(check_signs)
      {
      if(other.is_positive() && this->is_negative())
         return -1;

      if(other.is_negative() && this->is_positive())
         return 1;

      if(other.is_negative() && this->is_negative())
         return (-bigint_cmp(this->data(), this->sig_words(),
                             other.data(), other.sig_words()));
      }

   return bigint_cmp(this->data(), this->sig_words(),
                     other.data(), other.sig_words());
   }

bool BigInt::is_equal(const BigInt& other) const
   {
   if(this->sign() != other.sign())
      return false;

   return bigint_ct_is_eq(this->data(), this->sig_words(),
                          other.data(), other.sig_words());
   }

bool BigInt::is_less_than(const BigInt& other) const
   {
   if(this->is_negative() && other.is_positive())
      return true;

   if(this->is_positive() && other.is_negative())
      return false;

   if(other.is_negative() && this->is_negative())
      {
      return !bigint_ct_is_lt(other.data(), other.sig_words(),
                              this->data(), this->sig_words(), true);
      }

   return bigint_ct_is_lt(this->data(), this->sig_words(),
                          other.data(), other.sig_words());
   }

void BigInt::encode_words(word out[], size_t size) const
   {
   const size_t words = sig_words();

   if(words > size)
      throw Encoding_Error("BigInt::encode_words value too large to encode");

   clear_mem(out, size);
   copy_mem(out, data(), words);
   }

size_t BigInt::Data::calc_sig_words() const
   {
   size_t sig = m_reg.size();

   word sub = 1;

   for(size_t i = 0; i != m_reg.size(); ++i)
      {
      const word w = m_reg[m_reg.size() - i - 1];
      sub &= CT::is_zero(w);
      sig -= sub;
      }

   /*
   * This depends on the data so is poisoned, but unpoison it here as
   * later conditionals are made on the size.
   */
   CT::unpoison(sig);

   return sig;
   }

/*
* Return bits {offset...offset+length}
*/
uint32_t BigInt::get_substring(size_t offset, size_t length) const
   {
   if(length == 0 || length > 32)
      throw Invalid_Argument("BigInt::get_substring invalid substring length");

   const size_t byte_offset = offset / 8;
   const size_t shift = (offset % 8);
   const uint32_t mask = 0xFFFFFFFF >> (32 - length);

   const uint8_t b0 = byte_at(byte_offset);
   const uint8_t b1 = byte_at(byte_offset + 1);
   const uint8_t b2 = byte_at(byte_offset + 2);
   const uint8_t b3 = byte_at(byte_offset + 3);
   const uint8_t b4 = byte_at(byte_offset + 4);
   const uint64_t piece = make_uint64(0, 0, 0, b4, b3, b2, b1, b0);

   return static_cast<uint32_t>((piece >> shift) & mask);
   }

/*
* Convert this number to a uint32_t, if possible
*/
uint32_t BigInt::to_u32bit() const
   {
   if(is_negative())
      throw Encoding_Error("BigInt::to_u32bit: Number is negative");
   if(bits() > 32)
      throw Encoding_Error("BigInt::to_u32bit: Number is too big to convert");

   uint32_t out = 0;
   for(size_t i = 0; i != 4; ++i)
      out = (out << 8) | byte_at(3-i);
   return out;
   }

/*
* Set bit number n
*/
void BigInt::set_bit(size_t n)
   {
   const size_t which = n / BOTAN_MP_WORD_BITS;

   if(which >= size())
      {
      grow_to(which + 1);
      }

   const word mask = static_cast<word>(1) << (n % BOTAN_MP_WORD_BITS);
   m_data.set_word_at(which, word_at(which) | mask);
   }

/*
* Clear bit number n
*/
void BigInt::clear_bit(size_t n)
   {
   const size_t which = n / BOTAN_MP_WORD_BITS;

   if(which < size())
      {
      const word mask = ~(static_cast<word>(1) << (n % BOTAN_MP_WORD_BITS));
      m_data.set_word_at(which, word_at(which) & mask);
      }
   }

size_t BigInt::bytes() const
   {
   return round_up(bits(), 8) / 8;
   }

/*
* Count how many bits are being used
*/
size_t BigInt::bits() const
   {
   const size_t words = sig_words();

   if(words == 0)
      return 0;

   const size_t full_words = words - 1;
   const size_t bits = (full_words * BOTAN_MP_WORD_BITS + high_bit(word_at(full_words)));
   // Need to unpoison due to high_bit not being const time
   CT::unpoison(bits);
   return bits;
   }

/*
* Calcluate the size in a certain base
*/
size_t BigInt::encoded_size(Base base) const
   {
   static const double LOG_2_BASE_10 = 0.30102999566;

   if(base == Binary)
      return bytes();
   else if(base == Hexadecimal)
      return 2*bytes();
   else if(base == Decimal)
      return static_cast<size_t>((bits() * LOG_2_BASE_10) + 1);
   else
      throw Invalid_Argument("Unknown base for BigInt encoding");
   }

/*
* Return the negation of this number
*/
BigInt BigInt::operator-() const
   {
   BigInt x = (*this);
   x.flip_sign();
   return x;
   }

void BigInt::reduce_below(const BigInt& p, secure_vector<word>& ws)
   {
   if(p.is_negative())
      throw Invalid_Argument("BigInt::reduce_below mod must be positive");

   const size_t p_words = p.sig_words();

   if(size() < p_words + 1)
      grow_to(p_words + 1);

   if(ws.size() < p_words + 1)
      ws.resize(p_words + 1);

   clear_mem(ws.data(), ws.size());

   for(;;)
      {
      word borrow = bigint_sub3(ws.data(), data(), p_words + 1, p.data(), p_words);

      //CT::unpoison(borrow); // fixme
      if(borrow)
         break;

      swap_reg(ws);
      }
   }

/*
* Return the absolute value of this number
*/
BigInt BigInt::abs() const
   {
   BigInt x = (*this);
   x.set_sign(Positive);
   return x;
   }

/*
* Encode this number into bytes
*/
void BigInt::binary_encode(uint8_t output[]) const
   {
   const size_t sig_bytes = bytes();
   for(size_t i = 0; i != sig_bytes; ++i)
      output[sig_bytes-i-1] = byte_at(i);
   }

/*
* Set this number to the value in buf
*/
void BigInt::binary_decode(const uint8_t buf[], size_t length)
   {
   const size_t WORD_BYTES = sizeof(word);

   clear();
   secure_vector<word> reg((round_up((length / WORD_BYTES) + 1, 8)));

   // TODO can load a word at a time here
   for(size_t i = 0; i != length / WORD_BYTES; ++i)
      {
      const size_t top = length - WORD_BYTES*i;
      for(size_t j = WORD_BYTES; j > 0; --j)
         reg[i] = (reg[i] << 8) | buf[top - j];
      }

   for(size_t i = 0; i != length % WORD_BYTES; ++i)
      reg[length / WORD_BYTES] = (reg[length / WORD_BYTES] << 8) | buf[i];

   m_data.swap(reg);
   }

void BigInt::ct_cond_assign(bool predicate, BigInt& other)
   {
   const size_t t_words = size();
   const size_t o_words = other.size();

   const size_t r_words = std::max(t_words, o_words);

   const word mask = CT::expand_mask<word>(predicate);

   for(size_t i = 0; i != r_words; ++i)
      {
      this->set_word_at(i, CT::select<word>(mask, other.word_at(i), this->word_at(i)));
      }
   }

#if defined(BOTAN_HAS_VALGRIND)
void BigInt::const_time_poison() const
   {
   CT::poison(m_data.const_data(), m_data.size());
   }

void BigInt::const_time_unpoison() const
   {
   CT::unpoison(m_data.const_data(), m_data.size());
   }
#endif

void BigInt::const_time_lookup(secure_vector<word>& output,
                               const std::vector<BigInt>& vec,
                               size_t idx)
   {
   const size_t words = output.size();

   clear_mem(output.data(), output.size());

   CT::poison(&idx, sizeof(idx));

   for(size_t i = 0; i != vec.size(); ++i)
      {
      BOTAN_ASSERT(vec[i].size() >= words,
                   "Word size as expected in const_time_lookup");

      const word mask = CT::is_equal(i, idx);

      for(size_t w = 0; w != words; ++w)
         output[w] |= CT::select<word>(mask, vec[i].word_at(w), 0);
      }

   CT::unpoison(idx);
   CT::unpoison(output.data(), output.size());
   }

}
