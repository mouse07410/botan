/*
* Certificate Store
* (C) 1999-2019 Jack Lloyd
* (C) 2019      Patrick Schmidt
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/build.h>

#include <botan/certstor_flatfile.h>
#include <botan/data_src.h>
#include <botan/pem.h>

namespace Botan {
namespace {
std::vector<std::vector<uint8_t>> decode_all_certificates(DataSource& source)
   {
   std::vector<std::vector<uint8_t>> pems;

   while(!source.end_of_data())
      {
      std::string label;
      std::vector<uint8_t> cert;
      try
         {
         cert = unlock(PEM_Code::decode(source, label));

         if(label == "CERTIFICATE" || label == "X509 CERTIFICATE" || label == "TRUSTED CERTIFICATE")
            {
            pems.push_back(cert);
            }
         }
      catch(const Decoding_Error&) {}
      }

   return pems;
   }
}

Flatfile_Certificate_Store::Flatfile_Certificate_Store(const std::string& file, bool ignore_non_ca)
   {
   if(file.empty())
      {
      throw Invalid_Argument("Flatfile_Certificate_Store::Flatfile_Certificate_Store invalid file path");
      }

   DataSource_Stream file_stream(file);

   for(const std::vector<uint8_t> der : decode_all_certificates(file_stream))
      {
      std::shared_ptr<const X509_Certificate> cert = std::make_shared<const X509_Certificate>(der.data(), der.size());

      /*
      * Various weird or misconfigured system roots include intermediate certificates,
      * or even stranger certificates which are not valid for cert issuance at all.
      * Previously this code would error on such cases as an obvious misconfiguration,
      * but we cannot fix the trust store. So instead just ignore any such certificate.
      */
      if(cert->is_self_signed() && cert->is_CA_cert())
         {
         m_all_subjects.push_back(cert->subject_dn());
         m_dn_to_cert.emplace(cert->subject_dn(), cert);
         m_pubkey_sha1_to_cert.emplace(cert->subject_public_key_bitstring_sha1(), cert);
         m_subject_dn_sha256_to_cert.emplace(cert->raw_subject_dn_sha256(), cert);
         }
      else if(!ignore_non_ca)
         {
         throw Invalid_Argument("Flatfile_Certificate_Store received non CA cert " + cert->subject_dn().to_string());
         }
      }

   if(m_all_subjects.empty())
      {
      throw Invalid_Argument("Flatfile_Certificate_Store::Flatfile_Certificate_Store cert file is empty");
      }
   }

std::vector<X509_DN> Flatfile_Certificate_Store::all_subjects() const
   {
   return m_all_subjects;
   }

std::shared_ptr<const X509_Certificate>
Flatfile_Certificate_Store::find_cert(const X509_DN& subject_dn,
                                      const std::vector<uint8_t>& key_id) const
   {
   std::vector<std::shared_ptr<const X509_Certificate>> found_certs = find_all_certs(subject_dn, key_id);

   return !found_certs.empty() ? found_certs.front() : nullptr;
   }

std::vector<std::shared_ptr<const X509_Certificate>> Flatfile_Certificate_Store::find_all_certs(
         const X509_DN& subject_dn,
         const std::vector<uint8_t>& key_id) const
   {
   const auto found_range = m_dn_to_cert.equal_range(subject_dn);

   if(found_range.first == m_dn_to_cert.end())
      {
      return {};
      }

   std::vector<std::shared_ptr<const X509_Certificate>> found_certs;

   for(auto i = found_range.first; i != found_range.second; ++i)
      {
      const std::shared_ptr<const X509_Certificate> cert = i->second;

      if(key_id.empty() || key_id == cert->subject_key_id())
         {
         found_certs.push_back(cert);
         }
      }

   return found_certs;
   }

std::shared_ptr<const X509_Certificate>
Flatfile_Certificate_Store::find_cert_by_pubkey_sha1(const std::vector<uint8_t>& key_hash) const
   {
   if(key_hash.size() != 20)
      {
      throw Invalid_Argument("Flatfile_Certificate_Store::find_cert_by_pubkey_sha1 invalid hash");
      }

   auto found_cert = m_pubkey_sha1_to_cert.find(key_hash);

   if(found_cert != m_pubkey_sha1_to_cert.end())
      {
      return found_cert->second;
      }

   return nullptr;
   }

std::shared_ptr<const X509_Certificate>
Flatfile_Certificate_Store::find_cert_by_raw_subject_dn_sha256(const std::vector<uint8_t>& subject_hash) const
   {
   if(subject_hash.size() != 32)
      { throw Invalid_Argument("Flatfile_Certificate_Store::find_cert_by_raw_subject_dn_sha256 invalid hash"); }

   auto found_cert = m_subject_dn_sha256_to_cert.find(subject_hash);

   if(found_cert != m_subject_dn_sha256_to_cert.end())
      {
      return found_cert->second;
      }

   return nullptr;
   }

std::shared_ptr<const X509_CRL> Flatfile_Certificate_Store::find_crl_for(const X509_Certificate& subject) const
   {
   BOTAN_UNUSED(subject);
   return {};
   }
}
