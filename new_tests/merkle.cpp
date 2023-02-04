#include <algorithm>

#include "../new_src/utils/merkle.h"
#include "../new_src/libs/catch2/catch_amalgamated.hpp"

namespace TMerkle {
  TEST_CASE("Sample")
  {
    std::vector<Hash> tree = {
      Hash::random(), //0
      Hash::random(), //1
      Hash::random(), //2
      Hash::random(), //3
      Hash::random(), //4
      Hash::random(), //5
//      Hash::random(), //6
    };

//    int pos = 0;
//    for(Hash &hash : tree)
//    {
//      std::cout << "Generated Hash[" << pos << "]:" << hash.hex().get() << std::endl;
//      pos++;
//    }

    SECTION("Checking Merkle stored data...")
    {
      Merkle merkle(tree);
      std::vector<Hash> leaves = merkle.getLeaves();
      for(Hash& leaf: leaves)
      {
        auto item = std::find_if(tree.begin(), tree.end(), [&leaf](Hash& tLeaf) {
          return leaf.get() == Utils::sha3(tLeaf.get()).get();
        });
        bool found = item != tree.end();
        REQUIRE(found);
      }
    }

    SECTION("Recoverying proof on index")
    {
      Merkle merkle(tree);
      std::cout << "Printing entire Merkle Tree..." << std::endl;
      std::string tabs = "   ";
      int level = 0;
      for(const std::vector<Hash>& layer : merkle.getTree() )
      {
        int pos = 0;
        std::cout << "Layer " << level << std::endl;
        for(const Hash& hash : layer)
        {
          std::cout << tabs << pos <<": " << hash.hex().get() << std::endl;
          pos ++;
        }
        tabs += "   ";
        level++;
      }

      std::cout << "Get proof for leaf 3" << std::endl;

      int profPos = 0;
      std::vector<Hash> proofOfThree = merkle.getProof(3);
      for(const Hash& hash : proofOfThree)
      {
        std::cout << profPos << ": " << hash.hex().get() << std::endl;
        profPos++;
      }

//      Hash root = merkle.getRoot();

      ///Checking the authencity of the given hash
      Hash queried = tree.at(3);
      std::cout << "Queried: " << Utils::sha3(queried.get()).hex().get() << std::endl;
      Hash latest;
      for(const Hash& hash : proofOfThree)
      {
//        std::cout << "[Latest size: " << latest.size() <<" | empty: " << latest.empty() << " | get "<< latest.get() <<"]" << std::endl;
        if(latest.hex().get() == "0000000000000000000000000000000000000000000000000000000000000000")
        {
          std::cout << "Latest = Utils::sha3(" << hash.hex().get() << " + " << Utils::sha3(queried.get()).hex().get() << ");" << std::endl;
          latest = Utils::sha3(hash.get() + Utils::sha3(queried.get()).get());
          std::cout << "Result = " << latest.hex().get() << std::endl;
        }
        else
        {
          std::cout << "Latest = Utils::sha3(" << hash.hex().get() << " + " << latest.hex().get() << ");" << std::endl;
          latest = Utils::sha3(hash.get() + latest.get());
          std::cout << "Result = " << latest.hex().get() << std::endl;
        }
      }

      REQUIRE(latest.hex().get() == merkle.getRoot().hex().get());
    }
    SECTION("Dummy section") { REQUIRE(true); }
  }
}