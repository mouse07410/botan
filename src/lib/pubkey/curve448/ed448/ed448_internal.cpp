
/*
 * Ed448 Internals
 * (C) 2024 Jack Lloyd
 *     2024 Fabian Albert - Rohde & Schwarz Cybersecurity
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */
#include <botan/internal/ed448_internal.h>

#include <botan/exceptn.h>
#include <botan/types.h>
#include <botan/internal/ct_utils.h>
#include <botan/internal/loadstor.h>
#include <botan/internal/mp_core.h>
#include <botan/internal/shake_xof.h>
#include <botan/internal/stl_util.h>

namespace Botan {
namespace {

constexpr uint64_t MINUS_D = 39081;

std::vector<uint8_t> dom4(uint8_t x, std::span<const uint8_t> y) {
   // RFC 8032 2. Notation and Conventions
   // dom4(x, y) The octet string "SigEd448" || octet(x) ||
   //            octet(OLEN(y)) || y, where x is in range 0-255 and y
   //            is an octet string of at most 255 octets. "SigEd448"
   //            is in ASCII (8 octets).
   BOTAN_ARG_CHECK(y.size() < 256, "y is too long");
   return concat_as<std::vector<uint8_t>>(std::array<uint8_t, 8>{'S', 'i', 'g', 'E', 'd', '4', '4', '8'},
                                          store_le(x),
                                          store_le(static_cast<uint8_t>(y.size())),
                                          y);
}

template <ranges::spanable_range... Ts>
std::array<uint8_t, 2 * ED448_LEN> shake(bool f, std::span<const uint8_t> context, Ts... xs) {
   auto shake_xof = SHAKE_256_XOF();
   shake_xof.update(dom4(static_cast<uint8_t>(f), context));
   (shake_xof.update(std::span(xs)), ...);
   std::array<uint8_t, 2 * ED448_LEN> res;
   shake_xof.output(res);
   return res;
}

std::pair<std::span<const uint8_t, 57>, std::span<const uint8_t, 57>> split(std::span<const uint8_t, 114> arr) {
   BufferSlicer bs(arr);
   auto lhs = bs.take<57>();
   auto rhs = bs.take<57>();
   return {lhs, rhs};
}

Scalar448 scalar_from_xof(XOF& shake_xof) {
   // 5.2.5. Key Generation
   // The 57-byte public key is generated by the following steps:
   // 1. Hash the 57-byte private key using SHAKE256(x, 114), storing the
   //    digest in a 114-octet large buffer, denoted h. Only the lower 57
   //    bytes are used for generating the public key.
   std::array<uint8_t, ED448_LEN> raw_s;
   shake_xof.output(raw_s);
   // 2. Prune the buffer: The two least significant bits of the first
   //    octet are cleared, all eight bits the last octet are cleared, and
   //    the highest bit of the second to last octet is set.
   raw_s[0] &= 0xFC;
   raw_s[55] |= 0x80;
   raw_s[56] = 0;

   return Scalar448(raw_s);
}

}  // namespace

Ed448Point Ed448Point::decode(std::span<const uint8_t, ED448_LEN> enc) {
   // RFC 8032 5.2.3  Decoding
   // 1. First, interpret the string as an integer in little-endian
   //    representation.  Bit 455 of this number is the least significant
   //    bit of the x-coordinate, and denote this value x_0.  The
   //    y-coordinate is recovered simply by clearing this bit.  If the
   //    resulting value is >= p, decoding fails.
   if((enc.back() & 0x7F) != 0) {  // last byte is either 0x00 or 0x80
      throw Decoding_Error("Ed448 point has unacceptable x-distinguisher");
   }
   const bool x_distinguisher = enc.back() != 0;
   const auto y_data = std::span(enc).first<56>();
   if(!Gf448Elem::bytes_are_canonical_representation(y_data)) {
      throw Decoding_Error("Ed448 y-coordinate is not smaller than p");
   }
   const auto y = Gf448Elem(y_data);

   // 2. To recover the x-coordinate, the curve equation implies
   //    x^2 = (y^2 - 1) / (d y^2 - 1) (mod p).  The denominator is always
   //    non-zero mod p.  Let u = y^2 - 1 and v = d y^2 - 1.  To compute
   //    the square root of (u/v), the first step is to compute the
   //    candidate root x = (u/v)^((p+1)/4).  This can be done using the
   //    following trick, to use a single modular powering for both the
   //    inversion of v and the square root:
   //                      (p+1)/4    3            (p-3)/4
   //             x = (u/v)        = u  v (u^5 v^3)         (mod p)
   const auto d = -Gf448Elem(MINUS_D);
   const auto u = square(Gf448Elem(y)) - 1;
   const auto v = d * square(Gf448Elem(y)) - 1;
   const auto maybe_x = (u * square(u)) * v * root((square(square(u)) * u) * square(v) * v);

   // 3. If v * x^2 = u, the recovered x-coordinate is x.  Otherwise, no
   //    square root exists, and the decoding fails.
   if(v * square(maybe_x) != u) {
      throw Decoding_Error("Square root does not exist");
   }
   // 4. Finally, use the x_0 bit to select the right square root.  If
   //    x = 0, and x_0 = 1, decoding fails.  Otherwise, if x_0 != x mod
   //    2, set x <-- p - x.  Return the decoded point (x,y).
   if(maybe_x.is_zero() && x_distinguisher) {
      throw Decoding_Error("Square root of zero cannot be odd");
   }
   bool maybe_x_parity = maybe_x.is_odd();
   std::array<uint64_t, WORDS_448> x_data;
   CT::Mask<uint64_t>::expand(maybe_x_parity == x_distinguisher)
      .select_n(x_data.data(), maybe_x.words().data(), (-maybe_x).words().data(), 7);

   return {Gf448Elem(x_data), y};
}

Ed448Point Ed448Point::base_point() {
   constexpr std::array<const uint64_t, WORDS_448> x = {0x2626a82bc70cc05e,
                                                        0x433b80e18b00938e,
                                                        0x12ae1af72ab66511,
                                                        0xea6de324a3d3a464,
                                                        0x9e146570470f1767,
                                                        0x221d15a622bf36da,
                                                        0x4f1970c66bed0ded};
   constexpr std::array<const uint64_t, WORDS_448> y = {0x9808795bf230fa14,
                                                        0xfdbd132c4ed7c8ad,
                                                        0x3ad3ff1ce67c39c4,
                                                        0x87789c1e05a0c2d7,
                                                        0x4bea73736ca39840,
                                                        0x8876203756c9c762,
                                                        0x693f46716eb6bc24};
   return Ed448Point(Gf448Elem(x), Gf448Elem(y));
}

std::array<uint8_t, ED448_LEN> Ed448Point::encode() const {
   std::array<uint8_t, ED448_LEN> res_buf = {0};

   // RFC 8032 5.2.2
   //    All values are coded as octet strings, and integers are coded using
   //    little-endian convention. [...]
   //    First, encode the y-coordinate as a little-endian string of 57 octets.
   //    The final octet is always zero.
   y().to_bytes(std::span(res_buf).first<56>());

   //    To form the encoding of the point, copy the least significant bit of
   //    the x-coordinate to the most significant bit of the final octet.
   res_buf.back() = (static_cast<uint8_t>(x().is_odd()) << 7);

   return res_buf;
}

Ed448Point Ed448Point::operator+(const Ed448Point& other) const {
   // RFC 8032 5.2.4. - Point Addition (Add)
   const Gf448Elem A = m_z * other.m_z;
   const Gf448Elem B = square(A);
   const Gf448Elem C = m_x * other.m_x;
   const Gf448Elem D = m_y * other.m_y;
   const Gf448Elem E = (-Gf448Elem(MINUS_D)) * C * D;
   const Gf448Elem F = B - E;
   const Gf448Elem G = B + E;
   const Gf448Elem H = (m_x + m_y) * (other.m_x + other.m_y);
   const Gf448Elem X3 = A * F * (H - C - D);
   const Gf448Elem Y3 = A * G * (D - C);
   const Gf448Elem Z3 = F * G;

   return Ed448Point(X3, Y3, Z3);
}

Ed448Point Ed448Point::double_point() const {
   // RFC 8032 5.2.4. - Point Addition (Double)
   const Gf448Elem B = square(m_x + m_y);
   const Gf448Elem C = square(m_x);
   const Gf448Elem D = square(m_y);
   const Gf448Elem E = C + D;
   const Gf448Elem H = square(m_z);
   const Gf448Elem J = E - (H + H);
   const Gf448Elem X3 = (B - E) * J;
   const Gf448Elem Y3 = E * (C - D);
   const Gf448Elem Z3 = E * J;

   return Ed448Point(X3, Y3, Z3);
}

Ed448Point Ed448Point::scalar_mul(const Scalar448& s) const {
   Ed448Point res(0, 1);

   // Square and multiply (double and add) in constant time.
   // TODO: Optimization potential. E.g. for a = *this precompute
   //       0, a, 2a, 3a, ..., 15a and ct select and add the right one for
   //       each 4 bit window instead of conditional add.
   for(int16_t i = 445; i >= 0; --i) {
      res = res.double_point();
      // Conditional add if bit is set
      auto add_sum = res + *this;
      res.ct_conditional_assign(s.get_bit(i), add_sum);
   }
   return res;
}

bool Ed448Point::operator==(const Ed448Point& other) const {
   // Note that the operator== of of Gf448Elem is constant time
   const auto mask_x = CT::Mask<uint8_t>::expand(x() == other.x());
   const auto mask_y = CT::Mask<uint8_t>::expand(y() == other.y());

   return (mask_x & mask_y).as_bool();
}

void Ed448Point::ct_conditional_assign(bool cond, const Ed448Point& other) {
   m_x.ct_cond_assign(cond, other.m_x);
   m_y.ct_cond_assign(cond, other.m_y);
   m_z.ct_cond_assign(cond, other.m_z);
}

Ed448Point operator*(const Scalar448& lhs, const Ed448Point& rhs) {
   return rhs.scalar_mul(lhs);
}

std::array<uint8_t, ED448_LEN> create_pk_from_sk(std::span<const uint8_t, ED448_LEN> sk) {
   // 5.2.5. Key Generation
   // The 57-byte public key is generated by the following steps:
   auto shake_xof = SHAKE_256_XOF();
   shake_xof.update(sk);

   const Scalar448 s = scalar_from_xof(shake_xof);
   // 3. Interpret the buffer as the little-endian integer, forming a
   //    secret scalar s. Perform a known-base-point scalar
   //    multiplication [s]B.
   return (s * Ed448Point::base_point()).encode();
}

std::array<uint8_t, 2 * ED448_LEN> sign_message(std::span<const uint8_t, ED448_LEN> sk,
                                                std::span<const uint8_t, ED448_LEN> pk,
                                                bool pgflag,
                                                std::span<const uint8_t> context,
                                                std::span<const uint8_t> msg) {
   // 5.2.6. Signature Generation
   // The inputs to the signing procedure is the private key, a 57-octet
   // string, a flag F, which is 0 for Ed448, 1 for Ed448ph, context C of
   // at most 255 octets, and a message M of arbitrary size.
   // 1. Hash the private key, 57 octets, using SHAKE256(x, 114). Let h
   //    denote the resulting digest. Construct the secret scalar s from
   //    the first half of the digest, and the corresponding public key A,
   //    as described in the previous section. Let prefix denote the
   //    second half of the hash digest, h[57],...,h[113].
   auto shake_xof = SHAKE_256_XOF();
   shake_xof.update(sk);
   const Scalar448 s = scalar_from_xof(shake_xof);
   std::array<uint8_t, ED448_LEN> prefix;
   shake_xof.output(prefix);
   // 2. Compute SHAKE256(dom4(F, C) || prefix || PH(M), 114), where M is
   //    the message to be signed, F is 1 for Ed448ph, 0 for Ed448, and C
   //    is the context to use. Interpret the 114-octet digest as a
   //    little-endian integer r.
   const Scalar448 r(shake(pgflag, context, prefix, msg));
   // 3. Compute the point [r]B. For efficiency, do this by first
   //    reducing r modulo L, the group order of B. Let the string R be
   //    the encoding of this point.
   const auto big_r = (r * Ed448Point::base_point()).encode();
   // 4. Compute SHAKE256(dom4(F, C) || R || A || PH(M), 114), and
   //    interpret the 114-octet digest as a little-endian integer k.
   const Scalar448 k(shake(pgflag, context, big_r, pk, msg));
   // 5. Compute S = (r + k * s) mod L. For efficiency, again reduce k
   //    modulo L first.
   const auto big_s = r + k * s;  //r_plus_ks_mod_L(r, k, s);
   // 6. Form the signature of the concatenation of R (57 octets) and the
   //    little-endian encoding of S (57 octets; the ten most significant
   //    bits of the final octets are always zero).
   std::array<uint8_t, 2 * ED448_LEN> sig;
   BufferStuffer stuf(sig);
   stuf.append(big_r);
   stuf.append(big_s.to_bytes<ED448_LEN>());
   BOTAN_ASSERT(stuf.full(), "Buffer is full");

   return sig;
}

bool verify_signature(std::span<const uint8_t, ED448_LEN> pk,
                      bool phflag,
                      std::span<const uint8_t> context,
                      std::span<const uint8_t> sig,
                      std::span<const uint8_t> msg) {
   // RFC 8032 5.2.7. Verify
   // 1. To verify a signature on a message M using context C and public
   //    key A, with F being 0 for Ed448 and 1 for Ed448ph, first split
   //    the signature into two 57-octet halves. Decode the first half as
   //    a point R, and the second half as an integer S, in the range 0 <=
   //    s < L. Decode the public key A as point A’. If any of the
   //    decodings fail (including S being out of range), the signature is
   //    invalid.
   if(sig.size() != 2 * ED448_LEN) {
      // Wrong signature size
      throw Decoding_Error("Ed448 signature has wrong size");
   }
   const auto [big_r_bytes, big_s_bytes] = split(sig.first<2 * ED448_LEN>());
   const auto big_r = Ed448Point::decode(big_r_bytes);
   if(!Scalar448::bytes_are_reduced(big_s_bytes)) {
      // S not in range 0 <= s < L
      throw Decoding_Error("Ed448 signature has invalid S");
   }
   const Scalar448 big_s(big_s_bytes);
   // 2. Compute SHAKE256(dom4(F, C) || R || A || PH(M), 114), and
   //    interpret the 114-octet digest as a little-endian integer k.
   const Scalar448 k(shake(phflag, context, big_r_bytes, pk, msg));
   // 3. Check the group equation [4][S]B = [4]R + [4][k]A’. It’s
   //    sufficient, but not required, to instead check [S]B = R + [k]A’.
   return (big_s * Ed448Point::base_point()) == (big_r + k * Ed448Point::decode(pk));
}

}  // namespace Botan
