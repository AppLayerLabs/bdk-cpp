#include <algorithm>

#include "../new_src/utils/merkle.h"
#include "../new_src/libs/catch2/catch_amalgamated.hpp"

using Catch::Matchers::Equals;

namespace TMerkle {
  TEST_CASE("Merkle Tests", "[utils]") {
    SECTION("Simple Merkle Tree Test") {
      std::vector<std::string> unhashedLeafs = {"ab", "bc", "cd", "de", "ef", "fg", "gh", "hi", "ij", "jk", "km", "mn"};
      std::vector<Hash> hashedLeafs;
      for(const std::string& leaf : unhashedLeafs)
      {
        hashedLeafs.emplace_back(Utils::sha3(leaf));
      }

      Merkle tree(hashedLeafs);

      auto proof = tree.getProof(3);
      auto leaf = tree.getLeaves()[3];
      auto root = tree.getRoot();
      auto badLeaf = tree.getLeaves()[4];

      REQUIRE_THAT(root.hex(), Equals("3fb0308018d8a6b4c2081699003624e9719774be2b7f65b7f9ac45f2bebc20b7"));
      REQUIRE(Merkle::verify(proof, leaf, root));
      REQUIRE(!Merkle::verify(proof, badLeaf, root));
    }

    SECTION("Random Merkle Tree Test") {
      std::vector<Hash> hashedLeafs {
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
      };

      Merkle tree(hashedLeafs);

      auto proof = tree.getProof(5);
      auto leaf = tree.getLeaves()[5];
      auto root = tree.getRoot();
      auto badLeaf = tree.getLeaves()[6];

      REQUIRE(Merkle::verify(proof, leaf, root));
      REQUIRE(!Merkle::verify(proof, badLeaf, root));
    }
  }
}