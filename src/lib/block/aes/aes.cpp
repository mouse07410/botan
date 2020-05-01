/*
* AES
* (C) 1999-2010,2015,2017,2018,2020 Jack Lloyd
*
* Based on the public domain reference implementation by Paulo Baretto
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/aes.h>
#include <botan/loadstor.h>
#include <botan/cpuid.h>
#include <botan/rotate.h>
#include <botan/internal/bit_ops.h>
#include <botan/internal/ct_utils.h>
#include <type_traits>

/*
* This implementation is based on table lookups which are known to be
* vulnerable to timing and cache based side channel attacks. Some
* countermeasures are used which may be helpful in some situations:
*
* - Only a single 256-word T-table is used, with rotations applied.
*   Most implementations use 4 (or sometimes 5) T-tables, which leaks
*   much more information via cache usage.
*
* - The TE and TD tables are computed at runtime to avoid flush+reload
*   attacks using clflush. As different processes will not share the
*   same underlying table data, an attacker can't manipulate another
*   processes cache lines via their shared reference to the library
*   read only segment. (However, prime+probe attacks are still possible.)
*
* - Each cache line of the lookup tables is accessed at the beginning
*   of each call to encrypt or decrypt. (See the Z variable below)
*
* If available SSSE3 or AES-NI are used instead of this version, as both
* are faster and immune to side channel attacks.
*
* Some AES cache timing papers for reference:
*
* "Software mitigations to hedge AES against cache-based software side
* channel vulnerabilities" https://eprint.iacr.org/2006/052.pdf
*
* "Cache Games - Bringing Access-Based Cache Attacks on AES to Practice"
* http://www.ieee-security.org/TC/SP2011/PAPERS/2011/paper031.pdf
*
* "Cache-Collision Timing Attacks Against AES" Bonneau, Mironov
* http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.88.4753
*/

