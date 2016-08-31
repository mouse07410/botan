/*
* TLS Handshake Message
* (C) 2012 Jack Lloyd
*     2016 Matthias Gierlings
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_HANDSHAKE_MSG_H__
#define BOTAN_TLS_HANDSHAKE_MSG_H__

#include <botan/tls_magic.h>
#include <vector>
#include <string>

namespace Botan {

namespace TLS {

class Handshake_IO;
class Handshake_Hash;

/**
* TLS Handshake Message Base Class
*/
class BOTAN_DLL Handshake_Message
   {
   public:
      std::string type_string() const;

      virtual Handshake_Type type() const = 0;

      virtual std::vector<byte> serialize() const = 0;

      virtual ~Handshake_Message() {}
   };

}

}

#endif
