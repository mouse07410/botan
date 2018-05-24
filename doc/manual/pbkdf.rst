
.. _pbkdf:

PBKDF Algorithms
========================================

There are various procedures for turning a passphrase into a arbitrary
length key for use with a symmetric cipher. A general interface for
such algorithms is presented in ``pbkdf.h``. The main function is
``derive_key``, which takes a passphrase, a salt, an iteration count,
and the desired length of the output key, and returns a key of that
length, deterministically produced from the passphrase and salt. If an
algorithm can't produce a key of that size, it will throw an exception
(most notably, PKCS #5's PBKDF1 can only produce strings between 1 and
$n$ bytes, where $n$ is the output size of the underlying hash
function).

The purpose of the iteration count is to make the algorithm take
longer to compute the final key (reducing the speed of brute-force
attacks of various kinds). Most standards recommend an iteration count
of at least 10000. Currently defined PBKDF algorithms are
"PBKDF1(digest)", "PBKDF2(digest)"; you can retrieve any of these
using the ``get_pbkdf``, found in ``lookup.h``. As of this writing,
"PBKDF2(SHA-256)" with at least 100000 iterations and a 16 byte salt
is recommend for new applications.

.. cpp:function:: OctetString PBKDF::derive_key( \
   size_t output_len, const std::string& passphrase, \
   const uint8_t* salt, size_t salt_len, \
   size_t iterations) const

   Computes a key from *passphrase* and the *salt* (of length
   *salt_len* bytes) using an algorithm-specific interpretation of
   *iterations*, producing a key of length *output_len*.

   Use an iteration count of at least 10000. The salt should be
   randomly chosen by a good random number generator (see
   :ref:`random_number_generators` for how), or at the very least
   unique to this usage of the passphrase.

   If you call this function again with the same parameters, you will
   get the same key.

::

   PBKDF* pbkdf = get_pbkdf("PBKDF2(SHA-256)");
   AutoSeeded_RNG rng;

   secure_vector<uint8_t> salt = rng.random_vec(16);
   OctetString aes256_key = pbkdf->derive_key(32, "password",
                                              &salt[0], salt.size(),
                                              10000);

PBKDF1
------------

PBKDF1 is an old scheme that can only produce an output length at most
as long as the hash function. It is deprecated and will be removed in
a future release.

PBKDF2
------------

PBKDF2 is a the "standard" password derivation scheme, widely
implemented in many different libraries.

OpenPGP S2K
-------------

There are some oddities about OpenPGP's S2K algorithms that are
documented here. For one thing, it uses the iteration count in a
strange manner; instead of specifying how many times to iterate the
hash, it tells how many *bytes* should be hashed in total
(including the salt). So the exact iteration count will depend on the
size of the salt (which is fixed at 8 bytes by the OpenPGP standard,
though the implementation will allow any salt size) and the size of
the passphrase.

To get what OpenPGP calls "Simple S2K", set iterations to 0, and do
not specify a salt. To get "Salted S2K", again leave the iteration
count at 0, but give an 8-byte salt. "Salted and Iterated S2K"
requires an 8-byte salt and some iteration count (this should be
significantly larger than the size of the longest passphrase that
might reasonably be used; somewhere from 1024 to 65536 would probably
be about right). Using both a reasonably sized salt and a large
iteration count is highly recommended to prevent password guessing
attempts.

Scrypt
----------

Scrypt is a relatively newer design which is "memory hard" - in
addition to requiring large amounts of CPU power it uses a large block
of memory to compute the hash. This makes brute force attacks using
ASICs substantially more expensive.

Currently Scrypt uses a different interface from the standard PBKDF
functions. This will be remedied in a future major release which
redesigns the PBKDF interfaces.

.. cpp:function:: void scrypt(uint8_t output[], size_t output_len, \
                              const std::string& password, \
                              const uint8_t salt[], size_t salt_len, \
                              size_t N, size_t r, size_t p)

   Computes the Scrypt using the password and salt, and produces an output
   of arbitrary length.

   The N, r, p parameters control how much work and memory Scrypt
   uses.  N is the primary control of the workfactor, and must be a
   power of 2. For interactive logins use 32768, for protection of
   secret keys or backups use 1048576.

   The r parameter controls how 'wide' the internal hashing operation
   is. It also increases the amount of memory that is used. Values
   from 1 to 8 are reasonable.

   Setting p parameter to greater than one splits up the work in a way
   that up to p processors can work in parallel.

   As a general recommendation, use N=32768, r=8, p=1
