/**
 * XMSS Parameters
 * Descibes a signature method for XMSS, as defined in:
 * [1] XMSS: Extended Hash-Based Signatures,
 *     draft-itrf-cfrg-xmss-hash-based-signatures-06
 *     Release: July 2016.
 *     https://datatracker.ietf.org/doc/
 *     draft-irtf-cfrg-xmss-hash-based-signatures/?include_text=1
 *
 * (C) 2016 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#include <botan/xmss_parameters.h>

namespace Botan {

const std::map<std::string, XMSS_Parameters::xmss_algorithm_t>
   XMSS_Parameters::m_oid_name_lut =
      {
      { "XMSS_SHA2-256_W16_H10", XMSS_SHA2_256_W16_H10 },
      { "XMSS_SHA2-256_W16_H16", XMSS_SHA2_256_W16_H16 },
      { "XMSS_SHA2-256_W16_H20", XMSS_SHA2_256_W16_H20 },
      { "XMSS_SHA2-512_W16_H10", XMSS_SHA2_512_W16_H10 },
      { "XMSS_SHA2-512_W16_H16", XMSS_SHA2_512_W16_H16 },
      { "XMSS_SHA2-512_W16_H20", XMSS_SHA2_512_W16_H20 }
//      { "XMSS_SHAKE128_W16_H10", xmss_algorithm_t::XMSS_SHAKE128_W16_H10 },
//      { "XMSS_SHAKE128_W16_H16", xmss_algorithm_t::XMSS_SHAKE128_W16_H16 },
//      { "XMSS_SHAKE128_W16_H20", xmss_algorithm_t::XMSS_SHAKE128_W16_H20 },
//      { "XMSS_SHAKE256_W16_H10", xmss_algorithm_t::XMSS_SHAKE256_W16_H10 },
//      { "XMSS_SHAKE256_W16_H16", xmss_algorithm_t::XMSS_SHAKE256_W16_H16 },
//      { "XMSS_SHAKE256_W16_H20", xmss_algorithm_t::XMSS_SHAKE256_W16_H20 }
      };

XMSS_Parameters::XMSS_Parameters(const std::string& algo_name)
   : XMSS_Parameters(m_oid_name_lut.at(algo_name))
   {}

XMSS_Parameters::XMSS_Parameters(xmss_algorithm_t oid)
   : m_oid(oid)
   {
   switch(oid)
      {
      case XMSS_SHA2_256_W16_H10:
         m_element_size = 32;
         m_w = 16;
         m_len = 67;
         m_tree_height = 10;
         m_name = "XMSS_SHA2-256_W16_H10";
         m_hash_name = "SHA-256";
         m_strength = 256;
         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHA2_256_W16;
         break;
      case XMSS_SHA2_256_W16_H16:
         m_element_size = 32;
         m_w = 16;
         m_len = 67;
         m_tree_height = 16;
         m_name = "XMSS_SHA2-256_W16_H16";
         m_hash_name = "SHA-256";
         m_strength = 256;
         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHA2_256_W16;
         break;
      case XMSS_SHA2_256_W16_H20:
         m_element_size = 32;
         m_w = 16;
         m_len = 67;
         m_tree_height = 20;
         m_name = "XMSS_SHA2-256_W16_H20";
         m_hash_name = "SHA-256";
         m_strength = 256;
         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHA2_256_W16;
         break;
      case XMSS_SHA2_512_W16_H10:
         m_element_size = 64;
         m_w = 16;
         m_len = 131;
         m_tree_height = 10;
         m_name = "XMSS_SHA2-512_W16_H10";
         m_hash_name = "SHA-512";
         m_strength = 512;
         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHA2_512_W16;
         break;
      case XMSS_SHA2_512_W16_H16:
         m_element_size = 64;
         m_w = 16;
         m_len = 131;
         m_tree_height = 16;
         m_name = "XMSS_SHA2-512_W16_H16";
         m_hash_name = "SHA-512";
         m_strength = 512;
         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHA2_512_W16;
         break;
      case XMSS_SHA2_512_W16_H20:
         m_element_size = 64;
         m_w = 16;
         m_len = 131;
         m_tree_height = 20;
         m_name = "XMSS_SHA2-512_W16_H20";
         m_hash_name = "SHA-512";
         m_strength = 512;
         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHA2_512_W16;
         break;
// FIXME: Uncomment once SHAKE128/256 implementation is available in Botan.
//      case XMSS_SHAKE128_W16_H10:
//         m_element_size = 32;
//         m_w = 16;
//         m_len = 67;
//         m_tree_height = 10;
//         m_name = "XMSS_SHAKE128_W16_H10";
//         m_hash_name = "";
//         m_strength = 256;
//         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHAKE128_W16;
//         BOTAN_ASSERT(false, "XMSS_SHAKE128_W16_H10 not implemented.");
//         break;
//      case XMSS_SHAKE128_W16_H16:
//         m_element_size = 32;
//         m_w = 16;
//         m_len = 67;
//         m_tree_height = 16;
//         m_name = "XMSS_SHAKE128_W16_H16";
//         m_hash_name = "";
//         m_strength = 256;
//         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHAKE128_W16;
//         BOTAN_ASSERT(false, "XMSS_SHAKE128_W16_H16 not implemented.");
//         break;
//      case XMSS_SHAKE128_W16_H20:
//         m_element_size = 32;
//         m_w = 16;
//         m_len = 67;
//         m_tree_height = 20;
//         m_name = "XMSS_SHAKE128_W16_H20";
//         m_hash_name = "";
//         m_strength = 256;
//         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHAKE128_W16;
//         BOTAN_ASSERT(false, "XMSS_SHAKE128_W16_H20 not implemented.");
//         break;
//      case XMSS_SHAKE256_W16_H10:
//         m_element_size = 64;
//         m_w = 16;
//         m_len = 131;
//         m_tree_height = 10;
//         m_name = "XMSS_SHAKE256_W16_H10";
//         m_hash_name = "";
//         m_strength = 512;
//         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHAKE256_W16;
//         BOTAN_ASSERT(false, "XMSS_SHAKE256_W16_H10 not implemented.");
//         break;
//      case XMSS_SHAKE256_W16_H16:
//         m_element_size = 64;
//         m_w = 16;
//         m_len = 131;
//         m_tree_height = 16;
//         m_name = "XMSS_SHAKE256_W16_H16";
//         m_hash_name = "";
//         m_strength = 512;
//         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHAKE256_W16;
//         BOTAN_ASSERT(false, "XMSS_SHAKE256_W16_H16 not implemented.");
//         break;
//      case XMSS_SHAKE256_W16_H20:
//         m_element_size = 64;
//         m_w = 16;
//         m_len = 131;
//         m_tree_height = 20;
//         m_name = "XMSS_SHAKE256_W16_H20";
//         m_hash_name = "";
//         m_strength = 512;
//         m_wots_oid = XMSS_WOTS_Parameters::ots_algorithm_t::WOTSP_SHAKE256_W16;
//         BOTAN_ASSERT(false, "XMSS_SHAKE256_W16_H20 not implemented.");
//         break;
      default:
         throw Unsupported_Argument(
            "Algorithm id does not match any XMSS algorithm id.");
         break;
      }
   }

}
