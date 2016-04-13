/*
* Simple ASN.1 String Types
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/asn1_str.h>
#include <botan/der_enc.h>
#include <botan/ber_dec.h>
#include <botan/charset.h>
#include <botan/parsing.h>

namespace Botan {

namespace {

/*
* Choose an encoding for the string
*/
ASN1_Tag choose_encoding(const std::string& str,
                         const std::string& type)
   {
   static const byte IS_PRINTABLE[256] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00,
      0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
      0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00 };

   for(size_t i = 0; i != str.size(); ++i)
      {
      if(!IS_PRINTABLE[static_cast<byte>(str[i])])
         {
         if(type == "utf8")   return UTF8_STRING;
         if(type == "latin1") return T61_STRING;
         throw Invalid_Argument("choose_encoding: Bad string type " + type);
         }
      }
   return PRINTABLE_STRING;
   }

}

/*
* Create an ASN1_String
*/
ASN1_String::ASN1_String(const std::string& str, ASN1_Tag t) : m_iso_8859_str(Charset::transcode(str, LOCAL_CHARSET, LATIN1_CHARSET)), m_tag(t)
   {

   if(m_tag == DIRECTORY_STRING)
      m_tag = choose_encoding(m_iso_8859_str, "latin1");

   if(m_tag != NUMERIC_STRING &&
      m_tag != PRINTABLE_STRING &&
      m_tag != VISIBLE_STRING &&
      m_tag != T61_STRING &&
      m_tag != IA5_STRING &&
      m_tag != UTF8_STRING &&
      m_tag != BMP_STRING)
      throw Invalid_Argument("ASN1_String: Unknown string type " +
                             std::to_string(m_tag));
   }

/*
* Create an ASN1_String
*/
ASN1_String::ASN1_String(const std::string& str) : m_iso_8859_str(Charset::transcode(str, LOCAL_CHARSET, LATIN1_CHARSET)), m_tag(choose_encoding(m_iso_8859_str, "latin1"))
   {}

/*
* Return this string in ISO 8859-1 encoding
*/
std::string ASN1_String::iso_8859() const
   {
   return m_iso_8859_str;
   }

/*
* Return this string in local encoding
*/
std::string ASN1_String::value() const
   {
   return Charset::transcode(m_iso_8859_str, LATIN1_CHARSET, LOCAL_CHARSET);
   }

/*
* Return the type of this string object
*/
ASN1_Tag ASN1_String::tagging() const
   {
   return m_tag;
   }

/*
* DER encode an ASN1_String
*/
void ASN1_String::encode_into(DER_Encoder& encoder) const
   {
   std::string value = iso_8859();
   if(tagging() == UTF8_STRING)
      value = Charset::transcode(value, LATIN1_CHARSET, UTF8_CHARSET);
   encoder.add_object(tagging(), UNIVERSAL, value);
   }

/*
* Decode a BER encoded ASN1_String
*/
void ASN1_String::decode_from(BER_Decoder& source)
   {
   BER_Object obj = source.get_next_object();

   Character_Set charset_is;

   if(obj.type_tag == BMP_STRING)
      charset_is = UCS2_CHARSET;
   else if(obj.type_tag == UTF8_STRING)
      charset_is = UTF8_CHARSET;
   else
      charset_is = LATIN1_CHARSET;

   *this = ASN1_String(
      Charset::transcode(ASN1::to_string(obj), LOCAL_CHARSET, charset_is),
      obj.type_tag);
   }

}
