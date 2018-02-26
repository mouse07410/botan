/*
* Point arithmetic on elliptic curves over GF(p)
*
* (C) 2007 Martin Doering, Christoph Ludwig, Falko Strenzke
*     2008-2011,2014,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_POINT_GFP_H_
#define BOTAN_POINT_GFP_H_

#include <botan/curve_gfp.h>
#include <vector>

namespace Botan {

/**
* Exception thrown if you try to convert a zero point to an affine
* coordinate
*/
class BOTAN_PUBLIC_API(2,0) Illegal_Transformation final : public Exception
   {
   public:
      explicit Illegal_Transformation(const std::string& err =
                                      "Requested transformation is not possible") :
         Exception(err) {}
   };

/**
* Exception thrown if some form of illegal point is decoded
*/
class BOTAN_PUBLIC_API(2,0) Illegal_Point final : public Exception
   {
   public:
      explicit Illegal_Point(const std::string& err = "Malformed ECP point detected") :
         Exception(err) {}
   };

/**
* This class represents one point on a curve of GF(p)
*/
class BOTAN_PUBLIC_API(2,0) PointGFp final
   {
   public:
      enum Compression_Type {
         UNCOMPRESSED = 0,
         COMPRESSED   = 1,
         HYBRID       = 2
      };

      enum { WORKSPACE_SIZE = 10 };

      /**
      * Construct an uninitialized PointGFp
      */
      PointGFp() = default;

      /**
      * Construct the zero point
      * @param curve The base curve
      */
      explicit PointGFp(const CurveGFp& curve);

      /**
      * Copy constructor
      */
      PointGFp(const PointGFp&) = default;

      /**
      * Move Constructor
      */
      PointGFp(PointGFp&& other)
         {
         this->swap(other);
         }

      /**
      * Standard Assignment
      */
      PointGFp& operator=(const PointGFp&) = default;

      /**
      * Move Assignment
      */
      PointGFp& operator=(PointGFp&& other)
         {
         if(this != &other)
            this->swap(other);
         return (*this);
         }

      /**
      * Construct a point from its affine coordinates
      * @param curve the base curve
      * @param x affine x coordinate
      * @param y affine y coordinate
      */
      PointGFp(const CurveGFp& curve, const BigInt& x, const BigInt& y);

      /**
      * += Operator
      * @param rhs the PointGFp to add to the local value
      * @result resulting PointGFp
      */
      PointGFp& operator+=(const PointGFp& rhs);

      /**
      * -= Operator
      * @param rhs the PointGFp to subtract from the local value
      * @result resulting PointGFp
      */
      PointGFp& operator-=(const PointGFp& rhs);

      /**
      * *= Operator
      * @param scalar the PointGFp to multiply with *this
      * @result resulting PointGFp
      */
      PointGFp& operator*=(const BigInt& scalar);

      /**
      * Negate this point
      * @return *this
      */
      PointGFp& negate()
         {
         if(!is_zero())
            m_coord_y = m_curve.get_p() - m_coord_y;
         return *this;
         }

      /**
      * Return base curve of this point
      * @result the curve over GF(p) of this point
      */
      const CurveGFp& get_curve() const { return m_curve; }

      /**
      * get affine x coordinate
      * @result affine x coordinate
      */
      BigInt get_affine_x() const;

      /**
      * get affine y coordinate
      * @result affine y coordinate
      */
      BigInt get_affine_y() const;

      /**
      * Is this the point at infinity?
      * @result true, if this point is at infinity, false otherwise.
      */
      bool is_zero() const
         { return (m_coord_x.is_zero() && m_coord_z.is_zero()); }

      /**
      * Checks whether the point is to be found on the underlying
      * curve; used to prevent fault attacks.
      * @return if the point is on the curve
      */
      bool on_the_curve() const;

      /**
      * swaps the states of *this and other, does not throw!
      * @param other the object to swap values with
      */
      void swap(PointGFp& other);

      /**
      * Randomize the point representation
      * The actual value (get_affine_x, get_affine_y) does not change
      */
      void randomize_repr(RandomNumberGenerator& rng);

      /**
      * Equality operator
      */
      bool operator==(const PointGFp& other) const;

      /**
      * Point addition
      * @param workspace temp space, at least WORKSPACE_SIZE elements
      */
      void add(const PointGFp& other, std::vector<BigInt>& workspace);

      /**
      * Point doubling
      * @param workspace temp space, at least WORKSPACE_SIZE elements
      */
      void mult2(std::vector<BigInt>& workspace);

      /**
      * Point addition
      * @param workspace temp space, at least WORKSPACE_SIZE elements
      */
      PointGFp plus(const PointGFp& other, std::vector<BigInt>& workspace) const
         {
         PointGFp x = (*this);
         x.add(other, workspace);
         return x;
         }

      /**
      * Return the zero (aka infinite) point associated with this curve
      */
      PointGFp zero() const { return PointGFp(m_curve); }

   private:
      CurveGFp m_curve;
      BigInt m_coord_x, m_coord_y, m_coord_z;
   };

