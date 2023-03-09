#include "../src/libs/catch2/catch_amalgamated.hpp"
#include "../src/utils/utils.h"
#include "../src/utils/tx.h"
#include "../src/utils/block.h"

using Catch::Matchers::Equals;

namespace TBlock {
  TEST_CASE("Block Class", "[utils]") {
    SECTION("Simple Block Constructor") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      auto nPrevBlockHash = Hash();
      uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
      uint64_t nHeight = 100;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
      TxBlock tx(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"));

      for (uint64_t i = 0; i < 10; ++i) {
        newBlock.appendTx(tx);
      }

      newBlock.finalize(validatorPrivKey);
      std::string serializedBlock = newBlock.serializeBlock();

      Block reconstructedBlock(serializedBlock);

      std::cout << reconstructedBlock.txs().size() << std::endl;
      std::cout << newBlock.txs().size() << std::endl;

      REQUIRE(reconstructedBlock == newBlock);
      REQUIRE(reconstructedBlock.txs() == newBlock.txs());
    }
  }
}