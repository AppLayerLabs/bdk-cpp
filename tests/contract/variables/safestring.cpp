#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safestring.h"
#include <iostream>


namespace TSafeString {
  TEST_CASE("SafeString class", "[contract][variables][safestring]") {
    SECTION("SafeString constructor") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.revert();
      REQUIRE(safeString.get() == "");
      safeString = "Hello World";
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString assign") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.revert();
      REQUIRE(safeString.get() == "");
      safeString.assign("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString at") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.at(0) = 'h';
      REQUIRE(safeString.get() == "hello World");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString front") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.front() = 'h';
      REQUIRE(safeString.get() == "hello World");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString back") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.back() = 'D';
      REQUIRE(safeString.get() == "Hello WorlD");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString c_str()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      REQUIRE(safeString.c_str() == std::string("Hello World"));
      safeString.revert();
      REQUIRE(safeString.c_str() == std::string(""));
    }

    SECTION("SafeString data()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.data() == std::string("Hello World"));
      safeString.revert();
      REQUIRE(safeString.data() == std::string("Hello World"));
    }

    SECTION("SafeString being() end()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      for (auto it = safeString.begin(); it != safeString.end(); ++it) {
        *it = std::toupper(*it);
      }
      REQUIRE(safeString.get() == "HELLO WORLD");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString rbegin() rend()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      for (auto it = safeString.rbegin(); it != safeString.rend(); ++it) {
        *it = std::toupper(*it);
      }
      REQUIRE(safeString.get() == "HELLO WORLD");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString empty()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      REQUIRE(safeString.empty() == false);
      safeString.revert();
      REQUIRE(safeString.empty() == true);
    }

    SECTION("SafeString size()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      REQUIRE(safeString.size() == 11);
      safeString.revert();
      REQUIRE(safeString.size() == 0);
    }

    SECTION("SafeString length()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      REQUIRE(safeString.length() == 11);
      safeString.revert();
      REQUIRE(safeString.length() == 0);
    }

    SECTION("SafeString max_size()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      REQUIRE(safeString.max_size() == uint64_t(9223372036854775807));
      safeString.revert();
      REQUIRE(safeString.max_size() == uint64_t(9223372036854775807));
    }

    SECTION("SafeString reserve()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.shrink_to_fit();
      REQUIRE(safeString.capacity() >= 11);
      safeString.reserve(100);
      REQUIRE(safeString.capacity() >= 100);
      safeString.revert();
      REQUIRE(safeString.get() == "");
      REQUIRE(safeString.capacity() >= 0);
    }

    SECTION("SafeString capacity()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      REQUIRE(safeString.capacity() >= 11);
      safeString.revert();
      REQUIRE(safeString.get() == "");
      REQUIRE(safeString.capacity() >= 0);
    }

    SECTION("SafeString shrink_to_fit()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      REQUIRE(safeString.capacity() >= 11);
      safeString.reserve(100);
      REQUIRE(safeString.capacity() >= 100);
      safeString.shrink_to_fit();
      REQUIRE(safeString.capacity() >= 11);
      safeString.revert();
      REQUIRE(safeString.get() == "");
      REQUIRE(safeString.capacity() >= 0);
    }

    SECTION("SafeString clear()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.clear();
      REQUIRE(safeString.get() == "");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString insert()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.insert(0, "Goodbye ");
      REQUIRE(safeString.get() == "Goodbye Hello World");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString erase()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.erase(0, 5);
      REQUIRE(safeString.get() == " World");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString push_back()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.push_back('!');
      REQUIRE(safeString.get() == "Hello World!");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString pop_back()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.pop_back();
      REQUIRE(safeString.get() == "Hello Worl");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString append()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.append("!!!");
      REQUIRE(safeString.get() == "Hello World!!!");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString compare()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.compare("Hello World") == 0);
      REQUIRE(safeString.compare("Hello World!") != 0);
      safeString = "Hello World!";
      REQUIRE(safeString.compare("Hello World") != 0);
      REQUIRE(safeString.compare("Hello World!") == 0);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString starts_with()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.starts_with("Hello") == true);
      REQUIRE(safeString.starts_with("Hello World") == true);
      REQUIRE(safeString.starts_with("Hello World!") == false);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString ends_with()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.ends_with("World") == true);
      REQUIRE(safeString.ends_with("Hello World") == true);
      REQUIRE(safeString.ends_with("Hello World!") == false);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString replace()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.replace(6, 8, "OVERRIDE");
      REQUIRE(safeString.get() == "Hello OVERRIDE");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString substr()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();

      /// TODO: REQUIRE(SafeString==) doesn't work.
      bool REQUIRED = false;
      if (safeString.substr(6, 8) == "World") { REQUIRED = true; }
      REQUIRE(REQUIRED == true);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString copy()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      char buffer[100];
      safeString.copy(buffer, 11);
      SafeString bufferedString(buffer);
      REQUIRE(bufferedString == "Hello World");
      bufferedString.revert();
      REQUIRE(bufferedString.get() == "");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString resize()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      safeString.resize(5);
      REQUIRE(safeString.get() == "Hello");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString swap()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      SafeString safeString2("Goodbye World");
      REQUIRE(safeString2.get() == "Goodbye World");
      safeString2.commit();
      safeString.swap(safeString2);
      REQUIRE(safeString.get() == "Goodbye World");
      REQUIRE(safeString2.get() == "Hello World");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
      safeString2.revert();
      REQUIRE(safeString2.get() == "Goodbye World");
    }

    SECTION("SafeString find()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.find("Hello") == 0);
      REQUIRE(safeString.find("World") == 6);
      REQUIRE(safeString.find("Hello World") == 0);
      REQUIRE(safeString.find("Hello World!") == -1);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString rfind()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.rfind("Hello") == 0);
      REQUIRE(safeString.rfind("World") == 6);
      REQUIRE(safeString.rfind("Hello World") == 0);
      REQUIRE(safeString.rfind("Hello World!") == -1);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString find_first_of()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      std::string test = "Hello World";

      REQUIRE(safeString.find_first_of('l') == 2);
      REQUIRE(safeString.find_first_of('W') == 6);
      REQUIRE(safeString.find_first_of('d') == 10);
      REQUIRE(safeString.find_first_of('p') == -1);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString find_first_not_of()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.find_first_not_of("Hello") == 5);
      REQUIRE(safeString.find_first_not_of("Hello Wor") == 10);
      REQUIRE(safeString.find_first_not_of("Hell") == 4);

      REQUIRE(safeString.find_first_not_of("Hello World") == -1);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString find_last_of()") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.find_last_of('W') == 6);
      REQUIRE(safeString.find_last_of('l') == 9);
      REQUIRE(safeString.find_last_of('d') == 10);
      REQUIRE(safeString.find_last_of('p') == -1);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString find_last_not_of") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.find_last_not_of("Hello") == 10);
      REQUIRE(safeString.find_last_not_of("World") == 5);
      REQUIRE(safeString.find_last_not_of(" World") == 1);
      REQUIRE(safeString.find_last_not_of("Hello World") == -1);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString operator=") {
      SafeString safeString;
      REQUIRE(safeString.get() == "");
      safeString = "Hello World";
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString operator+=") {
      SafeString safeString;
      REQUIRE(safeString.get() == "");
      safeString += "Hello";
      REQUIRE(safeString.get() == "Hello");
      safeString += " World";
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString operator[]") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      REQUIRE(safeString[0] == 'H');
      REQUIRE(safeString[1] == 'e');
      REQUIRE(safeString[2] == 'l');
      REQUIRE(safeString[3] == 'l');
      REQUIRE(safeString[4] == 'o');
      REQUIRE(safeString[5] == ' ');
      REQUIRE(safeString[6] == 'W');
      REQUIRE(safeString[7] == 'o');
      REQUIRE(safeString[8] == 'r');
      REQUIRE(safeString[9] == 'l');
      REQUIRE(safeString[10] == 'd');
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
    }

    SECTION("SafeString operator+") {
      SafeString safeString("Hello");
      REQUIRE(safeString.get() == "Hello");
      safeString.commit();
      SafeString safeString2(" World");
      REQUIRE(safeString2.get() == " World");
      safeString2.commit();
      SafeString safeString3 = safeString + safeString2;
      REQUIRE(safeString3.get() == "Hello World");
      safeString.revert();
      REQUIRE(safeString.get() == "Hello");
      safeString2.revert();
      REQUIRE(safeString2.get() == " World");
    }

    SECTION("SafeString operator==") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      SafeString safeString2("Hello World");
      REQUIRE(safeString2.get() == "Hello World");
      safeString2.commit();
      REQUIRE(safeString == safeString2);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
      safeString2.revert();
      REQUIRE(safeString2.get() == "Hello World");
    }

    SECTION("SafeString operator!=") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      SafeString safeString2("Hello World!");
      REQUIRE(safeString2.get() == "Hello World!");
      safeString2.commit();
      REQUIRE(safeString != safeString2);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
      safeString2.revert();
      REQUIRE(safeString2.get() == "Hello World!");
    }

    SECTION("SafeString operator<") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      SafeString safeString2("Hello World!");
      REQUIRE(safeString2.get() == "Hello World!");
      safeString2.commit();
      REQUIRE(safeString < safeString2);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
      safeString2.revert();
      REQUIRE(safeString2.get() == "Hello World!");
    }

    SECTION("SafeString operator>") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      SafeString safeString2("Hello World!");
      REQUIRE(safeString2.get() == "Hello World!");
      safeString2.commit();
      REQUIRE(safeString2 > safeString);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
      safeString2.revert();
      REQUIRE(safeString2.get() == "Hello World!");
    }

    SECTION("SafeString operator<=") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      SafeString safeString2("Hello World!");
      REQUIRE(safeString2.get() == "Hello World!");
      safeString2.commit();
      REQUIRE(safeString <= safeString2);
      REQUIRE(safeString2 <= safeString2);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
      safeString2.revert();
      REQUIRE(safeString2.get() == "Hello World!");
    }

    SECTION("SafeString operator>=") {
      SafeString safeString("Hello World");
      REQUIRE(safeString.get() == "Hello World");
      safeString.commit();
      SafeString safeString2("Hello World!");
      REQUIRE(safeString2.get() == "Hello World!");
      safeString2.commit();
      REQUIRE(safeString2 >= safeString);
      REQUIRE(safeString2 >= safeString2);
      safeString.revert();
      REQUIRE(safeString.get() == "Hello World");
      safeString2.revert();
      REQUIRE(safeString2.get() == "Hello World!");
    }
  }
}