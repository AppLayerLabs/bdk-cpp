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
      REQUIRE(reconstructedBlock.getValidatorSig() == Signature(Hex::toBytes("fe3140171d99dac0026928a2b94b3f5a3a3a80cf76feb8c0a1bda78448b0b11057312196cb0ef07049ed5b00ac531fe66e69ab39a6aea2c8104b9852e1d9934c01")));
      REQUIRE(reconstructedBlock.getPrevBlockHash() == Hash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6")));
      REQUIRE(reconstructedBlock.getBlockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(1678400201858));
      REQUIRE(reconstructedBlock.getNHeight() == uint64_t(92137812));
      REQUIRE(reconstructedBlock.getTxValidators().size() == 0);
      REQUIRE(reconstructedBlock.getTxs().size() == 0);
      REQUIRE(reconstructedBlock.getValidatorPubKey() == UPubKey(Hex::toBytes("04a44addae24f026d4589b7e5943e31970bbd70125c576385e4aedf82660adfcdab4a1aee59e707a297149b18e3bfff46d8fcdabb24c870f44dc5dfd0d7ce82310")));
      REQUIRE(reconstructedBlock.isFinalized() == true);

      // Compare created reconstructed block with created block.
      REQUIRE(reconstructedBlock.getValidatorSig() == newBlock.getValidatorSig());
      REQUIRE(reconstructedBlock.getPrevBlockHash() == newBlock.getPrevBlockHash());
      REQUIRE(reconstructedBlock.getBlockRandomness() == newBlock.getBlockRandomness());
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == newBlock.getValidatorMerkleRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == newBlock.getTxMerkleRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == newBlock.getTimestamp());
      REQUIRE(reconstructedBlock.getNHeight() == newBlock.getNHeight());
      REQUIRE(reconstructedBlock.getTxValidators() == newBlock.getTxValidators());
      REQUIRE(reconstructedBlock.getTxs() == newBlock.getTxs());
      REQUIRE(reconstructedBlock.getValidatorPubKey() == newBlock.getValidatorPubKey());
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
      REQUIRE(reconstructedBlock.getValidatorSig() == Signature(Hex::toBytes("86ea28b408ac437d3a06449278b696cc8c83b48a6cb421f19e21be6ed47970b516ac0c6d6c81c73c6efa41d622f08c818a3120683d3f8706f864b01d3435932701")));
      REQUIRE(reconstructedBlock.getPrevBlockHash() == Hash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be")));
      REQUIRE(reconstructedBlock.getBlockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Hash(Hex::toBytes("5d8c59743808c403ac95ca03937d51bd01661d8951c1af7fade03495475281a5")));
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(1678400843315));
      REQUIRE(reconstructedBlock.getNHeight() == uint64_t(100));
      REQUIRE(reconstructedBlock.getTxValidators().size() == 0);
      REQUIRE(reconstructedBlock.getTxs().size() == 10);
      REQUIRE(reconstructedBlock.getValidatorPubKey() == UPubKey(Hex::toBytes("041f29fad59ec4befa2691303e125188a54651c97adc6601757fd9d62e331e17c7563a144928b32c7b82ab1d12a82a9ca73c97b5a833ebeb903244d74dfdfa97fc")));
      REQUIRE(reconstructedBlock.isFinalized() == true);
      // Compare transactions with original transactions.
      for (uint64_t i = 0; i < 10; ++i) {
        REQUIRE(reconstructedBlock.getTxs()[i] == tx);
      }
      // Compare created reconstructed block with created block.
      REQUIRE(reconstructedBlock.getValidatorSig() == newBlock.getValidatorSig());
      REQUIRE(reconstructedBlock.getPrevBlockHash() == newBlock.getPrevBlockHash());
      REQUIRE(reconstructedBlock.getBlockRandomness() == newBlock.getBlockRandomness());
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == newBlock.getValidatorMerkleRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == newBlock.getTxMerkleRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == newBlock.getTimestamp());
      REQUIRE(reconstructedBlock.getNHeight() == newBlock.getNHeight());
      REQUIRE(reconstructedBlock.getTxValidators() == newBlock.getTxValidators());
      REQUIRE(reconstructedBlock.getTxs() == newBlock.getTxs());
      REQUIRE(reconstructedBlock.getValidatorPubKey() == newBlock.getValidatorPubKey());
      REQUIRE(reconstructedBlock.isFinalized() == newBlock.isFinalized());
    }
  }
}