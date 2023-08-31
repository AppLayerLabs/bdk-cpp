/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safearray.h"
#include <iostream>


namespace TSafeArray {
  TEST_CASE("SafeArray Class", "[contract][variables][safearray]") {
    SECTION("SafeArray default constructor") {
      SafeArray<std::string, 5> array;
      REQUIRE(array.size() == 5);
      REQUIRE(array[0] == "");
      REQUIRE(array[1] == "");
      REQUIRE(array[2] == "");
      REQUIRE(array[3] == "");
      REQUIRE(array[4] == "");
    }

    SECTION("SafeArray Array Constructor") {
      SafeArray<std::string, 5> array ({"a", "b", "c", "d", "e"});
      SafeArray<std::string, 5> arrayCommit ({"a", "b", "c", "d", "e"});
      REQUIRE(array.size() == 5);
      REQUIRE(array[0] == "a");
      REQUIRE(array[1] == "b");
      REQUIRE(array[2] == "c");
      REQUIRE(array[3] == "d");
      REQUIRE(array[4] == "e");
      array.revert();
      REQUIRE(array.size() == 5);
      REQUIRE(array[0] == "");
      REQUIRE(array[1] == "");
      REQUIRE(array[2] == "");
      REQUIRE(array[3] == "");
      REQUIRE(array[4] == "");
      arrayCommit.commit();
      arrayCommit.revert();
      REQUIRE(arrayCommit.size() == 5);
      REQUIRE(arrayCommit[0] == "a");
      REQUIRE(arrayCommit[1] == "b");
      REQUIRE(arrayCommit[2] == "c");
      REQUIRE(arrayCommit[3] == "d");
      REQUIRE(arrayCommit[4] == "e");
    }

    SECTION("SafeArray at(pos)") {
      SafeArray<std::string, 5> array ({"a", "b", "c", "d", "e"});
      SafeArray<std::string, 5> arrayCommit ({"a", "b", "c", "d", "e"});
      REQUIRE(array.at(0) == "a");
      REQUIRE(array.at(1) == "b");
      REQUIRE(array.at(2) == "c");
      REQUIRE(array.at(3) == "d");
      REQUIRE(array.at(4) == "e");
      REQUIRE_THROWS(array.at(5));
      array.revert();
      REQUIRE(array.at(0) == "");
      REQUIRE(array.at(1) == "");
      REQUIRE(array.at(2) == "");
      REQUIRE(array.at(3) == "");
      REQUIRE(array.at(4) == "");
      REQUIRE_THROWS(array.at(5));
      arrayCommit.commit();
      arrayCommit.revert();
      REQUIRE(arrayCommit.at(0) == "a");
      REQUIRE(arrayCommit.at(1) == "b");
      REQUIRE(arrayCommit.at(2) == "c");
      REQUIRE(arrayCommit.at(3) == "d");
      REQUIRE(arrayCommit.at(4) == "e");
      REQUIRE_THROWS(arrayCommit.at(5));
    }

    SECTION("SafeArray operator[pos]") {
      SafeArray<std::string, 5> array ({"a", "b", "c", "d", "e"});
      SafeArray<std::string, 5> arrayCommit ({"a", "b", "c", "d", "e"});
      array[3] = "AAAA";
      REQUIRE(array.size() == 5);
      REQUIRE(array[0] == "a");
      REQUIRE(array[1] == "b");
      REQUIRE(array[2] == "c");
      REQUIRE(array[3] == "AAAA");
      REQUIRE(array[4] == "e");
      array.revert();
      REQUIRE(array[0] == "");
      REQUIRE(array[1] == "");
      REQUIRE(array[2] == "");
      REQUIRE(array[3] == "");
      REQUIRE(array[4] == "");
      arrayCommit.commit();
      arrayCommit.revert();
      REQUIRE(arrayCommit.size() == 5);
      arrayCommit[3] = "AAAA";
      REQUIRE(arrayCommit[0] == "a");
      REQUIRE(arrayCommit[1] == "b");
      REQUIRE(arrayCommit[2] == "c");
      REQUIRE(arrayCommit[3] == "AAAA");
      REQUIRE(arrayCommit[4] == "e");
      arrayCommit.revert();
      REQUIRE(arrayCommit[3] == "d");
    }

    SECTION("SafeArray cbegin() and crbegin()") {
      SafeArray<std::string, 3> safeArrayThree({"test1", "test2", "test3"});
      SafeArray<std::string, 3> safeArrayThreeCommit({"test1", "test2", "test3"});
      /// cbegin() ONLY RETURNS THE ORIGINAL ITERATOR
      /// YOU NEED TO COMMIT() TO GET THE DATA OR LOOP MANUALLY
      REQUIRE(*safeArrayThree.cbegin() == "");
      REQUIRE(*safeArrayThree.crbegin() == "");
      REQUIRE(safeArrayThree.size() == 3);
      safeArrayThree.revert();
      safeArrayThreeCommit.commit();
      safeArrayThreeCommit.revert();
      REQUIRE(*safeArrayThreeCommit.cbegin() == "test1");
      REQUIRE(*(safeArrayThreeCommit.cbegin() + 1) == "test2");
      REQUIRE(*(safeArrayThreeCommit.cbegin() + 2) == "test3");
      REQUIRE(*(safeArrayThreeCommit.crbegin() + 2) == "test1");
      REQUIRE(*(safeArrayThreeCommit.crbegin() + 1) == "test2");
      REQUIRE(*(safeArrayThreeCommit.crbegin()) == "test3");
      REQUIRE(safeArrayThreeCommit.size() == 3);
    }

    SECTION("SafeArray cend() and crend()") {
      SafeArray<std::string, 3> safeArrayThree({"test1", "test2", "test3"});
      SafeArray<std::string, 3> safeArrayThreeCommit({"test1", "test2", "test3"});
      /// cend() ONLY RETURNS THE ORIGINAL ITERATOR
      /// YOU NEED TO COMMIT() TO GET THE DATA OR LOOP MANUALLY
      REQUIRE(*safeArrayThree.cbegin() == "");
      REQUIRE(*safeArrayThree.crbegin() == "");
      REQUIRE(safeArrayThree.size() == 3);
      safeArrayThree.revert();
      safeArrayThreeCommit.commit();
      safeArrayThreeCommit.revert();
      /// .cend() is the element after the last element.
      REQUIRE(*(safeArrayThreeCommit.cend() - 1) == "test3");
      REQUIRE(*(safeArrayThreeCommit.cend() - 2) == "test2");
      REQUIRE(*(safeArrayThreeCommit.cend() - 3) == "test1");
      REQUIRE(*(safeArrayThreeCommit.crend() - 3) == "test3");
      REQUIRE(*(safeArrayThreeCommit.crend() - 2) == "test2");
      REQUIRE(*(safeArrayThreeCommit.crend() - 1) == "test1");
      REQUIRE(safeArrayThreeCommit.size() == 3);
    }

    SECTION("SafeArray iterator loop") {
      SafeArray<std::string, 3> safeArrayThree({"test1", "test2", "test3"});
      SafeArray<std::string, 3> safeArrayThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeArrayThree.size() == 3);
      safeArrayThree.revert();
      for (auto it = safeArrayThree.cbegin(); it != safeArrayThree.cend(); ++it) {
        REQUIRE(*it == "");
      }
      for (auto it = safeArrayThree.crbegin(); it != safeArrayThree.crend(); ++it) {
        REQUIRE(*it == "");
      }
      safeArrayThreeCommit.commit();
      safeArrayThreeCommit.revert();
      int i = 0;
      for (auto it = safeArrayThreeCommit.cbegin(); it != safeArrayThreeCommit.cend(); ++it) {
        REQUIRE(*it == safeArrayThreeCommit[i]);
        ++i;
      }
      i = 0;
      for (auto it = safeArrayThreeCommit.crbegin(); it != safeArrayThreeCommit.crend(); ++it) {
        REQUIRE(*it == safeArrayThreeCommit[safeArrayThreeCommit.size() - 1 - i]);
        ++i;
      }
      REQUIRE(safeArrayThreeCommit.size() == 3);
    }

