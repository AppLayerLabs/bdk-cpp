/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../blockchainwrapper.hpp" // blockchain.h -> consensus.h -> state.h -> (rdpos.h -> net/p2p/managernormal.h), dump.h -> storage.h, utils/db.h -> utils.h -> filesystem

const std::vector<Hash> validatorPrivKeysState {
  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

namespace TDumpManager {
  std::string testDumpPath = Utils::getTestDumpPath();
  TEST_CASE("DumpManager Class", "[dumpmanager]") {
    SECTION("DumpManager Test With DumpWorker") {
      Hash bestBlockHash = Hash();
      {
        auto blockchainWrapper = initialize(validatorPrivKeysState,
                               validatorPrivKeysState[0],
                               8080,
                               true,
                               testDumpPath + "/dumpManagerSimpleTests");
        // start the dump worker
        blockchainWrapper.state.dumpStartWorker();
        // create 1001 blocks
        for (uint64_t i = 0; i < 1001; ++i) {
          GLOGTRACE("Creating block: " + std::to_string(i));
          auto block = createValidBlock(validatorPrivKeysState,
                                        blockchainWrapper.state,
                                        blockchainWrapper.storage);
          REQUIRE(blockchainWrapper.state.tryProcessNextBlock(std::move(block)) == BlockValidationStatus::valid);
        }
        // stop the dump worker
        blockchainWrapper.state.dumpStopWorker();
        // Verify if the database was created
        REQUIRE(std::filesystem::exists(testDumpPath + "/dumpManagerSimpleTests"  + "/stateDb"));
        bestBlockHash = blockchainWrapper.storage.latest()->getHash();
      }
      auto blockchainWrapper = initialize(validatorPrivKeysState,
                             validatorPrivKeysState[0],
                             8080,
                             false,
                             testDumpPath + "/dumpManagerSimpleTests");


      REQUIRE(bestBlockHash == blockchainWrapper.storage.latest()->getHash());

    }
  }
}