/**
* Point multiplication operator
* @param scalar the scalar value
* @param point the point value
* @return scalar*point on the curve
*/
BOTAN_PUBLIC_API(2,0) PointGFp operator*(const BigInt& scalar, const PointGFp& point);

/**
* ECC point multiexponentiation - not constant time!
* @param p1 a point
* @param z1 a scalar
* @param p2 a point
* @param z2 a scalar
* @result (p1 * z1 + p2 * z2)
*/
BOTAN_PUBLIC_API(2,0) PointGFp multi_exponentiate(
   const PointGFp& p1, const BigInt& z1,
   const PointGFp& p2, const BigInt& z2);

// relational operators
inline bool operator!=(const PointGFp& lhs, const PointGFp& rhs)
   {
   return !(rhs == lhs);
   }

// arithmetic operators
inline PointGFp operator-(const PointGFp& lhs)
   {
   return PointGFp(lhs).negate();
   }

inline PointGFp operator+(const PointGFp& lhs, const PointGFp& rhs)
   {
   PointGFp tmp(lhs);
   return tmp += rhs;
   }

inline PointGFp operator-(const PointGFp& lhs, const PointGFp& rhs)
   {
   PointGFp tmp(lhs);
   return tmp -= rhs;
   }

inline PointGFp operator*(const PointGFp& point, const BigInt& scalar)
   {
   return scalar * point;
   }

// encoding and decoding
secure_vector<uint8_t> BOTAN_PUBLIC_API(2,0) EC2OSP(const PointGFp& point, uint8_t format);

PointGFp BOTAN_PUBLIC_API(2,0) OS2ECP(const uint8_t data[], size_t data_len,
                                      const CurveGFp& curve);

/**
* Perform point decoding
* @param data the encoded point
* @param data_len length of data in bytes
* @param curve_p the curve equation prime
* @param curve_a the curve equation a parameter
* @param curve_b the curve equation b parameter
*/
std::pair<BigInt, BigInt> BOTAN_PUBLIC_API(2,5) OS2ECP(const uint8_t data[], size_t data_len,
                                                       const BigInt& curve_p,
                                                       const BigInt& curve_a,
                                                       const BigInt& curve_b);

template<typename Alloc>
PointGFp OS2ECP(const std::vector<uint8_t, Alloc>& data, const CurveGFp& curve)
   { return OS2ECP(data.data(), data.size(), curve); }

/**
* Blinded ECC point multiplication
*/
class BOTAN_PUBLIC_API(2,5) PointGFp_Blinded_Multiplier final
   {
   public:
      /**
      * @param base_point the point that will be multiplied (eg, the base point of the curve)
      * @param w the window bits (leave as zero to take a default value)
      */
      PointGFp_Blinded_Multiplier(const PointGFp& base_point,
                                  size_t w = 0);

      /**
      * @param base_point the point that will be multiplied (eg, the base point of the curve)
      * @param ws a temporary workspace
      * @param w the window bits (leave as zero to take a default value)
      */
      PointGFp_Blinded_Multiplier(const PointGFp& base_point,
                                  std::vector<BigInt>& ws,
                                  size_t w = 0);

      /**
      * Randomize the internal state. Changing the values may provide
      * some protection against side channel attacks.
      * @param rng a random number generator
      */
      void randomize(RandomNumberGenerator& rng);

      /**
      * Perform blinded point multiplication
      * @param k the scalar
      * @param group_order the order of the group
      * @param rng a random number generator
      * @param ws a temporary workspace
      * @return base_point*k
      */
      PointGFp mul(const BigInt& k,
                   const BigInt& group_order,
                   RandomNumberGenerator& rng,
                   std::vector<BigInt>& ws) const;
   private:
      void init(const PointGFp& base_point, size_t w, std::vector<BigInt>& ws);

      std::vector<PointGFp> m_U;
      size_t m_h;
   };

class BOTAN_PUBLIC_API(2,0) BOTAN_DEPRECATED("Use PointGFp_Blinded_Multiplier") Blinded_Point_Multiply final
   {
   public:
      Blinded_Point_Multiply(const PointGFp& base, const BigInt& order, size_t h = 0) :
         m_ws(PointGFp::WORKSPACE_SIZE), m_order(order), m_point_mul(base, m_ws, h) {}

      PointGFp blinded_multiply(const BigInt& scalar, RandomNumberGenerator& rng)
         {
         m_point_mul.randomize(rng);
         return m_point_mul.mul(scalar, m_order, rng, m_ws);
         }
   private:
      std::vector<BigInt> m_ws;
      const BigInt& m_order;
      PointGFp_Blinded_Multiplier m_point_mul;
   };

}

namespace std {

template<>
inline void swap<Botan::PointGFp>(Botan::PointGFp& x, Botan::PointGFp& y)
   { x.swap(y); }

}

#endif
