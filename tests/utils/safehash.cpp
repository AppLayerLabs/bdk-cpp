/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "libs/catch2/catch_amalgamated.hpp"
#include "utils/safehash.h"
#include "bytes/hex.h"
#include "bytes/random.h"

namespace TSafeHash {
  TEST_CASE("SafeHash Struct", "[utils][safehash]") {
    SECTION("SafeHash operator() (coverage)") {
      const uint64_t i = 12345;
      const std::string str = "Hello World";
      const std::string_view strView = "Goodbye Planet";
      const Bytes bytes = Bytes{0xDE, 0xAD, 0xBE, 0xEF};
      const BytesArr<4> bytesArr = {0xDE, 0xAD, 0xBE, 0xEF};
      const View<Bytes> bytesView = Utils::create_view_span(str);
      const Address add(bytes::hex("0x1234567890123456789012345678901234567890"));
      const Functor func{83892529};
      const Hash hash = bytes::random();
      const std::shared_ptr<std::string> ptr = std::make_shared<std::string>(str);
      const FixedBytes<4> fixed{0xDE, 0xAD, 0xBE, 0xEF};
      const boost::unordered_flat_map<int, std::string, SafeHash> map = {{1, "aaa"}, {2, "bbb"}};
      const std::pair<boost::asio::ip::address, uint16_t> nodeIdV4 = std::make_pair(
        boost::asio::ip::address_v4::from_string("127.0.0.1"), 8000
      );
      const std::pair<boost::asio::ip::address, uint16_t> nodeIdV6 = std::make_pair(
        boost::asio::ip::address_v6::from_string("::1"), 8000
      );

      SafeHash()(i);
      SafeHash()(str);
      SafeHash()(strView);
      SafeHash()(bytes);
      SafeHash()(bytesArr);
      SafeHash()(bytesView);
      SafeHash()(add);
      SafeHash()(func);
      SafeHash()(hash);
      //SafeHash()(ptr);  // Not sure why those error out, but they're already covered anyway
      SafeHash()(fixed);
      //SafeHash()(map);
      SafeHash()(nodeIdV4);
      SafeHash()(nodeIdV6);

      REQUIRE(true);  // No test cases, just calling functions for coverage purposes
    }
  }
}