namespace Botan {

namespace {

alignas(64)
const uint8_t SE[256] = {
   0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B,
   0xFE, 0xD7, 0xAB, 0x76, 0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0,
   0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0, 0xB7, 0xFD, 0x93, 0x26,
   0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
   0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
   0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0,
   0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED,
   0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
   0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F,
   0x50, 0x3C, 0x9F, 0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5,
   0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2, 0xCD, 0x0C, 0x13, 0xEC,
   0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
   0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14,
   0xDE, 0x5E, 0x0B, 0xDB, 0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C,
   0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8, 0x37, 0x6D,
   0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
   0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F,
   0x4B, 0xBD, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E,
   0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E, 0xE1, 0xF8, 0x98, 0x11,
   0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
   0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F,
   0xB0, 0x54, 0xBB, 0x16 };

alignas(64)
const uint8_t SD[256] = {
   0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E,
   0x81, 0xF3, 0xD7, 0xFB, 0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87,
   0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB, 0x54, 0x7B, 0x94, 0x32,
   0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
   0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49,
   0x6D, 0x8B, 0xD1, 0x25, 0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,
   0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92, 0x6C, 0x70, 0x48, 0x50,
   0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
   0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05,
   0xB8, 0xB3, 0x45, 0x06, 0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02,
   0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B, 0x3A, 0x91, 0x11, 0x41,
   0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
   0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8,
   0x1C, 0x75, 0xDF, 0x6E, 0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89,
   0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B, 0xFC, 0x56, 0x3E, 0x4B,
   0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
   0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59,
   0x27, 0x80, 0xEC, 0x5F, 0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D,
   0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF, 0xA0, 0xE0, 0x3B, 0x4D,
   0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
   0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63,
   0x55, 0x21, 0x0C, 0x7D };

inline constexpr uint8_t xtime(uint8_t s) { return static_cast<uint8_t>(s << 1) ^ ((s >> 7) * 0x1B); }
inline constexpr uint8_t xtime3(uint8_t s) { return xtime(s) ^ s; }

inline uint32_t InvMixColumn(uint8_t s1)
   {
   const uint8_t s2 = xtime(s1);
   const uint8_t s4 = xtime(s2);
   const uint8_t s8 = xtime(s4);
   const uint8_t s9 = s8 ^ s1;
   const uint8_t s11 = s9 ^ s2;
   const uint8_t s13 = s9 ^ s4;
   const uint8_t s14 = s8 ^ s4 ^ s2;
   return make_uint32(s14, s9, s13, s11);
   }

/*
This is a bitsliced AES sbox computation which can execute up
to 32 parallel sbox computations.

The circuit is from "A depth-16 circuit for the AES S-box" by Boyar
and Peralta (https://eprint.iacr.org/2011/332.pdf)
*/
void AES_SBOX(uint32_t V[8])
   {
   const uint32_t I0 = V[0];
   const uint32_t I1 = V[1];
   const uint32_t I2 = V[2];
   const uint32_t I3 = V[3];
   const uint32_t I4 = V[4];
   const uint32_t I5 = V[5];
   const uint32_t I6 = V[6];
   const uint32_t I7 = V[7];

   // Figure 5:  Top linear transform in forward direction.
   const uint32_t T1 = I0 ^ I3;
   const uint32_t T2 = I0 ^ I5;
   const uint32_t T3 = I0 ^ I6;
   const uint32_t T4 = I3 ^ I5;
   const uint32_t T5 = I4 ^ I6;
   const uint32_t T6 = T1 ^ T5;
   const uint32_t T7 = I1 ^ I2;

   const uint32_t T8 = I7 ^ T6;
   const uint32_t T9 = I7 ^ T7;
   const uint32_t T10 = T6 ^ T7;
   const uint32_t T11 = I1 ^ I5;
   const uint32_t T12 = I2 ^ I5;
   const uint32_t T13 = T3 ^ T4;
   const uint32_t T14 = T6 ^ T11;

   const uint32_t T15 = T5 ^ T11;
   const uint32_t T16 = T5 ^ T12;
   const uint32_t T17 = T9 ^ T16;
   const uint32_t T18 = I3 ^ I7;
   const uint32_t T19 = T7 ^ T18;
   const uint32_t T20 = T1 ^ T19;
   const uint32_t T21 = I6 ^ I7;

   const uint32_t T22 = T7 ^ T21;
   const uint32_t T23 = T2 ^ T22;
   const uint32_t T24 = T2 ^ T10;
   const uint32_t T25 = T20 ^ T17;
   const uint32_t T26 = T3 ^ T16;
   const uint32_t T27 = T1 ^ T12;

   const uint32_t D = I7;

   // Figure 7:  Shared part of AES S-box circuit
   const uint32_t M1 = T13 & T6;
   const uint32_t M2 = T23 & T8;
   const uint32_t M3 = T14 ^ M1;
   const uint32_t M4 = T19 & D;
   const uint32_t M5 = M4 ^ M1;
   const uint32_t M6 = T3 & T16;
   const uint32_t M7 = T22 & T9;
   const uint32_t M8 = T26 ^ M6;
   const uint32_t M9 = T20 & T17;
   const uint32_t M10 = M9 ^ M6;
   const uint32_t M11 = T1 & T15;
   const uint32_t M12 = T4 & T27;
   const uint32_t M13 = M12 ^ M11;
   const uint32_t M14 = T2 & T10;
   const uint32_t M15 = M14 ^ M11;
   const uint32_t M16 = M3 ^ M2;

   const uint32_t M17 = M5 ^ T24;
   const uint32_t M18 = M8 ^ M7;
   const uint32_t M19 = M10 ^ M15;
   const uint32_t M20 = M16 ^ M13;
   const uint32_t M21 = M17 ^ M15;
   const uint32_t M22 = M18 ^ M13;
   const uint32_t M23 = M19 ^ T25;
   const uint32_t M24 = M22 ^ M23;
   const uint32_t M25 = M22 & M20;
   const uint32_t M26 = M21 ^ M25;
   const uint32_t M27 = M20 ^ M21;
   const uint32_t M28 = M23 ^ M25;
   const uint32_t M29 = M28 & M27;
   const uint32_t M30 = M26 & M24;
   const uint32_t M31 = M20 & M23;
   const uint32_t M32 = M27 & M31;

   const uint32_t M33 = M27 ^ M25;
   const uint32_t M34 = M21 & M22;
   const uint32_t M35 = M24 & M34;
   const uint32_t M36 = M24 ^ M25;
   const uint32_t M37 = M21 ^ M29;
   const uint32_t M38 = M32 ^ M33;
   const uint32_t M39 = M23 ^ M30;
   const uint32_t M40 = M35 ^ M36;
   const uint32_t M41 = M38 ^ M40;
   const uint32_t M42 = M37 ^ M39;
   const uint32_t M43 = M37 ^ M38;
   const uint32_t M44 = M39 ^ M40;
   const uint32_t M45 = M42 ^ M41;
   const uint32_t M46 = M44 & T6;
   const uint32_t M47 = M40 & T8;
   const uint32_t M48 = M39 & D;

   const uint32_t M49 = M43 & T16;
   const uint32_t M50 = M38 & T9;
   const uint32_t M51 = M37 & T17;
   const uint32_t M52 = M42 & T15;
   const uint32_t M53 = M45 & T27;
   const uint32_t M54 = M41 & T10;
   const uint32_t M55 = M44 & T13;
   const uint32_t M56 = M40 & T23;
   const uint32_t M57 = M39 & T19;
   const uint32_t M58 = M43 & T3;
   const uint32_t M59 = M38 & T22;
   const uint32_t M60 = M37 & T20;
   const uint32_t M61 = M42 & T1;
   const uint32_t M62 = M45 & T4;
   const uint32_t M63 = M41 & T2;

   // Figure 8:  Bottom linear transform in forward direction.
   const uint32_t L0 = M61 ^ M62;
   const uint32_t L1 = M50 ^ M56;
   const uint32_t L2 = M46 ^ M48;
   const uint32_t L3 = M47 ^ M55;
   const uint32_t L4 = M54 ^ M58;
   const uint32_t L5 = M49 ^ M61;
   const uint32_t L6 = M62 ^ L5;
   const uint32_t L7 = M46 ^ L3;
   const uint32_t L8 = M51 ^ M59;
   const uint32_t L9 = M52 ^ M53;
   const uint32_t L10 = M53 ^ L4;
   const uint32_t L11 = M60 ^ L2;
   const uint32_t L12 = M48 ^ M51;
   const uint32_t L13 = M50 ^ L0;
   const uint32_t L14 = M52 ^ M61;
   const uint32_t L15 = M55 ^ L1;
   const uint32_t L16 = M56 ^ L0;
   const uint32_t L17 = M57 ^ L1;
   const uint32_t L18 = M58 ^ L8;
   const uint32_t L19 = M63 ^ L4;

   const uint32_t L20 = L0 ^ L1;
   const uint32_t L21 = L1 ^ L7;
   const uint32_t L22 = L3 ^ L12;
   const uint32_t L23 = L18 ^ L2;
   const uint32_t L24 = L15 ^ L9;
   const uint32_t L25 = L6 ^ L10;
   const uint32_t L26 = L7 ^ L9;
   const uint32_t L27 = L8 ^ L10;
   const uint32_t L28 = L11 ^ L14;
   const uint32_t L29 = L11 ^ L17;

   const uint32_t S0 = L6 ^ L24;
   const uint32_t S1 = ~(L16 ^ L26);
   const uint32_t S2 = ~(L19 ^ L28);
   const uint32_t S3 = L6 ^ L21;
   const uint32_t S4 = L20 ^ L22;
   const uint32_t S5 = L25 ^ L29;
   const uint32_t S6 = ~(L13 ^ L27);
   const uint32_t S7 = ~(L6 ^ L23);

   V[0] = S0;
   V[1] = S1;
   V[2] = S2;
   V[3] = S3;
   V[4] = S4;
   V[5] = S5;
   V[6] = S6;
   V[7] = S7;
   }

inline uint32_t SE_word(uint32_t x)
   {
   uint32_t I[8] = { 0 };

   // 0 8 16 24 1 9 17 25 2 10 18 26 3 11 19 27 4 12 20 28 5 13 21 29 6 14 22 30 7 15 23 31
   x = bit_permute_step<uint32_t>(x, 0x00aa00aa, 7);  // Bit index swap 0,3
   x = bit_permute_step<uint32_t>(x, 0x0000cccc, 14); // Bit index swap 1,4
   x = bit_permute_step<uint32_t>(x, 0x00f000f0, 4);  // Bit index swap 2,3
   x = bit_permute_step<uint32_t>(x, 0x0000ff00, 8);  // Bit index swap 3,4

   for(size_t i = 0; i != 8; ++i)
      I[i] = (x >> (28-4*i)) & 0xF;

   AES_SBOX(I);

   x = 0;

   for(size_t i = 0; i != 8; ++i)
      x = (x << 4) + (I[i] & 0xF);

   // 0 4 8 12 16 20 24 28 1 5 9 13 17 21 25 29 2 6 10 14 18 22 26 30 3 7 11 15 19 23 27 31
   x = bit_permute_step<uint32_t>(x, 0x0a0a0a0a, 3);  // Bit index swap 0,2
   x = bit_permute_step<uint32_t>(x, 0x00cc00cc, 6);  // Bit index swap 1,3
   x = bit_permute_step<uint32_t>(x, 0x0000f0f0, 12);  // Bit index swap 2,4
   x = bit_permute_step<uint32_t>(x, 0x0000ff00, 8);  // Bit index swap 3,4

   return x;
   }

const uint32_t* AES_TE()
   {
   class TE_Table final
      {
      public:
         TE_Table()
            {
            uint32_t* p = reinterpret_cast<uint32_t*>(&data);
            for(size_t i = 0; i != 256; ++i)
               {
               const uint8_t s = SE[i];
               p[i] = make_uint32(xtime(s), s, s, xtime3(s));
               }
            }

         const uint32_t* ptr() const
            {
            return reinterpret_cast<const uint32_t*>(&data);
            }
      private:
         std::aligned_storage<256*sizeof(uint32_t), 64>::type data;
      };

   static TE_Table table;
   return table.ptr();
   }

const uint32_t* AES_TD()
   {
   class TD_Table final
      {
      public:
         TD_Table()
            {
            uint32_t* p = reinterpret_cast<uint32_t*>(&data);
            for(size_t i = 0; i != 256; ++i)
               {
               p[i] = InvMixColumn(SD[i]);
               }
            }

         const uint32_t* ptr() const
            {
            return reinterpret_cast<const uint32_t*>(&data);
            }
      private:
         std::aligned_storage<256*sizeof(uint32_t), 64>::type data;
      };

   static TD_Table table;
   return table.ptr();
   }

#define AES_T(T, K, V0, V1, V2, V3)                                     \
   (K ^ T[get_byte(0, V0)] ^                                            \
    rotr< 8>(T[get_byte(1, V1)]) ^                                      \
    rotr<16>(T[get_byte(2, V2)]) ^                                      \
    rotr<24>(T[get_byte(3, V3)]))

/*
* AES Encryption
*/
void aes_encrypt_n(const uint8_t in[], uint8_t out[],
                   size_t blocks,
                   const secure_vector<uint32_t>& EK,
                   const secure_vector<uint8_t>& ME)
   {
   BOTAN_ASSERT(EK.size() && ME.size() == 16, "Key was set");

   const size_t cache_line_size = CPUID::cache_line_size();
   const uint32_t* TE = AES_TE();

   // Hit every cache line of TE
   volatile uint32_t Z = 0;
   for(size_t i = 0; i < 256; i += cache_line_size / sizeof(uint32_t))
      {
      Z |= TE[i];
      }
   Z &= TE[82]; // this is zero, which hopefully the compiler cannot deduce

   for(size_t i = 0; i < blocks; ++i)
      {
      uint32_t T0, T1, T2, T3;
      load_be(in + 16*i, T0, T1, T2, T3);

      T0 ^= EK[0];
      T1 ^= EK[1];
      T2 ^= EK[2];
      T3 ^= EK[3];

      T0 ^= Z;

      uint32_t B0 = AES_T(TE, EK[4], T0, T1, T2, T3);
      uint32_t B1 = AES_T(TE, EK[5], T1, T2, T3, T0);
      uint32_t B2 = AES_T(TE, EK[6], T2, T3, T0, T1);
      uint32_t B3 = AES_T(TE, EK[7], T3, T0, T1, T2);

      for(size_t r = 2*4; r < EK.size(); r += 2*4)
         {
         T0 = AES_T(TE, EK[r  ], B0, B1, B2, B3);
         T1 = AES_T(TE, EK[r+1], B1, B2, B3, B0);
         T2 = AES_T(TE, EK[r+2], B2, B3, B0, B1);
         T3 = AES_T(TE, EK[r+3], B3, B0, B1, B2);

         B0 = AES_T(TE, EK[r+4], T0, T1, T2, T3);
         B1 = AES_T(TE, EK[r+5], T1, T2, T3, T0);
         B2 = AES_T(TE, EK[r+6], T2, T3, T0, T1);
         B3 = AES_T(TE, EK[r+7], T3, T0, T1, T2);
         }

      /*
      * Use TE[x] >> 8 instead of SE[] so encryption only references a single
      * lookup table.
      */
      out[16*i+ 0] = static_cast<uint8_t>(TE[get_byte(0, B0)] >> 8) ^ ME[0];
      out[16*i+ 1] = static_cast<uint8_t>(TE[get_byte(1, B1)] >> 8) ^ ME[1];
      out[16*i+ 2] = static_cast<uint8_t>(TE[get_byte(2, B2)] >> 8) ^ ME[2];
      out[16*i+ 3] = static_cast<uint8_t>(TE[get_byte(3, B3)] >> 8) ^ ME[3];
      out[16*i+ 4] = static_cast<uint8_t>(TE[get_byte(0, B1)] >> 8) ^ ME[4];
      out[16*i+ 5] = static_cast<uint8_t>(TE[get_byte(1, B2)] >> 8) ^ ME[5];
      out[16*i+ 6] = static_cast<uint8_t>(TE[get_byte(2, B3)] >> 8) ^ ME[6];
      out[16*i+ 7] = static_cast<uint8_t>(TE[get_byte(3, B0)] >> 8) ^ ME[7];
      out[16*i+ 8] = static_cast<uint8_t>(TE[get_byte(0, B2)] >> 8) ^ ME[8];
      out[16*i+ 9] = static_cast<uint8_t>(TE[get_byte(1, B3)] >> 8) ^ ME[9];
      out[16*i+10] = static_cast<uint8_t>(TE[get_byte(2, B0)] >> 8) ^ ME[10];
      out[16*i+11] = static_cast<uint8_t>(TE[get_byte(3, B1)] >> 8) ^ ME[11];
      out[16*i+12] = static_cast<uint8_t>(TE[get_byte(0, B3)] >> 8) ^ ME[12];
      out[16*i+13] = static_cast<uint8_t>(TE[get_byte(1, B0)] >> 8) ^ ME[13];
      out[16*i+14] = static_cast<uint8_t>(TE[get_byte(2, B1)] >> 8) ^ ME[14];
      out[16*i+15] = static_cast<uint8_t>(TE[get_byte(3, B2)] >> 8) ^ ME[15];
      }
   }

/*
* AES Decryption
*/
void aes_decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks,
                   const secure_vector<uint32_t>& DK,
                   const secure_vector<uint8_t>& MD)
   {
   BOTAN_ASSERT(DK.size() && MD.size() == 16, "Key was set");

   const size_t cache_line_size = CPUID::cache_line_size();
   const uint32_t* TD = AES_TD();

   volatile uint32_t Z = 0;
   for(size_t i = 0; i < 256; i += cache_line_size / sizeof(uint32_t))
      {
      Z |= TD[i];
      }
   for(size_t i = 0; i < 256; i += cache_line_size)
      {
      Z |= SD[i];
      }
   Z &= TD[99]; // this is zero, which hopefully the compiler cannot deduce

   for(size_t i = 0; i != blocks; ++i)
      {
      uint32_t T0 = load_be<uint32_t>(in, 0) ^ DK[0];
      uint32_t T1 = load_be<uint32_t>(in, 1) ^ DK[1];
      uint32_t T2 = load_be<uint32_t>(in, 2) ^ DK[2];
      uint32_t T3 = load_be<uint32_t>(in, 3) ^ DK[3];

      T0 ^= Z;

      uint32_t B0 = AES_T(TD, DK[4], T0, T3, T2, T1);
      uint32_t B1 = AES_T(TD, DK[5], T1, T0, T3, T2);
      uint32_t B2 = AES_T(TD, DK[6], T2, T1, T0, T3);
      uint32_t B3 = AES_T(TD, DK[7], T3, T2, T1, T0);

      for(size_t r = 2*4; r < DK.size(); r += 2*4)
         {
         T0 = AES_T(TD, DK[r  ], B0, B3, B2, B1);
         T1 = AES_T(TD, DK[r+1], B1, B0, B3, B2);
         T2 = AES_T(TD, DK[r+2], B2, B1, B0, B3);
         T3 = AES_T(TD, DK[r+3], B3, B2, B1, B0);

         B0 = AES_T(TD, DK[r+4], T0, T3, T2, T1);
         B1 = AES_T(TD, DK[r+5], T1, T0, T3, T2);
         B2 = AES_T(TD, DK[r+6], T2, T1, T0, T3);
         B3 = AES_T(TD, DK[r+7], T3, T2, T1, T0);
         }

      out[ 0] = SD[get_byte(0, B0)] ^ MD[0];
      out[ 1] = SD[get_byte(1, B3)] ^ MD[1];
      out[ 2] = SD[get_byte(2, B2)] ^ MD[2];
      out[ 3] = SD[get_byte(3, B1)] ^ MD[3];
      out[ 4] = SD[get_byte(0, B1)] ^ MD[4];
      out[ 5] = SD[get_byte(1, B0)] ^ MD[5];
      out[ 6] = SD[get_byte(2, B3)] ^ MD[6];
      out[ 7] = SD[get_byte(3, B2)] ^ MD[7];
      out[ 8] = SD[get_byte(0, B2)] ^ MD[8];
      out[ 9] = SD[get_byte(1, B1)] ^ MD[9];
      out[10] = SD[get_byte(2, B0)] ^ MD[10];
      out[11] = SD[get_byte(3, B3)] ^ MD[11];
      out[12] = SD[get_byte(0, B3)] ^ MD[12];
      out[13] = SD[get_byte(1, B2)] ^ MD[13];
      out[14] = SD[get_byte(2, B1)] ^ MD[14];
      out[15] = SD[get_byte(3, B0)] ^ MD[15];

      in += 16;
      out += 16;
      }
   }

#undef AES_T

void aes_key_schedule(const uint8_t key[], size_t length,
                      secure_vector<uint32_t>& EK,
                      secure_vector<uint32_t>& DK,
                      secure_vector<uint8_t>& ME,
                      secure_vector<uint8_t>& MD)
   {
   static const uint32_t RC[10] = {
      0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000,
      0x20000000, 0x40000000, 0x80000000, 0x1B000000, 0x36000000 };

   const size_t X = length / 4;

   // Can't happen, but make static analyzers happy
   BOTAN_ASSERT_NOMSG(X == 4 || X == 6 || X == 8);

   const size_t rounds = (length / 4) + 6;

   CT::poison(key, length);

   secure_vector<uint32_t> XEK(length + 32);
   secure_vector<uint32_t> XDK(length + 32);

   for(size_t i = 0; i != X; ++i)
      XEK[i] = load_be<uint32_t>(key, i);

   for(size_t i = X; i < 4*(rounds+1); i += X)
      {
      XEK[i] = XEK[i-X] ^ RC[(i-X)/X] ^ rotl<8>(SE_word(XEK[i-1]));

      for(size_t j = 1; j != X; ++j)
         {
         XEK[i+j] = XEK[i+j-X];

         if(X == 8 && j == 4)
            XEK[i+j] ^= SE_word(XEK[i+j-1]);
         else
            XEK[i+j] ^= XEK[i+j-1];
         }
      }

   for(size_t i = 0; i != 4*(rounds+1); i += 4)
      {
      XDK[i  ] = XEK[4*rounds-i  ];
      XDK[i+1] = XEK[4*rounds-i+1];
      XDK[i+2] = XEK[4*rounds-i+2];
      XDK[i+3] = XEK[4*rounds-i+3];
      }

   for(size_t i = 4; i != length + 24; ++i)
      {
      const uint8_t s0 = get_byte(0, XDK[i]);
      const uint8_t s1 = get_byte(1, XDK[i]);
      const uint8_t s2 = get_byte(2, XDK[i]);
      const uint8_t s3 = get_byte(3, XDK[i]);

      XDK[i] = InvMixColumn(s0) ^
         rotr<8>(InvMixColumn(s1)) ^
         rotr<16>(InvMixColumn(s2)) ^
         rotr<24>(InvMixColumn(s3));
      }

   ME.resize(16);
   MD.resize(16);

   for(size_t i = 0; i != 4; ++i)
      {
      store_be(XEK[i+4*rounds], &ME[4*i]);
      store_be(XEK[i], &MD[4*i]);
      }

   EK.resize(length + 24);
   DK.resize(length + 24);
   copy_mem(EK.data(), XEK.data(), EK.size());
   copy_mem(DK.data(), XDK.data(), DK.size());

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      // ARM needs the subkeys to be byte reversed

      for(size_t i = 0; i != EK.size(); ++i)
         EK[i] = reverse_bytes(EK[i]);
      for(size_t i = 0; i != DK.size(); ++i)
         DK[i] = reverse_bytes(DK[i]);
      }
#endif

