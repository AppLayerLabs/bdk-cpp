#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safevector.h"
#include <iostream>


namespace TSafeVector {
  TEST_CASE("SafeVector Class", "[contract][variables][safevector]") {
    SECTION("SafeVector Constructor (default)") {
      SafeVector<std::string> safeVectorEmpty;
      REQUIRE(safeVectorEmpty.empty());
      REQUIRE(safeVectorEmpty.size() == 0);
    }

    SECTION("SafeVector Constructor (count, value)") {
      SafeVector<std::string> safeVectorThree(3, "test");
      SafeVector<std::string> safeVectorThreeCommit(3, "test");
      REQUIRE(safeVectorThree[0] == "test");
      REQUIRE(safeVectorThree[1] == "test");
      REQUIRE(safeVectorThree[2] == "test");
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit[0] == "test");
      REQUIRE(safeVectorThreeCommit[1] == "test");
      REQUIRE(safeVectorThreeCommit[2] == "test");
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector Constructor (count)") {
      SafeVector<std::string> safeVectorThree(3);
      SafeVector<std::string> safeVectorThreeCommit(3);
      REQUIRE(safeVectorThree[0] == "");
      REQUIRE(safeVectorThree[1] == "");
      REQUIRE(safeVectorThree[2] == "");
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit[0] == "");
      REQUIRE(safeVectorThreeCommit[1] == "");
      REQUIRE(safeVectorThreeCommit[2] == "");
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector Constructor (initializer list)") {
      SafeVector<uint64_t> safeVectorThree({10, 20, 30});
      SafeVector<uint64_t> safeVectorThreeCommit({10, 20, 30});
      REQUIRE(safeVectorThree[0] == 10);
      REQUIRE(safeVectorThree[1] == 20);
      REQUIRE(safeVectorThree[2] == 30);
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit[0] == 10);
      REQUIRE(safeVectorThreeCommit[1] == 20);
      REQUIRE(safeVectorThreeCommit[2] == 30);
    }

    SECTION("SafeVector Constructor (iterator first, iterator last)") {
      std::vector<std::string> vectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThree(vectorThree.begin(), vectorThree.end());
      SafeVector<std::string> safeVectorThreeCommit(vectorThree.begin(), vectorThree.end());
      REQUIRE(safeVectorThree[0] == "test1");
      REQUIRE(safeVectorThree[1] == "test2");
      REQUIRE(safeVectorThree[2] == "test3");
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
      REQUIRE(safeVectorThreeCommit[2] == "test3");
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector Constructor(SafeVector)") {
      SafeVector<std::string> safeVectorThreeExample({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThree(safeVectorThreeExample);
      SafeVector<std::string> safeVectorThreeCommit(safeVectorThreeExample);
      REQUIRE(safeVectorThree[0] == "test1");
      REQUIRE(safeVectorThree[1] == "test2");
      REQUIRE(safeVectorThree[2] == "test3");
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
      REQUIRE(safeVectorThreeCommit[2] == "test3");
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector assign(count, value)") {
      SafeVector<std::string> safeVectorThree;
      SafeVector<std::string> safeVectorThreeCommit;
      safeVectorThree.assign(3, "test");
      safeVectorThreeCommit.assign(3, "test");
      REQUIRE(safeVectorThree[0] == "test");
      REQUIRE(safeVectorThree[1] == "test");
      REQUIRE(safeVectorThree[2] == "test");
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit[0] == "test");
      REQUIRE(safeVectorThreeCommit[1] == "test");
      REQUIRE(safeVectorThreeCommit[2] == "test");
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector assign(iterator first, iterator last)") {
      std::vector<std::string> vectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThree;
      SafeVector<std::string> safeVectorThreeCommit;
      safeVectorThree.assign(vectorThree.begin(), vectorThree.end());
      safeVectorThreeCommit.assign(vectorThree.begin(), vectorThree.end());
      REQUIRE(safeVectorThree[0] == "test1");
      REQUIRE(safeVectorThree[1] == "test2");
      REQUIRE(safeVectorThree[2] == "test3");
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
      REQUIRE(safeVectorThreeCommit[2] == "test3");
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector at()") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.at(0) == "test1");
      REQUIRE(safeVectorThree.at(1) == "test2");
      REQUIRE(safeVectorThree.at(2) == "test3");
      REQUIRE(safeVectorThree.size() == 3);
      REQUIRE_THROWS_AS(safeVectorThree.at(3), std::out_of_range);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.at(0) == "test1");
      REQUIRE(safeVectorThreeCommit.at(1) == "test2");
      REQUIRE(safeVectorThreeCommit.at(2) == "test3");
      REQUIRE(safeVectorThreeCommit.size() == 3);
      REQUIRE_THROWS_AS(safeVectorThreeCommit.at(3), std::out_of_range);
    }

    SECTION("SafeVector operator[]") {
      // A little redundant, but it's good to be thorough
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree[0] == "test1");
      REQUIRE(safeVectorThree[1] == "test2");
      REQUIRE(safeVectorThree[2] == "test3");
      REQUIRE(safeVectorThree.size() == 3);
      REQUIRE_THROWS_AS(safeVectorThree[3], std::out_of_range);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      REQUIRE_THROWS_AS(safeVectorThree[0], std::out_of_range);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
      REQUIRE(safeVectorThreeCommit[2] == "test3");
      REQUIRE(safeVectorThreeCommit.size() == 3);
      REQUIRE_THROWS_AS(safeVectorThreeCommit[3], std::out_of_range);
    }


    SECTION("SafeVector cbegin() and crbegin()") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      /// cbegin() ONLY RETURNS THE ORIGINAL ITERATOR
      /// YOU NEED TO COMMIT() TO GET THE DATA OR LOOP MANUALLY
      REQUIRE(safeVectorThree.cbegin() == safeVectorThree.cend());
      REQUIRE(safeVectorThree.crbegin() == safeVectorThree.crend());
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(*safeVectorThreeCommit.cbegin() == "test1");
      REQUIRE(*(safeVectorThreeCommit.cbegin() + 1) == "test2");
      REQUIRE(*(safeVectorThreeCommit.cbegin() + 2) == "test3");
      REQUIRE(*(safeVectorThreeCommit.crbegin() + 2) == "test1");
      REQUIRE(*(safeVectorThreeCommit.crbegin() + 1) == "test2");
      REQUIRE(*(safeVectorThreeCommit.crbegin()) == "test3");
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector cend() and crend()") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      /// cend() ONLY RETURNS THE ORIGINAL ITERATOR
      /// YOU NEED TO COMMIT() TO GET THE DATA OR LOOP MANUALLY
      REQUIRE(safeVectorThree.cbegin() == safeVectorThree.cend());
      REQUIRE(safeVectorThree.crbegin() == safeVectorThree.crend());
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      /// .cend() is the element after the last element.
      REQUIRE(*(safeVectorThreeCommit.cend() - 1) == "test3");
      REQUIRE(*(safeVectorThreeCommit.cend() - 2) == "test2");
      REQUIRE(*(safeVectorThreeCommit.cend() - 3) == "test1");
      REQUIRE(*(safeVectorThreeCommit.crend() - 3) == "test3");
      REQUIRE(*(safeVectorThreeCommit.crend() - 2) == "test2");
      REQUIRE(*(safeVectorThreeCommit.crend() - 1) == "test1");
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector iterator loop") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      int i = 0;
      for (auto it = safeVectorThreeCommit.cbegin(); it != safeVectorThreeCommit.cend(); ++it) {
        REQUIRE(*it == safeVectorThreeCommit[i]);
        ++i;
      }
      i = 0;
      for (auto it = safeVectorThreeCommit.crbegin(); it != safeVectorThreeCommit.crend(); ++it) {
        REQUIRE(*it == safeVectorThreeCommit[safeVectorThreeCommit.size() - 1 - i]);
        ++i;
      }
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector empty()") {
      // Also redundant, but good to be thorough
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE_FALSE(safeVectorThree.empty());
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE_FALSE(safeVectorThreeCommit.empty());
    }

    SECTION("SafeVector size()") {
      // Too redundant? hah
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.size() == 0);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector clear()") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.clear();
      REQUIRE(safeVectorThree.size() == 0);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.clear();
      REQUIRE(safeVectorThreeCommit.size() == 0);
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector insert()") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.insert(0, "test0");
      REQUIRE(safeVectorThree.size() == 4);
      REQUIRE(safeVectorThree[0] == "test0");
      REQUIRE(safeVectorThree[1] == "test1");
      REQUIRE(safeVectorThree[2] == "test2");
      REQUIRE(safeVectorThree[3] == "test3");
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.insert(0, "test0");
      REQUIRE(safeVectorThreeCommit.size() == 4);
      REQUIRE(safeVectorThreeCommit[0] == "test0");
      REQUIRE(safeVectorThreeCommit[1] == "test1");
      REQUIRE(safeVectorThreeCommit[2] == "test2");
      REQUIRE(safeVectorThreeCommit[3] == "test3");
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.insert(1, "test0");
      REQUIRE(safeVectorThreeCommit.size() == 4);
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test0");
      REQUIRE(safeVectorThreeCommit[2] == "test2");
      REQUIRE(safeVectorThreeCommit[3] == "test3");
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector erase(pos)") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.erase(0);
      REQUIRE(safeVectorThree.size() == 2);
      REQUIRE(safeVectorThree[0] == "test2");
      REQUIRE(safeVectorThree[1] == "test3");
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.erase(0);
      REQUIRE(safeVectorThreeCommit.size() == 2);
      REQUIRE(safeVectorThreeCommit[0] == "test2");
      REQUIRE(safeVectorThreeCommit[1] == "test3");
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.erase(1);
      REQUIRE(safeVectorThreeCommit.size() == 2);
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test3");
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
      REQUIRE(safeVectorThreeCommit[2] == "test3");
      safeVectorThreeCommit.erase(1);
      REQUIRE(safeVectorThreeCommit.size() == 2);
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test3");
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 2);
    }

    SECTION("SafeVector erase(first, last)") {
      SafeVector<std::string> safeVectorFive({"test1", "test2", "test3", "test4", "test5"});
      SafeVector<std::string> safeVectorFiveCommit({"test1", "test2", "test3", "test4", "test5"});
      REQUIRE(safeVectorFive.size() == 5);
      safeVectorFive.erase(0, 2);
      REQUIRE(safeVectorFive.size() == 3);
      REQUIRE(safeVectorFive[0] == "test3");
      REQUIRE(safeVectorFive[1] == "test4");
      REQUIRE(safeVectorFive[2] == "test5");
      safeVectorFive.revert();
      REQUIRE(safeVectorFive.size() == 0);
      safeVectorFiveCommit.erase(2, 4);
      safeVectorFiveCommit.commit();
      safeVectorFiveCommit.revert();
      REQUIRE(safeVectorFiveCommit.size() == 3);
      REQUIRE(safeVectorFiveCommit[0] == "test1");
      REQUIRE(safeVectorFiveCommit[1] == "test2");
      REQUIRE(safeVectorFiveCommit[2] == "test5");
      safeVectorFiveCommit.erase(0,3);
      REQUIRE(safeVectorFiveCommit.size() == 0);
      safeVectorFiveCommit.revert();
      REQUIRE(safeVectorFiveCommit.size() == 3);
    }

    SECTION("SafeVector push_back") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.push_back("test4");
      REQUIRE(safeVectorThree.size() == 4);
      REQUIRE(safeVectorThree[0] == "test1");
      REQUIRE(safeVectorThree[1] == "test2");
      REQUIRE(safeVectorThree[2] == "test3");
      REQUIRE(safeVectorThree[3] == "test4");
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.push_back("test4");
      REQUIRE(safeVectorThreeCommit.size() == 4);
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
      REQUIRE(safeVectorThreeCommit[2] == "test3");
      REQUIRE(safeVectorThreeCommit[3] == "test4");
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector emplace_back") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.emplace_back("test4");
      REQUIRE(safeVectorThree.size() == 4);
      REQUIRE(safeVectorThree[0] == "test1");
      REQUIRE(safeVectorThree[1] == "test2");
      REQUIRE(safeVectorThree[2] == "test3");
      REQUIRE(safeVectorThree[3] == "test4");
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.empty());
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.emplace_back("test4");
      REQUIRE(safeVectorThreeCommit.size() == 4);
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
      REQUIRE(safeVectorThreeCommit[2] == "test3");
      REQUIRE(safeVectorThreeCommit[3] == "test4");
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
    }

    SECTION("SafeVector pop_back()") {
      SafeVector<std::string> safeVectorThree({"test1", "test2", "test3"});
      SafeVector<std::string> safeVectorThreeCommit({"test1", "test2", "test3"});
      REQUIRE(safeVectorThree.size() == 3);
      safeVectorThree.pop_back();
      REQUIRE(safeVectorThree.size() == 2);
      REQUIRE(safeVectorThree[0] == "test1");
      REQUIRE(safeVectorThree[1] == "test2");
      safeVectorThree.revert();
      REQUIRE(safeVectorThree.size() == 0);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.pop_back();
      REQUIRE(safeVectorThreeCommit.size() == 2);
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 3);
      safeVectorThreeCommit.pop_back();
      REQUIRE(safeVectorThreeCommit.size() == 2);
      safeVectorThreeCommit.commit();
      safeVectorThreeCommit.revert();
      REQUIRE(safeVectorThreeCommit.size() == 2);
      REQUIRE(safeVectorThreeCommit[0] == "test1");
      REQUIRE(safeVectorThreeCommit[1] == "test2");
    }

    SECTION("SafeVector resize(count)") {
      SafeVector<std::string> safeVectorFiveLower({"test1", "test2", "test3", "test4", "test5"});
      SafeVector<std::string> safeVectorFiveHigher({"test1", "test2", "test3", "test4", "test5"});
      SafeVector<std::string> safeVectorFiveCommitLower({"test1", "test2", "test3", "test4", "test5"});
      SafeVector<std::string> safeVectorFiveCommitHigher({"test1", "test2", "test3", "test4", "test5"});
      REQUIRE(safeVectorFiveLower.size() == 5);
      safeVectorFiveLower.resize(3);
      REQUIRE(safeVectorFiveLower.size() == 3);
      REQUIRE(safeVectorFiveLower[0] == "test1");
      REQUIRE(safeVectorFiveLower[1] == "test2");
      REQUIRE(safeVectorFiveLower[2] == "test3");
      safeVectorFiveLower.revert();
      REQUIRE(safeVectorFiveLower.size() == 0);
      REQUIRE(safeVectorFiveHigher.size() == 5);
      safeVectorFiveHigher.resize(7);
      REQUIRE(safeVectorFiveHigher.size() == 7);
      REQUIRE(safeVectorFiveHigher[0] == "test1");
      REQUIRE(safeVectorFiveHigher[1] == "test2");
      REQUIRE(safeVectorFiveHigher[2] == "test3");
      REQUIRE(safeVectorFiveHigher[3] == "test4");
      REQUIRE(safeVectorFiveHigher[4] == "test5");
      REQUIRE(safeVectorFiveHigher[5] == "");
      REQUIRE(safeVectorFiveHigher[6] == "");
      safeVectorFiveHigher.revert();
      REQUIRE(safeVectorFiveHigher.size() == 0);
      REQUIRE(safeVectorFiveCommitLower.size() == 5);
      safeVectorFiveCommitLower.resize(3);
      safeVectorFiveCommitLower.commit();
      safeVectorFiveCommitLower.revert();
      REQUIRE(safeVectorFiveCommitLower.size() == 3);
      REQUIRE(safeVectorFiveCommitLower[0] == "test1");
      REQUIRE(safeVectorFiveCommitLower[1] == "test2");
      REQUIRE(safeVectorFiveCommitLower[2] == "test3");
      REQUIRE(safeVectorFiveCommitHigher.size() == 5);
      safeVectorFiveCommitHigher.resize(7);
      safeVectorFiveCommitHigher.commit();
      safeVectorFiveCommitHigher.revert();
      REQUIRE(safeVectorFiveCommitHigher.size() == 7);
      REQUIRE(safeVectorFiveCommitHigher[0] == "test1");
      REQUIRE(safeVectorFiveCommitHigher[1] == "test2");
      REQUIRE(safeVectorFiveCommitHigher[2] == "test3");
      REQUIRE(safeVectorFiveCommitHigher[3] == "test4");
      REQUIRE(safeVectorFiveCommitHigher[4] == "test5");
      REQUIRE(safeVectorFiveCommitHigher[5] == "");
      REQUIRE(safeVectorFiveCommitHigher[6] == "");
      safeVectorFiveCommitHigher.resize(3);
      safeVectorFiveCommitHigher.commit();
      safeVectorFiveCommitHigher.revert();
      REQUIRE(safeVectorFiveCommitHigher.size() == 3);
      REQUIRE(safeVectorFiveCommitHigher[0] == "test1");
      REQUIRE(safeVectorFiveCommitHigher[1] == "test2");
      REQUIRE(safeVectorFiveCommitHigher[2] == "test3");
      safeVectorFiveCommitHigher.resize(8);
      safeVectorFiveCommitHigher.commit();
      safeVectorFiveCommitHigher.revert();
      REQUIRE(safeVectorFiveCommitHigher[0] == "test1");
      REQUIRE(safeVectorFiveCommitHigher[1] == "test2");
      REQUIRE(safeVectorFiveCommitHigher[2] == "test3");
      REQUIRE(safeVectorFiveCommitHigher[3] == "");
      REQUIRE(safeVectorFiveCommitHigher[4] == "");
      REQUIRE(safeVectorFiveCommitHigher[5] == "");
      REQUIRE(safeVectorFiveCommitHigher[6] == "");
      REQUIRE(safeVectorFiveCommitHigher[7] == "");
    }

    SECTION("SafeVector resize(count, value)") {
      SafeVector<std::string> safeVectorFiveLower({"test1", "test2", "test3", "test4", "test5"});
      SafeVector<std::string> safeVectorFiveHigher({"test1", "test2", "test3", "test4", "test5"});
      SafeVector<std::string> safeVectorFiveCommitLower({"test1", "test2", "test3", "test4", "test5"});
      SafeVector<std::string> safeVectorFiveCommitHigher({"test1", "test2", "test3", "test4", "test5"});
      REQUIRE(safeVectorFiveLower.size() == 5);
      safeVectorFiveLower.resize(3, "TEST");
      REQUIRE(safeVectorFiveLower.size() == 3);
      REQUIRE(safeVectorFiveLower[0] == "test1");
      REQUIRE(safeVectorFiveLower[1] == "test2");
      REQUIRE(safeVectorFiveLower[2] == "test3");
      safeVectorFiveLower.revert();
      REQUIRE(safeVectorFiveLower.size() == 0);
      REQUIRE(safeVectorFiveHigher.size() == 5);
      safeVectorFiveHigher.resize(7, "TEST");
      REQUIRE(safeVectorFiveHigher.size() == 7);
      REQUIRE(safeVectorFiveHigher[0] == "test1");
      REQUIRE(safeVectorFiveHigher[1] == "test2");
      REQUIRE(safeVectorFiveHigher[2] == "test3");
      REQUIRE(safeVectorFiveHigher[3] == "test4");
      REQUIRE(safeVectorFiveHigher[4] == "test5");
      REQUIRE(safeVectorFiveHigher[5] == "TEST");
      REQUIRE(safeVectorFiveHigher[6] == "TEST");
      safeVectorFiveHigher.revert();
      REQUIRE(safeVectorFiveHigher.size() == 0);
      REQUIRE(safeVectorFiveCommitLower.size() == 5);
      safeVectorFiveCommitLower.resize(3, "TEST");
      safeVectorFiveCommitLower.commit();
      safeVectorFiveCommitLower.revert();
      REQUIRE(safeVectorFiveCommitLower.size() == 3);
      REQUIRE(safeVectorFiveCommitLower[0] == "test1");
      REQUIRE(safeVectorFiveCommitLower[1] == "test2");
      REQUIRE(safeVectorFiveCommitLower[2] == "test3");
      REQUIRE(safeVectorFiveCommitHigher.size() == 5);
      safeVectorFiveCommitHigher.resize(7, "TEST");
      safeVectorFiveCommitHigher.commit();
      safeVectorFiveCommitHigher.revert();
      REQUIRE(safeVectorFiveCommitHigher.size() == 7);
      REQUIRE(safeVectorFiveCommitHigher[0] == "test1");
      REQUIRE(safeVectorFiveCommitHigher[1] == "test2");
      REQUIRE(safeVectorFiveCommitHigher[2] == "test3");
      REQUIRE(safeVectorFiveCommitHigher[3] == "test4");
      REQUIRE(safeVectorFiveCommitHigher[4] == "test5");
      REQUIRE(safeVectorFiveCommitHigher[5] == "TEST");
      REQUIRE(safeVectorFiveCommitHigher[6] == "TEST");
      safeVectorFiveCommitHigher.resize(3, "TEST");
      safeVectorFiveCommitHigher.commit();
      safeVectorFiveCommitHigher.revert();
      REQUIRE(safeVectorFiveCommitHigher.size() == 3);
      REQUIRE(safeVectorFiveCommitHigher[0] == "test1");
      REQUIRE(safeVectorFiveCommitHigher[1] == "test2");
      REQUIRE(safeVectorFiveCommitHigher[2] == "test3");
      safeVectorFiveCommitHigher.resize(8, "TEST");
      REQUIRE(safeVectorFiveCommitHigher.size() == 8);
      REQUIRE(safeVectorFiveCommitHigher[0] == "test1");
      REQUIRE(safeVectorFiveCommitHigher[1] == "test2");
      REQUIRE(safeVectorFiveCommitHigher[2] == "test3");
      REQUIRE(safeVectorFiveCommitHigher[3] == "TEST");
      REQUIRE(safeVectorFiveCommitHigher[4] == "TEST");
      REQUIRE(safeVectorFiveCommitHigher[5] == "TEST");
      REQUIRE(safeVectorFiveCommitHigher[6] == "TEST");
      REQUIRE(safeVectorFiveCommitHigher[7] == "TEST");
      safeVectorFiveCommitHigher.revert();
      REQUIRE(safeVectorFiveCommitHigher.size() == 3);
    }
  }
}