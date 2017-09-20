/*
* (C) 2015,2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_FFI_UTILS_H_
#define BOTAN_FFI_UTILS_H_

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <botan/exceptn.h>
#include <botan/mem_ops.h>

namespace Botan_FFI {

#define BOTAN_ASSERT_ARG_NON_NULL(p) \
   do { if(!p) throw Botan::Invalid_Argument("Argument " #p " is null"); } while(0)

class FFI_Error : public Botan::Exception
   {
   public:
      explicit FFI_Error(const std::string& what) : Exception("FFI error", what) {}
   };

template<typename T, uint32_t MAGIC>
struct botan_struct
   {
   public:
      botan_struct(T* obj) : m_magic(MAGIC), m_obj(obj) {}
      virtual ~botan_struct() { m_magic = 0; m_obj.reset(); }

      bool magic_ok() const { return (m_magic == MAGIC); }

      T* get() const
         {
         if(magic_ok() == false)
            throw FFI_Error("Bad magic " + std::to_string(m_magic) +
                            " in ffi object expected " + std::to_string(MAGIC));
         return m_obj.get();
         }
   private:
      uint32_t m_magic = 0;
      std::unique_ptr<T> m_obj;
   };

#define BOTAN_FFI_DECLARE_STRUCT(NAME, TYPE, MAGIC) \
   struct NAME : public botan_struct<TYPE, MAGIC> { explicit NAME(TYPE* x) : botan_struct(x) {} }

// Declared in ffi.cpp
int ffi_error_exception_thrown(const char* func_name, const char* exn);

template<typename T, uint32_t M>
T& safe_get(botan_struct<T,M>* p)
   {
   if(!p)
      throw FFI_Error("Null pointer argument");
   if(T* t = p->get())
      return *t;
   throw FFI_Error("Invalid object pointer");
   }

template<typename T, uint32_t M>
const T& safe_get(const botan_struct<T,M>* p)
   {
   if(!p)
      throw FFI_Error("Null pointer argument");
   if(const T* t = p->get())
      return *t;
   throw FFI_Error("Invalid object pointer");
   }

template<typename Thunk>
int ffi_guard_thunk(const char* func_name, Thunk thunk)
   {
   try
      {
      return thunk();
      }
   catch(std::bad_alloc)
      {
      return ffi_error_exception_thrown(func_name, "bad_alloc");
      }
   catch(std::exception& e)
      {
      return ffi_error_exception_thrown(func_name, e.what());
      }
   catch(...)
      {
      return ffi_error_exception_thrown(func_name, "unknown exception");
      }

   return BOTAN_FFI_ERROR_UNKNOWN_ERROR;
   }

template<typename T, uint32_t M, typename F>
int apply_fn(botan_struct<T, M>* o, const char* func_name, F func)
   {
   try
      {
      if(!o)
         throw FFI_Error("Null object to " + std::string(func_name));
      if(T* t = o->get())
         return func(*t);
      }
   catch(std::bad_alloc)
      {
      return ffi_error_exception_thrown(func_name, "bad_alloc");
      }
   catch(std::exception& e)
      {
      return ffi_error_exception_thrown(func_name, e.what());
      }
   catch(...)
      {
      return ffi_error_exception_thrown(func_name, "unknown exception");
      }

   return BOTAN_FFI_ERROR_UNKNOWN_ERROR;
   }

#define BOTAN_FFI_DO(T, obj, param, block)                              \
   apply_fn(obj, BOTAN_CURRENT_FUNCTION,                                \
            [=](T& param) -> int { do { block } while(0); return BOTAN_FFI_SUCCESS; })

template<typename T, uint32_t M>
int ffi_delete_object(botan_struct<T, M>* obj, const char* func_name)
   {
   try
      {
      if(obj == nullptr)
         return BOTAN_FFI_SUCCESS; // ignore delete of null objects

      if(obj->magic_ok() == false)
         return BOTAN_FFI_ERROR_INVALID_INPUT;

      delete obj;
      return BOTAN_FFI_SUCCESS;
      }
   catch(std::exception& e)
      {
      return ffi_error_exception_thrown(func_name, e.what());
      }
   catch(...)
      {
      return ffi_error_exception_thrown(func_name, "unknown exception");
      }
   }

#define BOTAN_FFI_CHECKED_DELETE(o) ffi_delete_object(o, BOTAN_CURRENT_FUNCTION)

inline int write_output(uint8_t out[], size_t* out_len, const uint8_t buf[], size_t buf_len)
   {
   const size_t avail = *out_len;
   *out_len = buf_len;

   if(avail >= buf_len)
      {
      Botan::copy_mem(out, buf, buf_len);
      return BOTAN_FFI_SUCCESS;
      }
   else
      {
      Botan::clear_mem(out, avail);
      return BOTAN_FFI_ERROR_INSUFFICIENT_BUFFER_SPACE;
      }
   }

template<typename Alloc>
int write_vec_output(uint8_t out[], size_t* out_len, const std::vector<uint8_t, Alloc>& buf)
   {
   return write_output(out, out_len, buf.data(), buf.size());
   }

inline int write_str_output(uint8_t out[], size_t* out_len, const std::string& str)
   {
   return write_output(out, out_len,
                       reinterpret_cast<const uint8_t*>(str.c_str()),
                       str.size() + 1);
   }

inline int write_str_output(char out[], size_t* out_len, const std::string& str)
   {
   return write_str_output(reinterpret_cast<uint8_t*>(out), out_len, str);
   }

inline int write_str_output(char out[], size_t* out_len, const std::vector<uint8_t>& str_vec)
   {
   return write_output(reinterpret_cast<uint8_t*>(out), out_len,
                       reinterpret_cast<const uint8_t*>(str_vec.data()),
                       str_vec.size());
   }

}

#endif
