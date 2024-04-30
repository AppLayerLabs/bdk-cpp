/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/variables/safetuple.h"
#include <cstdint>
#include <iostream>
#include <utility>

namespace TSafeTuple {
  TEST_CASE("SafeTuple Class", "[contract][variables][safetuple]") {
    SECTION("SafeTuple default constructor") {
      SafeTuple<uint64_t, uint64_t> commitedTuple;
      SafeTuple<uint64_t, uint64_t> revertedTuple;
      REQUIRE(get<0>(commitedTuple) == 0);
      REQUIRE(get<1>(commitedTuple) == 0);
      REQUIRE(get<0>(revertedTuple) == 0);
      REQUIRE(get<1>(revertedTuple) == 0);
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 0);
      REQUIRE(get<1>(commitedTuple) == 0);
      REQUIRE(get<0>(revertedTuple) == 0);
      REQUIRE(get<1>(revertedTuple) == 0);
    }

    SECTION("SafeTuple copy constructor") {
      SafeTuple<uint64_t, std::string> commitedTuple;
      SafeTuple<uint64_t, std::string> revertedTuple;
      SafeTuple<uint64_t, std::string> commitedCopyTuple(commitedTuple);
      SafeTuple<uint64_t, std::string> revertedCopyTuple(revertedTuple);
      REQUIRE(get<0>(commitedCopyTuple) == 0);
      REQUIRE(get<1>(commitedCopyTuple) == "");
      REQUIRE(get<0>(revertedCopyTuple) == 0);
      REQUIRE(get<1>(revertedCopyTuple) == "");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedCopyTuple) == 0);
      REQUIRE(get<1>(commitedCopyTuple) == "");
      REQUIRE(get<0>(revertedCopyTuple) == 0);
      REQUIRE(get<1>(revertedCopyTuple) == "");
    }

    SECTION("SafeTuple move constructor") {
      SafeTuple<uint64_t, std::string> commitedTuple(std::make_tuple(1, "test"));
      SafeTuple<uint64_t, std::string> revertedTuple(std::make_tuple(2, "test2"));
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
    }

    SECTION("SafeTuple inicialization constructor") {
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
    }

    SECTION("SafeTuple pair constructor") {
      SafeTuple<uint64_t, std::string> commitedTuple(std::make_pair(1, "test"));
      SafeTuple<uint64_t, std::string> revertedTuple(std::make_pair(2, "test2"));
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
    }

    SECTION("SafeTuple operator= copy") {
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple;
      revertedTuple = commitedTuple;
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 1);
      REQUIRE(get<1>(revertedTuple) == "test");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 0);
      REQUIRE(get<1>(revertedTuple) == "");
    }

    SECTION("SafeTuple operator= move") {
      SafeTuple<uint64_t, std::string> commitedTuple;
      SafeTuple<uint64_t, std::string> revertedTuple;
      commitedTuple = std::make_tuple(1, "test");
      revertedTuple = std::make_tuple(2, "test2");
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 0);
      REQUIRE(get<1>(revertedTuple) == "");
    }

    SECTION("SafeTuple operator= implicit conversion") {
      SafeTuple<int, std::string> commitedTuple(1, "test");
      SafeTuple<int, std::string> revertedTuple(2, "test2");
      SafeTuple<long, std::string> commitedCopyTuple(100L, "test");
      commitedTuple = commitedCopyTuple;
      revertedTuple = commitedCopyTuple;
      REQUIRE(get<0>(commitedTuple) == 100L);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 100L);
      REQUIRE(get<1>(revertedTuple) == "test");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 100L);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
    }

    SECTION("SafeTuple operator= conversion/move") {
      SafeTuple<long, std::string> commitedTuple(1L, "test");
      SafeTuple<long, std::string> revertedTuple(2L, "test2");
      commitedTuple = std::make_tuple(100, "test");
      revertedTuple = std::make_tuple(200, "test2");
      REQUIRE(get<0>(commitedTuple) == 100);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 200);
      REQUIRE(get<1>(revertedTuple) == "test2");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 100);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2L);
      REQUIRE(get<1>(revertedTuple) == "test2");
    }

    SECTION("SafeTuple operator= pair") {
      std::pair<long, std::string> testPair(1L, "test");
      SafeTuple<long, std::string> commitedTuple;
      SafeTuple<long, std::string> revertedTuple;
      commitedTuple = testPair;
      revertedTuple = testPair;
      REQUIRE(get<0>(commitedTuple) == 1L);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 1L);
      REQUIRE(get<1>(revertedTuple) == "test");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 1L);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 0L);
      REQUIRE(get<1>(revertedTuple) == "");
    }

    SECTION("SafeTuple operator= pair/move") {
      SafeTuple<uint64_t, std::string> commitedTuple;
      SafeTuple<uint64_t, std::string> revertedTuple;
      commitedTuple = std::make_pair(1, "test");
      revertedTuple = std::make_pair(2, "test2");
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 0);
      REQUIRE(get<1>(revertedTuple) == "");
    }

    SECTION("SafeTuple swap"){
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      commitedTuple.swap(revertedTuple);
      REQUIRE(get<0>(commitedTuple) == 2);
      REQUIRE(get<1>(commitedTuple) == "test2");
      REQUIRE(get<0>(revertedTuple) == 1);
      REQUIRE(get<1>(revertedTuple) == "test");
      commitedTuple.commit();
      revertedTuple.revert();
      // When we swap the values, the original values of the variables are now the
      // result of the swap, so it's not possible to revert to the original value.
      // I'm doing like this because this is the idea of the swap.
      REQUIRE(get<0>(commitedTuple) == 2);
      REQUIRE(get<1>(commitedTuple) == "test2");
      REQUIRE(get<0>(revertedTuple) == 1);
      REQUIRE(get<1>(revertedTuple) == "test");
    }

    SECTION("SafeTuple non-member swap") {
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      swap(commitedTuple, revertedTuple);
      REQUIRE(get<0>(commitedTuple) == 2);
      REQUIRE(get<1>(commitedTuple) == "test2");
      REQUIRE(get<0>(revertedTuple) == 1);
      REQUIRE(get<1>(revertedTuple) == "test");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 2);
      REQUIRE(get<1>(commitedTuple) == "test2");
      REQUIRE(get<0>(revertedTuple) == 1);
      REQUIRE(get<1>(revertedTuple) == "test");
    }

    SECTION("SafeTuple get"){
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(get<0>(commitedTuple) == 1);
      REQUIRE(get<1>(commitedTuple) == "test");
      REQUIRE(get<0>(revertedTuple) == 2);
      REQUIRE(get<1>(revertedTuple) == "test2");
    }

    SECTION("SafeTuple operator=="){
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      SafeTuple<uint64_t, std::string> commitedCopyTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedCopyTuple(2, "test2");
      REQUIRE(commitedTuple == commitedCopyTuple);
      REQUIRE(revertedTuple == revertedCopyTuple);
      REQUIRE_FALSE(commitedTuple == revertedTuple);
      REQUIRE_FALSE(commitedCopyTuple == revertedCopyTuple);
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(commitedTuple == commitedCopyTuple);
      REQUIRE(revertedTuple == revertedCopyTuple);
      REQUIRE_FALSE(commitedTuple == revertedTuple);
      REQUIRE_FALSE(commitedCopyTuple == revertedCopyTuple);
    }

    SECTION("SafeTuple operator!="){
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      SafeTuple<uint64_t, std::string> commitedCopyTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedCopyTuple(2, "test2");
      REQUIRE_FALSE(commitedTuple != commitedCopyTuple);
      REQUIRE_FALSE(revertedTuple != revertedCopyTuple);
      REQUIRE(commitedTuple != revertedTuple);
      REQUIRE(commitedCopyTuple != revertedCopyTuple);
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE_FALSE(commitedTuple != commitedCopyTuple);
      REQUIRE_FALSE(revertedTuple != revertedCopyTuple);
      REQUIRE(commitedTuple != revertedTuple);
      REQUIRE(commitedCopyTuple != revertedCopyTuple);
    }

    SECTION("SafeTuple operator<"){
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      SafeTuple<uint64_t, std::string> commitedCopyTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedCopyTuple(2, "test2");
      REQUIRE(commitedTuple < revertedTuple);
      REQUIRE(commitedCopyTuple < revertedCopyTuple);
      REQUIRE_FALSE(commitedTuple < commitedCopyTuple);
      REQUIRE_FALSE(revertedTuple < revertedCopyTuple);
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(commitedTuple < revertedTuple);
      REQUIRE(commitedCopyTuple < revertedCopyTuple);
      REQUIRE_FALSE(commitedTuple < commitedCopyTuple);
      REQUIRE_FALSE(revertedTuple < revertedCopyTuple);
    }

    SECTION("SafeTuple operator<="){
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      SafeTuple<uint64_t, std::string> commitedCopyTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedCopyTuple(2, "test2");
      REQUIRE(commitedTuple <= revertedTuple);
      REQUIRE(commitedCopyTuple <= revertedCopyTuple);
      REQUIRE(commitedTuple <= commitedCopyTuple);
      REQUIRE(revertedTuple <= revertedCopyTuple);
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(commitedTuple <= revertedTuple);
      REQUIRE(commitedCopyTuple <= revertedCopyTuple);
      REQUIRE(commitedTuple <= commitedCopyTuple);
      REQUIRE(revertedTuple <= revertedCopyTuple);
    }

    SECTION("SafeTuple operator>"){
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      SafeTuple<uint64_t, std::string> commitedCopyTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedCopyTuple(2, "test2");
      REQUIRE(revertedTuple > commitedTuple);
      REQUIRE(revertedCopyTuple > commitedCopyTuple);
      REQUIRE_FALSE(commitedTuple > commitedCopyTuple);
      REQUIRE_FALSE(revertedTuple > revertedCopyTuple);
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(revertedTuple > commitedTuple);
      REQUIRE(revertedCopyTuple > commitedCopyTuple);
      REQUIRE_FALSE(commitedTuple > commitedCopyTuple);
      REQUIRE_FALSE(revertedTuple > revertedCopyTuple);
    }

    SECTION("SafeTuple operator>="){
      SafeTuple<uint64_t, std::string> commitedTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedTuple(2, "test2");
      SafeTuple<uint64_t, std::string> commitedCopyTuple(1, "test");
      SafeTuple<uint64_t, std::string> revertedCopyTuple(2, "test2");
      REQUIRE(revertedTuple >= commitedTuple);
      REQUIRE(revertedCopyTuple >= commitedCopyTuple);
      REQUIRE(commitedTuple >= commitedCopyTuple);
      REQUIRE(revertedTuple >= revertedCopyTuple);
      commitedTuple.commit();
      revertedTuple.revert();
      REQUIRE(revertedTuple >= commitedTuple);
      REQUIRE(revertedCopyTuple >= commitedCopyTuple);
      REQUIRE(commitedTuple >= commitedCopyTuple);
      REQUIRE(revertedTuple >= revertedCopyTuple);
    }
  }
}

