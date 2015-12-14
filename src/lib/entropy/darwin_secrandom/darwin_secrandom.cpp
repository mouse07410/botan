/*
* Darwin SecRandomCopyBytes EntropySource
* (C) 2015 Daniel Seither (Kullo GmbH)
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include <botan/internal/darwin_secrandom.h>
#include <Security/Security.h>

namespace Botan {

/**
* Gather entropy from SecRandomCopyBytes
*/
void Darwin_SecRandom::poll(Entropy_Accumulator& accum)
   {
   secure_vector<byte>& buf = accum.get_io_buf(BOTAN_SYSTEM_RNG_POLL_REQUEST);

   if(0 == SecRandomCopyBytes(kSecRandomDefault, buf.size(), buf.data()))
      {
      accum.add(buf.data(), buf.size(), BOTAN_ENTROPY_ESTIMATE_STRONG_RNG);
      }
   }

}