    SECTION("SafeArray empty()") {
      SafeArray<std::string, 0> emptyArray;
      SafeArray<std::string, 3> notEmptyArray;
      REQUIRE(emptyArray.empty() == true);
      REQUIRE(notEmptyArray.empty() == false);
    }

    SECTION("SafeArray size()") {
      SafeArray<std::string, 0> sizeZeroArray;
      SafeArray<std::string, 3> sizeThreeArray;
      SafeArray<std::string, 5> sizeFiveArray;
      REQUIRE(sizeZeroArray.size() == 0);
      REQUIRE(sizeThreeArray.size() == 3);
      REQUIRE(sizeFiveArray.size() == 5);
    }

    SECTION("SafeArray max_size()") {
      SafeArray<std::string, 0> sizeZeroArray;
      SafeArray<std::string, 3> sizeThreeArray;
      SafeArray<std::string, 5> sizeFiveArray;
      REQUIRE(sizeZeroArray.max_size() == 0);
      REQUIRE(sizeThreeArray.max_size() == 3);
      REQUIRE(sizeFiveArray.max_size() == 5);
    }

    SECTION("SafeArray fill()") {
      SafeArray<std::string, 5> array;
      SafeArray<std::string, 5> arrayCommit;
      array.fill("test");
      REQUIRE(array[0] == "test");
      REQUIRE(array[1] == "test");
      REQUIRE(array[2] == "test");
      REQUIRE(array[3] == "test");
      REQUIRE(array[4] == "test");
      array.revert();
      REQUIRE(array[0] == "");
      REQUIRE(array[1] == "");
      REQUIRE(array[2] == "");
      REQUIRE(array[3] == "");
      REQUIRE(array[4] == "");
      arrayCommit.fill("test");
      arrayCommit.commit();
      arrayCommit.revert();
      REQUIRE(arrayCommit[0] == "test");
      REQUIRE(arrayCommit[1] == "test");
      REQUIRE(arrayCommit[2] == "test");
      REQUIRE(arrayCommit[3] == "test");
      REQUIRE(arrayCommit[4] == "test");
      arrayCommit.fill("test2");
      REQUIRE(arrayCommit[0] == "test2");
      REQUIRE(arrayCommit[1] == "test2");
      REQUIRE(arrayCommit[2] == "test2");
      REQUIRE(arrayCommit[3] == "test2");
      REQUIRE(arrayCommit[4] == "test2");
      arrayCommit.revert();
      REQUIRE(arrayCommit[0] == "test");
      REQUIRE(arrayCommit[1] == "test");
      REQUIRE(arrayCommit[2] == "test");
      REQUIRE(arrayCommit[3] == "test");
      REQUIRE(arrayCommit[4] == "test");
    }
  }
}