/**
 * XMSS Hash
 * A collection of pseudorandom hash functions required for XMSS and WOTS
 * computations.
 * (C) 2016 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#include <botan/xmss_hash.h>

namespace Botan {

XMSS_Hash::XMSS_Hash(const XMSS_Hash& hash)
   : XMSS_Hash(hash.m_hash_func_name)
   {
   }

XMSS_Hash::XMSS_Hash(const std::string& h_func_name)
   : m_hash(HashFunction::create(h_func_name)),
     m_msg_hash(HashFunction::create(h_func_name)),
     m_output_length(m_hash->output_length()),
     m_zero_padding(m_output_length - 1, 0x00),
     m_hash_func_name(h_func_name)
   {
   BOTAN_ASSERT(m_output_length > 0, "Hash output length of zero is invalid.");
   }

void
XMSS_Hash::h(secure_vector<byte>& result,
             const secure_vector<byte>& key,
             const secure_vector<byte>& data)
   {
   m_hash->update(m_zero_padding);
   m_hash->update(m_id_h);
   m_hash->update(key);
   m_hash->update(data);
   m_hash->final(result);
   }

void XMSS_Hash::h_msg_init(const secure_vector<byte>& randomness,
                           const secure_vector<byte>& root,
                           const secure_vector<byte>& index_bytes)
   {
   m_msg_hash->clear();
   m_msg_hash->update(m_zero_padding);
   m_msg_hash->update(m_id_hmsg);
   m_msg_hash->update(randomness);
   m_msg_hash->update(root);
   m_msg_hash->update(index_bytes);
   }

void XMSS_Hash::h_msg_update(const secure_vector<byte>& data)
   {
   m_msg_hash->update(data);
   }

void XMSS_Hash::h_msg_update(const byte data[], size_t size)
   {
   m_msg_hash->update(data, size);
   }

secure_vector<byte> XMSS_Hash::h_msg_final()
   {
   return m_msg_hash->final();
   }

secure_vector<byte>
XMSS_Hash::h_msg(const secure_vector<byte>& randomness,
                 const secure_vector<byte>& root,
                 const secure_vector<byte>& index_bytes,
                 const secure_vector<byte>& data)
   {
   h_msg_init(randomness, root, index_bytes);
   m_msg_hash->update(data);
   return m_msg_hash->final();
   }

}