   CT::unpoison(EK.data(), EK.size());
   CT::unpoison(DK.data(), DK.size());
   CT::unpoison(ME.data(), ME.size());
   CT::unpoison(MD.data(), MD.size());
   CT::unpoison(key, length);
   }

size_t aes_parallelism()
   {
#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return 4;
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return 4;
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return 4;
      }
#endif

   return 1;
   }

const char* aes_provider()
   {
#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return "aesni";
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return "power8";
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return "armv8";
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return "vperm";
      }
#endif

   return "base";
   }

}

std::string AES_128::provider() const { return aes_provider(); }
std::string AES_192::provider() const { return aes_provider(); }
std::string AES_256::provider() const { return aes_provider(); }

size_t AES_128::parallelism() const { return aes_parallelism(); }
size_t AES_192::parallelism() const { return aes_parallelism(); }
size_t AES_256::parallelism() const { return aes_parallelism(); }

void AES_128::encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_EK.empty() == false);

#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return armv8_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return power8_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_encrypt_n(in, out, blocks);
      }
#endif

   aes_encrypt_n(in, out, blocks, m_EK, m_ME);
   }

void AES_128::decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_DK.empty() == false);

#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return armv8_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return power8_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_decrypt_n(in, out, blocks);
      }
#endif

   aes_decrypt_n(in, out, blocks, m_DK, m_MD);
   }

