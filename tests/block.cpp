#include "../src/libs/catch2/catch_amalgamated.hpp"
#include "../src/utils/utils.h"
#include "../src/utils/tx.h"
#include "../src/utils/block.h"

using Catch::Matchers::Equals;

namespace TBlock {
  TEST_CASE("Block Class", "[utils]") {
    SECTION("Block creation with no transactions") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6"));
      uint64_t timestamp = 1678400201858;
      uint64_t nHeight = 92137812;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      newBlock.finalize(validatorPrivKey);

      Block reconstructedBlock(newBlock.serializeBlock());

      // Check within reconstructed block.
      REQUIRE(reconstructedBlock.validatorSig() == Signature(Hex::toBytes("fe3140171d99dac0026928a2b94b3f5a3a3a80cf76feb8c0a1bda78448b0b11057312196cb0ef07049ed5b00ac531fe66e69ab39a6aea2c8104b9852e1d9934c01")));
      REQUIRE(reconstructedBlock.prevBlockHash() == Hash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6")));
      REQUIRE(reconstructedBlock.blockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.validatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.txMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.timestamp() == uint64_t(1678400201858));
      REQUIRE(reconstructedBlock.nHeight() == uint64_t(92137812));
      REQUIRE(reconstructedBlock.txValidators().size() == 0);
      REQUIRE(reconstructedBlock.txs().size() == 0);
      REQUIRE(reconstructedBlock.validatorPubKey() == UPubKey(Hex::toBytes("04a44addae24f026d4589b7e5943e31970bbd70125c576385e4aedf82660adfcdab4a1aee59e707a297149b18e3bfff46d8fcdabb24c870f44dc5dfd0d7ce82310")));
      REQUIRE(reconstructedBlock.isFinalized() == true);

      // Compare created reconstructed block with created block.
      REQUIRE(reconstructedBlock.validatorSig() == newBlock.validatorSig());
      REQUIRE(reconstructedBlock.prevBlockHash() == newBlock.prevBlockHash());
      REQUIRE(reconstructedBlock.blockRandomness() == newBlock.blockRandomness());
      REQUIRE(reconstructedBlock.validatorMerkleRoot() == newBlock.validatorMerkleRoot());
      REQUIRE(reconstructedBlock.txMerkleRoot() == newBlock.txMerkleRoot());
      REQUIRE(reconstructedBlock.timestamp() == newBlock.timestamp());
      REQUIRE(reconstructedBlock.nHeight() == newBlock.nHeight());
      REQUIRE(reconstructedBlock.txValidators() == newBlock.txValidators());
      REQUIRE(reconstructedBlock.txs() == newBlock.txs());
      REQUIRE(reconstructedBlock.validatorPubKey() == newBlock.validatorPubKey());
      REQUIRE(reconstructedBlock.isFinalized() == newBlock.isFinalized());
    }
    
    SECTION("Block creation with 10 transactions") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be"));
      uint64_t timestamp = 1678400843315;
      uint64_t nHeight = 100;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
      TxBlock tx(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"));

      for (uint64_t i = 0; i < 10; ++i) {
        newBlock.appendTx(tx);
      }

      newBlock.finalize(validatorPrivKey);

      Block reconstructedBlock(newBlock.serializeBlock());

      // Check within reconstructed block.
      REQUIRE(reconstructedBlock.validatorSig() == Signature(Hex::toBytes("86ea28b408ac437d3a06449278b696cc8c83b48a6cb421f19e21be6ed47970b516ac0c6d6c81c73c6efa41d622f08c818a3120683d3f8706f864b01d3435932701")));
      REQUIRE(reconstructedBlock.prevBlockHash() == Hash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be")));
      REQUIRE(reconstructedBlock.blockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.validatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.txMerkleRoot() == Hash(Hex::toBytes("5d8c59743808c403ac95ca03937d51bd01661d8951c1af7fade03495475281a5")));
      REQUIRE(reconstructedBlock.timestamp() == uint64_t(1678400843315));
      REQUIRE(reconstructedBlock.nHeight() == uint64_t(100));
      REQUIRE(reconstructedBlock.txValidators().size() == 0);
      REQUIRE(reconstructedBlock.txs().size() == 10);
      REQUIRE(reconstructedBlock.validatorPubKey() == UPubKey(Hex::toBytes("041f29fad59ec4befa2691303e125188a54651c97adc6601757fd9d62e331e17c7563a144928b32c7b82ab1d12a82a9ca73c97b5a833ebeb903244d74dfdfa97fc")));
      REQUIRE(reconstructedBlock.isFinalized() == true);
      // Compare transactions with original transactions.
      for (uint64_t i = 0; i < 10; ++i) {
        REQUIRE(reconstructedBlock.txs()[i] == tx);
      }
      // Compare created reconstructed block with created block.
      REQUIRE(reconstructedBlock.validatorSig() == newBlock.validatorSig());
      REQUIRE(reconstructedBlock.prevBlockHash() == newBlock.prevBlockHash());
      REQUIRE(reconstructedBlock.blockRandomness() == newBlock.blockRandomness());
      REQUIRE(reconstructedBlock.validatorMerkleRoot() == newBlock.validatorMerkleRoot());
      REQUIRE(reconstructedBlock.txMerkleRoot() == newBlock.txMerkleRoot());
      REQUIRE(reconstructedBlock.timestamp() == newBlock.timestamp());
      REQUIRE(reconstructedBlock.nHeight() == newBlock.nHeight());
      REQUIRE(reconstructedBlock.txValidators() == newBlock.txValidators());
      REQUIRE(reconstructedBlock.txs() == newBlock.txs());
      REQUIRE(reconstructedBlock.validatorPubKey() == newBlock.validatorPubKey());
      REQUIRE(reconstructedBlock.isFinalized() == newBlock.isFinalized());
    }
  }
}