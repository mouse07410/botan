/*
* Division
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DIVISON_ALGORITHM_H_
#define BOTAN_DIVISON_ALGORITHM_H_

#include <botan/bigint.h>

namespace Botan {

/**
* BigInt Division
* @param x an integer
* @param y a non-zero integer
* @param q will be set to x / y
* @param r will be set to x % y
*/
void BOTAN_PUBLIC_API(2,0) divide(const BigInt& x,
                                  const BigInt& y,
                                  BigInt& q,
                                  BigInt& r);

/**
* BigInt division, const time variant
*
* This runs with control flow independent of the values of x/y.
* Warning: the loop bounds still leak the sizes of x and y.
*
* @param x an integer
* @param y a non-zero integer
* @param q will be set to x / y
* @param r will be set to x % y
*/
void BOTAN_PUBLIC_API(2,9) ct_divide(const BigInt& x,
                                     const BigInt& y,
                                     BigInt& q,
                                     BigInt& r);

}

#endif
