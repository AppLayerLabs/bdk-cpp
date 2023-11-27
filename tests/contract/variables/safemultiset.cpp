/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safemultiset.h"
#include <iostream>
#include <iterator>

namespace TSafeMultiSet {
  TEST_CASE("SafeMultiSet Class", "[contract][variables][safemultiset]") {
    SECTION("SafeMultiSet Constructor (default)") {
      SafeMultiSet<std::string> safeMultiSetEmpty;
      REQUIRE(safeMultiSetEmpty.empty());
      REQUIRE(safeMultiSetEmpty.size() == 0);
    }

    SECTION("SafeMultiSet Constructor (initializer list)") {
      SafeMultiSet<std::string> safeMultiSetThree({"aaa", "bbb", "ccc"});
      SafeMultiSet<std::string> safeMultiSetThreeCommit({"aaa", "bbb", "ccc"});
      REQUIRE(safeMultiSetThree.contains("aaa"));
      REQUIRE(safeMultiSetThree.contains("bbb"));
      REQUIRE(safeMultiSetThree.contains("ccc"));
      REQUIRE(safeMultiSetThree.size() == 3);
      safeMultiSetThree.revert();
      REQUIRE(safeMultiSetThree.empty());
      safeMultiSetThreeCommit.commit();
      REQUIRE(safeMultiSetThreeCommit.contains("aaa"));
      REQUIRE(safeMultiSetThreeCommit.contains("bbb"));
      REQUIRE(safeMultiSetThreeCommit.contains("ccc"));
    }

    SECTION("SafeMultiSet Constructor (iterator first, iterator last)") {
      std::multiset<std::string> multisetThree({"test1", "test2", "test3"});
      SafeMultiSet<std::string> safeMultiSetThree(multisetThree.begin(), multisetThree.end());
      SafeMultiSet<std::string> safeMultiSetThreeCommit(multisetThree.begin(), multisetThree.end());
      REQUIRE(safeMultiSetThree.contains("test1"));
      REQUIRE(safeMultiSetThree.contains("test2"));
      REQUIRE(safeMultiSetThree.contains("test3"));
      REQUIRE(safeMultiSetThree.size() == 3);
      safeMultiSetThree.revert();
      REQUIRE(safeMultiSetThree.empty());
      safeMultiSetThreeCommit.commit();
      REQUIRE(safeMultiSetThreeCommit.contains("test1"));
      REQUIRE(safeMultiSetThreeCommit.contains("test2"));
      REQUIRE(safeMultiSetThreeCommit.contains("test3"));
      REQUIRE(safeMultiSetThreeCommit.size() == 3);
    }

    SECTION("SafeMultiSet Constructor(SafeMultiSet)") {
      SafeMultiSet<std::string> safeMultiSetThreeExample({"test1", "test2", "test3"});
      SafeMultiSet<std::string> safeMultiSetThree(safeMultiSetThreeExample);
      SafeMultiSet<std::string> safeMultiSetThreeCommit(safeMultiSetThreeExample);
      REQUIRE(safeMultiSetThree.contains("test1"));
      REQUIRE(safeMultiSetThree.contains("test2"));
      REQUIRE(safeMultiSetThree.contains("test3"));
      REQUIRE(safeMultiSetThree.size() == 3);
      safeMultiSetThree.revert();
      REQUIRE(safeMultiSetThree.empty());
      safeMultiSetThreeCommit.commit();
      REQUIRE(safeMultiSetThreeCommit.contains("test1"));
      REQUIRE(safeMultiSetThreeCommit.contains("test2"));
      REQUIRE(safeMultiSetThreeCommit.contains("test3"));
      REQUIRE(safeMultiSetThreeCommit.size() == 3);
    }

    SECTION("SafeMultiSet find") {
      SafeMultiSet<std::string> safeMultiSet({"f1", "f2", "f3"});
      REQUIRE(safeMultiSet.find("f0") == safeMultiSet.end());
      REQUIRE(safeMultiSet.find("f1") != safeMultiSet.end());
      REQUIRE(safeMultiSet.find("f2") != safeMultiSet.end());
      REQUIRE(safeMultiSet.find("f3") != safeMultiSet.end());
      REQUIRE(safeMultiSet.find("f4") == safeMultiSet.end());
    }

    SECTION("SafeMultiSet insert (all)") {
      SafeMultiSet<std::string> safeMultiSet;
      std::string moveStr1 = "ins2";
      std::string moveStr2 = "ins3";
      SafeMultiSet<std::string> safeMultiSetInsertIt({"ins4", "ins5", "ins6"});
      safeMultiSet.insert("ins1");
      safeMultiSet.insert(std::move(moveStr1));
      safeMultiSet.insert(safeMultiSet.begin(), "ins0");
      safeMultiSet.insert(safeMultiSet.end(), moveStr2);
      safeMultiSet.insert(safeMultiSetInsertIt.begin(), safeMultiSetInsertIt.end());
      safeMultiSet.insert({"ins7", "ins8", "ins9"});
      REQUIRE(safeMultiSet.contains("ins0"));
      REQUIRE(safeMultiSet.contains("ins1"));
      REQUIRE(safeMultiSet.contains("ins2"));
      REQUIRE(safeMultiSet.contains("ins3"));
      REQUIRE(safeMultiSet.contains("ins4"));
      REQUIRE(safeMultiSet.contains("ins5"));
      REQUIRE(safeMultiSet.contains("ins6"));
      REQUIRE(safeMultiSet.contains("ins7"));
      REQUIRE(safeMultiSet.contains("ins8"));
      REQUIRE(safeMultiSet.contains("ins9"));
    }

    SECTION("SafeMultiSet emplace (all)") {
      SafeMultiSet<std::string> safeMultiSet;
      safeMultiSet.emplace("emp3");
      safeMultiSet.emplace("emp4");
      safeMultiSet.emplace("emp5");
      safeMultiSet.emplace_hint(safeMultiSet.begin(), "emp2");
      safeMultiSet.emplace_hint(safeMultiSet.begin(), "emp1");
      safeMultiSet.emplace_hint(safeMultiSet.begin(), "emp0");
      REQUIRE(safeMultiSet.contains("emp0"));
      REQUIRE(safeMultiSet.contains("emp1"));
      REQUIRE(safeMultiSet.contains("emp2"));
      REQUIRE(safeMultiSet.contains("emp3"));
      REQUIRE(safeMultiSet.contains("emp4"));
      REQUIRE(safeMultiSet.contains("emp5"));
    }

    SECTION("SafeMultiSet erase (all)") {
      SafeMultiSet<std::string> safeMultiSet(
        {"del0", "del1", "del2", "del3", "del4", "del5", "del6", "del7", "del8", "del9"}
      );
      safeMultiSet.erase(safeMultiSet.begin());
      REQUIRE(!safeMultiSet.contains("del0"));
      safeMultiSet.erase(safeMultiSet.find("del2"), safeMultiSet.find("del9")); // 2 to 8
      REQUIRE(!safeMultiSet.contains("del2"));
      REQUIRE(!safeMultiSet.contains("del3"));
      REQUIRE(!safeMultiSet.contains("del4"));
      REQUIRE(!safeMultiSet.contains("del5"));
      REQUIRE(!safeMultiSet.contains("del6"));
      REQUIRE(!safeMultiSet.contains("del7"));
      REQUIRE(!safeMultiSet.contains("del8"));
      REQUIRE(safeMultiSet.erase("del1"));
      REQUIRE(safeMultiSet.erase("del9"));
      REQUIRE(safeMultiSet.empty());
    }

    SECTION("SafeMultiSet swap") {
      SafeMultiSet<std::string> safeMultiSet1({"swap1", "swap2", "swap3"});
      SafeMultiSet<std::string> safeMultiSet2({"swap4", "swap5", "swap6"});
      safeMultiSet1.swap(safeMultiSet2);
      REQUIRE(safeMultiSet1.contains("swap4"));
      REQUIRE(safeMultiSet1.contains("swap5"));
      REQUIRE(safeMultiSet1.contains("swap6"));
      REQUIRE(safeMultiSet2.contains("swap1"));
      REQUIRE(safeMultiSet2.contains("swap2"));
      REQUIRE(safeMultiSet2.contains("swap3"));
    }

    SECTION("SafeMultiSet extract (all)") {
      std::string extracted;
      SafeMultiSet<std::string> safeMultiSet({"ext1", "ext2", "ext3"});
      extracted = safeMultiSet.extract(safeMultiSet.begin()).value();
      REQUIRE((extracted == "ext1" && !safeMultiSet.contains("ext1")));
      extracted = safeMultiSet.extract("ext2").value();
      REQUIRE((extracted == "ext2" && !safeMultiSet.contains("ext2")));
      std::string moveExt = "ext3";
      extracted = safeMultiSet.extract(std::move(moveExt)).value();
      REQUIRE((extracted == "ext3" && !safeMultiSet.contains("ext3")));
      REQUIRE(safeMultiSet.empty());
    }

    SECTION("SafeMultiSet count") {
      SafeMultiSet<std::string> safeMultiSet1({"c1", "c1", "c2", "c2", "c2"});
      SafeMultiSet<std::string> safeMultiSet2({"c3", "c3", "c4", "c5"});
      SafeMultiSet<std::string> safeMultiSet3({"c6", "c6", "c6"});
      SafeMultiSet<std::string> safeMultiSet4({"c7", "c8"});
      SafeMultiSet<std::string> safeMultiSet5({"c9"});
      REQUIRE(safeMultiSet1.count("c1") == 2);
      REQUIRE(safeMultiSet1.count("c2") == 3);
      REQUIRE(safeMultiSet2.count("c3") == 2);
      REQUIRE(safeMultiSet2.count("c4") == 1);
      REQUIRE(safeMultiSet2.count("c5") == 1);
      REQUIRE(safeMultiSet3.count("c6") == 3);
      REQUIRE(safeMultiSet4.count("c7") == 1);
      REQUIRE(safeMultiSet4.count("c8") == 1);
      REQUIRE(safeMultiSet5.count("c9") == 1);
      REQUIRE(safeMultiSet5.count("c0") == 0);
    }

    SECTION("SafeMultiSet bounds") {
      SafeMultiSet<std::string> safeMultiSet({"b1", "b2", "b3", "b4", "b5"});
      auto it1 = safeMultiSet.lower_bound("b2");
      auto it2 = safeMultiSet.upper_bound("b4");
      auto it3 = safeMultiSet.lower_bound("b6");
      auto it4 = safeMultiSet.upper_bound("b5");
      REQUIRE(it1 != safeMultiSet.end());
      REQUIRE(it2 != safeMultiSet.end());
      REQUIRE(it3 == safeMultiSet.end());
      REQUIRE(it4 == safeMultiSet.end());
      REQUIRE(*it1 == "b2");
      REQUIRE(*it2 == "b5");
    }

    SECTION("SafeMultiSet erase_if") {
      SafeMultiSet<int> safeMultiSet({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
      safeMultiSet.erase_if([](int i){ return i % 2 == 0; });
      for (int i = 1; i <= 10; i++) {
        if (i % 2 == 0) {
          REQUIRE(!safeMultiSet.contains(i));
        } else {
          REQUIRE(safeMultiSet.contains(i));
        }
      }
    }
  }
}