void AES_128::key_schedule(const uint8_t key[], size_t length)
   {
#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_key_schedule(key, length);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_key_schedule(key, length);
      }
#endif

   aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
   }

void AES_128::clear()
   {
   zap(m_EK);
   zap(m_DK);
   zap(m_ME);
   zap(m_MD);
   }

void AES_192::encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_EK.empty() == false);

#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return armv8_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return power8_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_encrypt_n(in, out, blocks);
      }
#endif

   aes_encrypt_n(in, out, blocks, m_EK, m_ME);
   }

void AES_192::decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_DK.empty() == false);

#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return armv8_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return power8_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_decrypt_n(in, out, blocks);
      }
#endif

   aes_decrypt_n(in, out, blocks, m_DK, m_MD);
   }

void AES_192::key_schedule(const uint8_t key[], size_t length)
   {
#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_key_schedule(key, length);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_key_schedule(key, length);
      }
#endif

   aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
   }

void AES_192::clear()
   {
   zap(m_EK);
   zap(m_DK);
   zap(m_ME);
   zap(m_MD);
   }

void AES_256::encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_EK.empty() == false);

#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return armv8_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return power8_encrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_encrypt_n(in, out, blocks);
      }
#endif

   aes_encrypt_n(in, out, blocks, m_EK, m_ME);
   }

void AES_256::decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const
   {
   verify_key_set(m_DK.empty() == false);

#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return armv8_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return power8_decrypt_n(in, out, blocks);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_decrypt_n(in, out, blocks);
      }
#endif

   aes_decrypt_n(in, out, blocks, m_DK, m_MD);
   }

void AES_256::key_schedule(const uint8_t key[], size_t length)
   {
#if defined(BOTAN_HAS_AES_NI)
   if(CPUID::has_aes_ni())
      {
      return aesni_key_schedule(key, length);
      }
#endif

#if defined(BOTAN_HAS_AES_ARMV8)
   if(CPUID::has_arm_aes())
      {
      return aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
      }
#endif

#if defined(BOTAN_HAS_AES_POWER8)
   if(CPUID::has_power_crypto())
      {
      return aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
      }
#endif

#if defined(BOTAN_HAS_AES_VPERM)
   if(CPUID::has_vperm())
      {
      return vperm_key_schedule(key, length);
      }
#endif

   aes_key_schedule(key, length, m_EK, m_DK, m_ME, m_MD);
   }

void AES_256::clear()
   {
   zap(m_EK);
   zap(m_DK);
   zap(m_ME);
   zap(m_MD);
   }

}
