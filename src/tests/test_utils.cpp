/*
* (C) 2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#include "tests.h"
#include <functional>
#include <botan/loadstor.h>
#include <botan/calendar.h>
#include <botan/internal/rounding.h>

#if defined(BOTAN_HAS_BASE64_CODEC)
  #include <botan/base64.h>
#endif

namespace Botan_Tests {

namespace {

class Utility_Function_Tests : public Text_Based_Test
   {
   public:
      Utility_Function_Tests() : Text_Based_Test("util.vec",
                                                 {"In1","In2","Out"})
         {}

      Test::Result run_one_test(const std::string& algo, const VarMap& vars) override
         {
         Test::Result result("Util " + algo);

         if(algo == "round_up")
            {
            const size_t x = get_req_sz(vars, "In1");
            const size_t to = get_req_sz(vars, "In2");

            result.test_eq(algo, Botan::round_up(x, to), get_req_sz(vars, "Out"));

            try
               {
               Botan::round_up(x, 0);
               result.test_failure("round_up did not reject invalid input");
               }
            catch(std::exception) {}
            }
         else if(algo == "round_down")
            {
            const size_t x = get_req_sz(vars, "In1");
            const size_t to = get_req_sz(vars, "In2");

            result.test_eq(algo, Botan::round_down<size_t>(x, to), get_req_sz(vars, "Out"));
            result.test_eq(algo, Botan::round_down<size_t>(x, 0), x);
            }

         return result;
         }

      std::vector<Test::Result> run_final_tests() override
         {
         std::vector<Test::Result> results;

         results.push_back(test_loadstore());

         return results;
         }

      Test::Result test_loadstore()
         {
         Test::Result result("Util load/store");

         const std::vector<uint8_t> membuf =
            Botan::hex_decode("00112233445566778899AABBCCDDEEFF");
         const uint8_t* mem = membuf.data();

         const uint16_t in16 = 0x1234;
         const uint32_t in32 = 0xA0B0C0D0;
         const uint64_t in64 = 0xABCDEF0123456789;

         result.test_is_eq<uint8_t>(Botan::get_byte(0, in32), 0xA0);
         result.test_is_eq<uint8_t>(Botan::get_byte(1, in32), 0xB0);
         result.test_is_eq<uint8_t>(Botan::get_byte(2, in32), 0xC0);
         result.test_is_eq<uint8_t>(Botan::get_byte(3, in32), 0xD0);

         result.test_is_eq<uint16_t>(Botan::make_u16bit(0xAA, 0xBB), 0xAABB);
         result.test_is_eq<uint32_t>(Botan::make_u32bit(0x01, 0x02, 0x03, 0x04), 0x01020304);

         result.test_is_eq<uint16_t>(Botan::load_be<uint16_t>(mem, 0), 0x0011);
         result.test_is_eq<uint16_t>(Botan::load_be<uint16_t>(mem, 1), 0x2233);
         result.test_is_eq<uint16_t>(Botan::load_be<uint16_t>(mem, 2), 0x4455);
         result.test_is_eq<uint16_t>(Botan::load_be<uint16_t>(mem, 3), 0x6677);

         result.test_is_eq<uint16_t>(Botan::load_le<uint16_t>(mem, 0), 0x1100);
         result.test_is_eq<uint16_t>(Botan::load_le<uint16_t>(mem, 1), 0x3322);
         result.test_is_eq<uint16_t>(Botan::load_le<uint16_t>(mem, 2), 0x5544);
         result.test_is_eq<uint16_t>(Botan::load_le<uint16_t>(mem, 3), 0x7766);

         result.test_is_eq<uint32_t>(Botan::load_be<uint32_t>(mem, 0), 0x00112233);
         result.test_is_eq<uint32_t>(Botan::load_be<uint32_t>(mem, 1), 0x44556677);
         result.test_is_eq<uint32_t>(Botan::load_be<uint32_t>(mem, 2), 0x8899AABB);
         result.test_is_eq<uint32_t>(Botan::load_be<uint32_t>(mem, 3), 0xCCDDEEFF);

         result.test_is_eq<uint32_t>(Botan::load_le<uint32_t>(mem, 0), 0x33221100);
         result.test_is_eq<uint32_t>(Botan::load_le<uint32_t>(mem, 1), 0x77665544);
         result.test_is_eq<uint32_t>(Botan::load_le<uint32_t>(mem, 2), 0xBBAA9988);
         result.test_is_eq<uint32_t>(Botan::load_le<uint32_t>(mem, 3), 0xFFEEDDCC);

         result.test_is_eq<uint64_t>(Botan::load_be<uint64_t>(mem, 0), 0x0011223344556677);
         result.test_is_eq<uint64_t>(Botan::load_be<uint64_t>(mem, 1), 0x8899AABBCCDDEEFF);

         result.test_is_eq<uint64_t>(Botan::load_le<uint64_t>(mem, 0), 0x7766554433221100);
         result.test_is_eq<uint64_t>(Botan::load_le<uint64_t>(mem, 1), 0xFFEEDDCCBBAA9988);

         // Check misaligned loads:
         result.test_is_eq<uint16_t>(Botan::load_be<uint16_t>(mem + 1, 0), 0x1122);
         result.test_is_eq<uint16_t>(Botan::load_le<uint16_t>(mem + 3, 0), 0x4433);

         result.test_is_eq<uint32_t>(Botan::load_be<uint32_t>(mem + 1, 1), 0x55667788);
         result.test_is_eq<uint32_t>(Botan::load_le<uint32_t>(mem + 3, 1), 0xAA998877);

         result.test_is_eq<uint64_t>(Botan::load_be<uint64_t>(mem + 1, 0), 0x1122334455667788);
         result.test_is_eq<uint64_t>(Botan::load_le<uint64_t>(mem + 7, 0), 0xEEDDCCBBAA998877);
         result.test_is_eq<uint64_t>(Botan::load_le<uint64_t>(mem + 5, 0), 0xCCBBAA9988776655);

         byte outbuf[16] = { 0 };

         for(size_t offset = 0; offset != 7; ++offset)
            {
            byte* out = outbuf + offset;

            Botan::store_be(in16, out);
            result.test_is_eq<uint8_t>(out[0], 0x12);
            result.test_is_eq<uint8_t>(out[1], 0x34);

            Botan::store_le(in16, out);
            result.test_is_eq<uint8_t>(out[0], 0x34);
            result.test_is_eq<uint8_t>(out[1], 0x12);

            Botan::store_be(in32, out);
            result.test_is_eq<uint8_t>(out[0], 0xA0);
            result.test_is_eq<uint8_t>(out[1], 0xB0);
            result.test_is_eq<uint8_t>(out[2], 0xC0);
            result.test_is_eq<uint8_t>(out[3], 0xD0);

            Botan::store_le(in32, out);
            result.test_is_eq<uint8_t>(out[0], 0xD0);
            result.test_is_eq<uint8_t>(out[1], 0xC0);
            result.test_is_eq<uint8_t>(out[2], 0xB0);
            result.test_is_eq<uint8_t>(out[3], 0xA0);

            Botan::store_be(in64, out);
            result.test_is_eq<uint8_t>(out[0], 0xAB);
            result.test_is_eq<uint8_t>(out[1], 0xCD);
            result.test_is_eq<uint8_t>(out[2], 0xEF);
            result.test_is_eq<uint8_t>(out[3], 0x01);
            result.test_is_eq<uint8_t>(out[4], 0x23);
            result.test_is_eq<uint8_t>(out[5], 0x45);
            result.test_is_eq<uint8_t>(out[6], 0x67);
            result.test_is_eq<uint8_t>(out[7], 0x89);

            Botan::store_le(in64, out);
            result.test_is_eq<uint8_t>(out[0], 0x89);
            result.test_is_eq<uint8_t>(out[1], 0x67);
            result.test_is_eq<uint8_t>(out[2], 0x45);
            result.test_is_eq<uint8_t>(out[3], 0x23);
            result.test_is_eq<uint8_t>(out[4], 0x01);
            result.test_is_eq<uint8_t>(out[5], 0xEF);
            result.test_is_eq<uint8_t>(out[6], 0xCD);
            result.test_is_eq<uint8_t>(out[7], 0xAB);
            }

         return result;
         }
   };

BOTAN_REGISTER_TEST("util", Utility_Function_Tests);

class Date_Format_Tests : public Text_Based_Test
   {
   public:
      Date_Format_Tests() : Text_Based_Test("dates.vec",
                                            std::vector<std::string>{"Date"})
         {}

      std::vector<uint32_t> parse_date(const std::string& s)
         {
         const std::vector<std::string> parts = Botan::split_on(s, ',');
         if(parts.size() != 6)
            throw Test_Error("Bad date format '" + s + "'");

         std::vector<uint32_t> u32s;
         for(auto&& sub : parts)
            {
            u32s.push_back(Botan::to_u32bit(sub));
            }
         return u32s;
         }

      Test::Result run_one_test(const std::string& type, const VarMap& vars) override
         {
         Test::Result result("Date parsing");

         const std::vector<uint32_t> d = parse_date(get_req_str(vars, "Date"));

         if(type == "valid" || type == "valid.not_std")
            {
            Botan::calendar_point c(d[0], d[1], d[2], d[3], d[4], d[5]);
            result.test_is_eq("year", c.year, d[0]);
            result.test_is_eq("month", c.month, d[1]);
            result.test_is_eq("day", c.day, d[2]);
            result.test_is_eq("hour", c.hour, d[3]);
            result.test_is_eq("minute", c.minutes, d[4]);
            result.test_is_eq("second", c.seconds, d[5]);

            if(type == "valid.not_std")
               {
               result.test_throws("valid but out of std::timepoint range", [c]() { c.to_std_timepoint(); });
               }
            else
               {
               Botan::calendar_point c2 = Botan::calendar_value(c.to_std_timepoint());
               result.test_is_eq("year", c2.year, d[0]);
               result.test_is_eq("month", c2.month, d[1]);
               result.test_is_eq("day", c2.day, d[2]);
               result.test_is_eq("hour", c2.hour, d[3]);
               result.test_is_eq("minute", c2.minutes, d[4]);
               result.test_is_eq("second", c2.seconds, d[5]);
               }
            }
         else if(type == "invalid")
            {
            result.test_throws("invalid date",
                               [d]() { Botan::calendar_point c(d[0], d[1], d[2], d[3], d[4], d[5]); });
            }
         else
            {
            throw Test_Error("Unexpected header '" + type + "' in date format tests");
            }

         return result;
         }

   };

BOTAN_REGISTER_TEST("util_dates", Date_Format_Tests);

#if defined(BOTAN_HAS_BASE64_CODEC)

class Base64_Tests : public Text_Based_Test
   {
   public:
      Base64_Tests() : Text_Based_Test("base64.vec",
                                       std::vector<std::string>({"Base64"}),
                                       {"Binary"})
         {}

      Test::Result run_one_test(const std::string& type, const VarMap& vars) override
         {
         Test::Result result("Base64");

         const bool is_valid = (type == "valid");
         const std::string base64 = get_req_str(vars, "Base64");

         try
            {
            if(is_valid)
               {
               const std::vector<byte> binary = get_req_bin(vars, "Binary");
               result.test_eq("base64 decoding", Botan::base64_decode(base64), binary);
               result.test_eq("base64 encoding", Botan::base64_encode(binary), base64);
               }
            else
               {
               auto res = Botan::base64_decode(base64);
               result.test_failure("decoded invalid base64 to " + Botan::hex_encode(res));
               }
            }
         catch(std::exception& e)
            {
            if(is_valid)
               {
               result.test_failure("rejected valid base64", e.what());
               }
            else
               {
               result.test_note("rejected invalid base64");
               }
            }

         return result;
         }

      std::vector<Test::Result> run_final_tests() override
         {
         Test::Result result("Base64");
         const std::string valid_b64 = "Zg==";

         for(char ws_char : { ' ', '\t', '\r', '\n' })
            {
            for(size_t i = 0; i <= valid_b64.size(); ++i)
               {
               std::string b64_ws = valid_b64;
               b64_ws.insert(i, 1, ws_char);

               try
                  {
                  result.test_failure("decoded whitespace base64", Botan::base64_decode(b64_ws, false));
                  }
               catch(std::exception&) {}

               try
                  {
                  result.test_eq("base64 decoding with whitespace", Botan::base64_decode(b64_ws, true), "66");
                  }
               catch(std::exception& e)
                  {
                  result.test_failure(b64_ws, e.what());
                  }
               }
            }

         return {result};
         }
   };

BOTAN_REGISTER_TEST("base64", Base64_Tests);

#endif

}

}
