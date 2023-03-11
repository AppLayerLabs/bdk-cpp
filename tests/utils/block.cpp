#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/tx.h"
#include "../../src/utils/block.h"

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

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock());

      // Check within reconstructed block

      REQUIRE(reconstructedBlock.getValidatorSig() == Signature(Hex::toBytes("a65b66624ee7419b9ea21a1cb52df606b69f4992427e5ccbef90aadeef287f8461351d1bb99cc351f19c959706d47b7f923883e20f955cbe7c5eebbe2234e54100")));
      REQUIRE(reconstructedBlock.getPrevBlockHash() == Hash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6")));
      REQUIRE(reconstructedBlock.getBlockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(1678400201858));
      REQUIRE(reconstructedBlock.getNHeight() == uint64_t(92137812));
      REQUIRE(reconstructedBlock.getTxValidators().size() == 0);
      REQUIRE(reconstructedBlock.getTxs().size() == 0);
      REQUIRE(reconstructedBlock.getValidatorPubKey() == UPubKey(Hex::toBytes("046ab1f056c30ae181f92e97d0cbb73f4a8778e926c35f10f0c4d1626d8dfd51672366413809a48589aa103e1865e08bd6ddfd0559e095841eb1bd3021d9cc5e62")));
      REQUIRE(reconstructedBlock.isFinalized() == true);

      // Compare created reconstructed block with created block
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

      // Compare created reconstructed block with block copy constructor
      REQUIRE(reconstructedBlock.getValidatorSig() == blockCopyConstructor.getValidatorSig());
      REQUIRE(reconstructedBlock.getPrevBlockHash() == blockCopyConstructor.getPrevBlockHash());
      REQUIRE(reconstructedBlock.getBlockRandomness() == blockCopyConstructor.getBlockRandomness());
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == blockCopyConstructor.getValidatorMerkleRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == blockCopyConstructor.getTxMerkleRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == blockCopyConstructor.getTimestamp());
      REQUIRE(reconstructedBlock.getNHeight() == blockCopyConstructor.getNHeight());
      REQUIRE(reconstructedBlock.getTxValidators() == blockCopyConstructor.getTxValidators());
      REQUIRE(reconstructedBlock.getTxs() == blockCopyConstructor.getTxs());
      REQUIRE(reconstructedBlock.getValidatorPubKey() == blockCopyConstructor.getValidatorPubKey());
      REQUIRE(reconstructedBlock.isFinalized() == blockCopyConstructor.isFinalized());

      std::shared_ptr<Block> blockPtr = std::make_shared<Block>(std::move(newBlock));

      // New block was moved, check blockPtr and newBlock.
      REQUIRE(blockPtr->getValidatorSig() == reconstructedBlock.getValidatorSig());
      REQUIRE(blockPtr->getPrevBlockHash() == reconstructedBlock.getPrevBlockHash());
      REQUIRE(blockPtr->getBlockRandomness() == reconstructedBlock.getBlockRandomness());
      REQUIRE(blockPtr->getValidatorMerkleRoot() == reconstructedBlock.getValidatorMerkleRoot());
      REQUIRE(blockPtr->getTxMerkleRoot() == reconstructedBlock.getTxMerkleRoot());
      REQUIRE(blockPtr->getTimestamp() == reconstructedBlock.getTimestamp());
      REQUIRE(blockPtr->getNHeight() == reconstructedBlock.getNHeight());
      REQUIRE(blockPtr->getTxValidators() == reconstructedBlock.getTxValidators());
      REQUIRE(blockPtr->getTxs() == reconstructedBlock.getTxs());
      REQUIRE(blockPtr->getValidatorPubKey() == reconstructedBlock.getValidatorPubKey());
      REQUIRE(blockPtr->isFinalized() == reconstructedBlock.isFinalized());

      REQUIRE(newBlock.getValidatorSig().get() == std::string(""));
      REQUIRE(newBlock.getPrevBlockHash().get() == std::string(""));
      REQUIRE(newBlock.getBlockRandomness().get() == std::string(""));
      REQUIRE(newBlock.getValidatorMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTxMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTimestamp() == 1678400201858);
      REQUIRE(newBlock.getNHeight() == 92137812);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
      REQUIRE(newBlock.getValidatorPubKey().get() == std::string(""));
      REQUIRE(newBlock.isFinalized() == false);
    }

    SECTION("Block creation with 10 transactions") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be"));
      uint64_t timestamp = 1678400843315;
      uint64_t nHeight = 100;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
      TxBlock tx(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"));

      for (uint64_t i = 0; i < 10; i++) newBlock.appendTx(tx);

      newBlock.finalize(validatorPrivKey);
      
      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock());

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getValidatorSig() == Signature(Hex::toBytes("1248be6307454c4caf6bb6b0f5dc5c5f9bf94d1e78d78b847e8e93f6b3fbf0df4714c34732cb24c8a8e7009eb24a9705830f17ce7945d835cac632b5dc8e7af201")));
      REQUIRE(reconstructedBlock.getPrevBlockHash() == Hash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be")));
      REQUIRE(reconstructedBlock.getBlockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Hash(Hex::toBytes("5d8c59743808c403ac95ca03937d51bd01661d8951c1af7fade03495475281a5")));
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(1678400843315));
      REQUIRE(reconstructedBlock.getNHeight() == uint64_t(100));
      REQUIRE(reconstructedBlock.getTxValidators().size() == 0);
      REQUIRE(reconstructedBlock.getTxs().size() == 10);
      REQUIRE(reconstructedBlock.getValidatorPubKey() == UPubKey(Hex::toBytes("046ab1f056c30ae181f92e97d0cbb73f4a8778e926c35f10f0c4d1626d8dfd51672366413809a48589aa103e1865e08bd6ddfd0559e095841eb1bd3021d9cc5e62")));
      REQUIRE(reconstructedBlock.isFinalized() == true);

      // Compare transactions with original transactions
      for (uint64_t i = 0; i < 10; i++) REQUIRE(reconstructedBlock.getTxs()[i] == tx);

      // Compare created reconstructed block with created block
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

      // Compare created reconstructed block with block copy constructor
      REQUIRE(reconstructedBlock.getValidatorSig() == blockCopyConstructor.getValidatorSig());
      REQUIRE(reconstructedBlock.getPrevBlockHash() == blockCopyConstructor.getPrevBlockHash());
      REQUIRE(reconstructedBlock.getBlockRandomness() == blockCopyConstructor.getBlockRandomness());
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == blockCopyConstructor.getValidatorMerkleRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == blockCopyConstructor.getTxMerkleRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == blockCopyConstructor.getTimestamp());
      REQUIRE(reconstructedBlock.getNHeight() == blockCopyConstructor.getNHeight());
      REQUIRE(reconstructedBlock.getTxValidators() == blockCopyConstructor.getTxValidators());
      REQUIRE(reconstructedBlock.getTxs() == blockCopyConstructor.getTxs());
      REQUIRE(reconstructedBlock.getValidatorPubKey() == blockCopyConstructor.getValidatorPubKey());
      REQUIRE(reconstructedBlock.isFinalized() == blockCopyConstructor.isFinalized());

      std::shared_ptr<Block> blockPtr = std::make_shared<Block>(std::move(newBlock));

      // New block was moved, check blockPtr and newBlock.
      REQUIRE(blockPtr->getValidatorSig() == reconstructedBlock.getValidatorSig());
      REQUIRE(blockPtr->getPrevBlockHash() == reconstructedBlock.getPrevBlockHash());
      REQUIRE(blockPtr->getBlockRandomness() == reconstructedBlock.getBlockRandomness());
      REQUIRE(blockPtr->getValidatorMerkleRoot() == reconstructedBlock.getValidatorMerkleRoot());
      REQUIRE(blockPtr->getTxMerkleRoot() == reconstructedBlock.getTxMerkleRoot());
      REQUIRE(blockPtr->getTimestamp() == reconstructedBlock.getTimestamp());
      REQUIRE(blockPtr->getNHeight() == reconstructedBlock.getNHeight());
      REQUIRE(blockPtr->getTxValidators() == reconstructedBlock.getTxValidators());
      REQUIRE(blockPtr->getTxs() == reconstructedBlock.getTxs());
      REQUIRE(blockPtr->getValidatorPubKey() == reconstructedBlock.getValidatorPubKey());
      REQUIRE(blockPtr->isFinalized() == reconstructedBlock.isFinalized());

      REQUIRE(newBlock.getValidatorSig().get() == std::string(""));
      REQUIRE(newBlock.getPrevBlockHash().get() == std::string(""));
      REQUIRE(newBlock.getBlockRandomness().get() == std::string(""));
      REQUIRE(newBlock.getValidatorMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTxMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTimestamp() == 1678400843315);
      REQUIRE(newBlock.getNHeight() == 100);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
      REQUIRE(newBlock.getValidatorPubKey().get() == std::string(""));
      REQUIRE(newBlock.isFinalized() == false);
    }

 
    SECTION("Block creation with 64 TxBlock transactions and 16 TxValidator transactions") {
      // There is 16 TxValidator transactions, but only 8 of them are used for block randomness.
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699")); 
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
 
      TxBlock tx(Hex::toBytes("0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"));
 
      for (uint64_t i = 0; i < 64; ++i) {
        newBlock.appendTx(tx);
      }
 
      // Create and append 8 
      std::vector<Hash> randomSeeds(8, Hash::random());
      std::string randomSeed; // Concatenated random seed of block.
      for (const auto &seed : randomSeeds) randomSeed += seed.get();

      std::vector<TxValidator> txValidators;
      Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(txValidatorPrivKey));

      // Create 8 TxValidator transactions with type 0xcfffe746 (random hash)
      for (const auto &seed : randomSeeds) {
         std::string data = Hex::toBytes("0xcfffe746") + Utils::sha3(seed.get()).get();
         txValidators.emplace_back(
          validatorAddress,
          data,
          8080,
          nHeight,
          txValidatorPrivKey
        );
      }

      // create 8 TxValidator transactions with type 0x6fc5a2d6 (random seed)
      for (const auto &seed : randomSeeds) {
         std::string data = Hex::toBytes("0x6fc5a2d6") + seed.get();
         txValidators.emplace_back(
          validatorAddress,
          data,
          8080,
          nHeight,
          txValidatorPrivKey
        );
      }

      // Append transactions to block.
      for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);

      // Sign block with block validator private key.
      newBlock.finalize(blockValidatorPrivKey);

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock());

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getPrevBlockHash() == Hash(Hex::toBytes("7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b")));
      REQUIRE(reconstructedBlock.getBlockRandomness() == Utils::sha3(randomSeed));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Merkle(txValidators).getRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Hash(Hex::toBytes("7e5ad20b00eccd6eef2b87c1eb774a34b0aa10ef6e7e4ccd2642823a1ad34df7")));
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(1678464099412509));
      REQUIRE(reconstructedBlock.getNHeight() == uint64_t(331653115));
      REQUIRE(reconstructedBlock.getTxValidators().size() == 16);
      REQUIRE(reconstructedBlock.getTxs().size() == 64);
      REQUIRE(reconstructedBlock.getValidatorPubKey() == UPubKey(Hex::toBytes("04fe2ce68b894b105f4e5ce5047cfb5dd77570fc512509125cffa2bdbf5539f253116e1d4d9a32b3c3680a1cda5a79e70148908cd9adf18d1d9d7b4e2723b6085e")));
      REQUIRE(reconstructedBlock.isFinalized() == true);

      // Compare transactions with original transactions
      for (uint64_t i = 0; i < 64; ++i) REQUIRE(reconstructedBlock.getTxs()[i] == tx);
      for (uint64_t i = 0; i < 16; ++i) REQUIRE(reconstructedBlock.getTxValidators()[i] == txValidators[i]);

      // Compare created reconstructed block with created block
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

      // Compare created reconstructed block with block copy constructor
      REQUIRE(reconstructedBlock.getValidatorSig() == blockCopyConstructor.getValidatorSig());
      REQUIRE(reconstructedBlock.getPrevBlockHash() == blockCopyConstructor.getPrevBlockHash());
      REQUIRE(reconstructedBlock.getBlockRandomness() == blockCopyConstructor.getBlockRandomness());
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == blockCopyConstructor.getValidatorMerkleRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == blockCopyConstructor.getTxMerkleRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == blockCopyConstructor.getTimestamp());
      REQUIRE(reconstructedBlock.getNHeight() == blockCopyConstructor.getNHeight());
      REQUIRE(reconstructedBlock.getTxValidators() == blockCopyConstructor.getTxValidators());
      REQUIRE(reconstructedBlock.getTxs() == blockCopyConstructor.getTxs());
      REQUIRE(reconstructedBlock.getValidatorPubKey() == blockCopyConstructor.getValidatorPubKey());
      REQUIRE(reconstructedBlock.isFinalized() == blockCopyConstructor.isFinalized());

      std::shared_ptr<Block> blockPtr = std::make_shared<Block>(std::move(newBlock));

      // New block was moved, check blockPtr and newBlock.
      REQUIRE(blockPtr->getValidatorSig() == reconstructedBlock.getValidatorSig());
      REQUIRE(blockPtr->getPrevBlockHash() == reconstructedBlock.getPrevBlockHash());
      REQUIRE(blockPtr->getBlockRandomness() == reconstructedBlock.getBlockRandomness());
      REQUIRE(blockPtr->getValidatorMerkleRoot() == reconstructedBlock.getValidatorMerkleRoot());
      REQUIRE(blockPtr->getTxMerkleRoot() == reconstructedBlock.getTxMerkleRoot());
      REQUIRE(blockPtr->getTimestamp() == reconstructedBlock.getTimestamp());
      REQUIRE(blockPtr->getNHeight() == reconstructedBlock.getNHeight());
      REQUIRE(blockPtr->getTxValidators() == reconstructedBlock.getTxValidators());
      REQUIRE(blockPtr->getTxs() == reconstructedBlock.getTxs());
      REQUIRE(blockPtr->getValidatorPubKey() == reconstructedBlock.getValidatorPubKey());
      REQUIRE(blockPtr->isFinalized() == reconstructedBlock.isFinalized());

      REQUIRE(newBlock.getValidatorSig().get() == std::string(""));
      REQUIRE(newBlock.getPrevBlockHash().get() == std::string(""));
      REQUIRE(newBlock.getBlockRandomness().get() == std::string(""));
      REQUIRE(newBlock.getValidatorMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTxMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTimestamp() == 1678464099412509);
      REQUIRE(newBlock.getNHeight() == 331653115);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
      REQUIRE(newBlock.getValidatorPubKey().get() == std::string(""));
      REQUIRE(newBlock.isFinalized() == false);
    }

    SECTION("Block with 500 dynamically created transactions and 64 dynamically created validator transactions") {
      // There is 16 TxValidator transactions, but only 8 of them are used for block randomness.
      PrivKey blockValidatorPrivKey = PrivKey::random();
      Hash nPrevBlockHash = Hash::random();
      uint64_t timestamp = 64545214243;
      uint64_t nHeight = 6414363551;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
 
      std::vector<TxBlock> txs;

      for (uint64_t i = 0; i < 500; ++i) {
        PrivKey txPrivKey = PrivKey::random();
        Address from = Secp256k1::toAddress(Secp256k1::toUPub(txPrivKey));
        Address to(Utils::randBytes(20), true);
        std::string data = Utils::randBytes(32);
        uint64_t chainId = Utils::bytesToUint32(Utils::randBytes(4));
        uint256_t nonce = Utils::bytesToUint32(Utils::randBytes(4));
        uint256_t value = Utils::bytesToUint64(Utils::randBytes(8));
        uint256_t gas = Utils::bytesToUint32(Utils::randBytes(4));
        uint256_t gasPrice = Utils::bytesToUint32(Utils::randBytes(4));
        txs.emplace_back(
          to,
          from,
          data,
          chainId,
          nonce,
          value,
          gas,
          gasPrice,
          txPrivKey
        );
      }
 
      // Create and append 32 randomSeeds
      std::vector<Hash> randomSeeds(32, Hash::random());
      std::string randomSeed; // Concatenated random seed of block.
      for (const auto &seed : randomSeeds) randomSeed += seed.get();

      std::vector<TxValidator> txValidators;

      // Create 64 TxValidator transactions, half for each type.
      for (const auto &seed : randomSeeds) {
        PrivKey txValidatorPrivKey = PrivKey::random();
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(txValidatorPrivKey));
        std::string hashTxData = Hex::toBytes("0xcfffe746") + Utils::sha3(seed.get()).get();
        txValidators.emplace_back(
          validatorAddress,
          hashTxData,
          8080,
          nHeight,
          txValidatorPrivKey
        );
        std::string seedTxData = Hex::toBytes("0x6fc5a2d6") + seed.get();
        txValidators.emplace_back(
          validatorAddress,
          seedTxData,
          8080,
          nHeight,
          txValidatorPrivKey
        );
      }

      // Append transactions to block.
      for (const auto &tx : txs) newBlock.appendTx(tx);
      for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);

      // Sign block with block validator private key.
      newBlock.finalize(blockValidatorPrivKey);

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock());

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getPrevBlockHash() == nPrevBlockHash);
      REQUIRE(reconstructedBlock.getBlockRandomness() == Utils::sha3(randomSeed));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Merkle(txValidators).getRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Merkle(txs).getRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(64545214243));
      REQUIRE(reconstructedBlock.getNHeight() == uint64_t(6414363551));
      REQUIRE(reconstructedBlock.getTxValidators().size() == 64);
      REQUIRE(reconstructedBlock.getTxs().size() == 500);


      // Compare transactions with original transactions
      for (uint64_t i = 0; i < 500; ++i) REQUIRE(reconstructedBlock.getTxs()[i] == txs[i]);
      for (uint64_t i = 0; i < 64; ++i) REQUIRE(reconstructedBlock.getTxValidators()[i] == txValidators[i]);

      // Compare created reconstructed block with created block
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

      // Compare created reconstructed block with block copy constructor
      REQUIRE(reconstructedBlock.getValidatorSig() == blockCopyConstructor.getValidatorSig());
      REQUIRE(reconstructedBlock.getPrevBlockHash() == blockCopyConstructor.getPrevBlockHash());
      REQUIRE(reconstructedBlock.getBlockRandomness() == blockCopyConstructor.getBlockRandomness());
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == blockCopyConstructor.getValidatorMerkleRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == blockCopyConstructor.getTxMerkleRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == blockCopyConstructor.getTimestamp());
      REQUIRE(reconstructedBlock.getNHeight() == blockCopyConstructor.getNHeight());
      REQUIRE(reconstructedBlock.getTxValidators() == blockCopyConstructor.getTxValidators());
      REQUIRE(reconstructedBlock.getTxs() == blockCopyConstructor.getTxs());
      REQUIRE(reconstructedBlock.getValidatorPubKey() == blockCopyConstructor.getValidatorPubKey());
      REQUIRE(reconstructedBlock.isFinalized() == blockCopyConstructor.isFinalized());

      std::shared_ptr<Block> blockPtr = std::make_shared<Block>(std::move(newBlock));

      // New block was moved, check blockPtr and newBlock.
      REQUIRE(blockPtr->getValidatorSig() == reconstructedBlock.getValidatorSig());
      REQUIRE(blockPtr->getPrevBlockHash() == reconstructedBlock.getPrevBlockHash());
      REQUIRE(blockPtr->getBlockRandomness() == reconstructedBlock.getBlockRandomness());
      REQUIRE(blockPtr->getValidatorMerkleRoot() == reconstructedBlock.getValidatorMerkleRoot());
      REQUIRE(blockPtr->getTxMerkleRoot() == reconstructedBlock.getTxMerkleRoot());
      REQUIRE(blockPtr->getTimestamp() == reconstructedBlock.getTimestamp());
      REQUIRE(blockPtr->getNHeight() == reconstructedBlock.getNHeight());
      REQUIRE(blockPtr->getTxValidators() == reconstructedBlock.getTxValidators());
      REQUIRE(blockPtr->getTxs() == reconstructedBlock.getTxs());
      REQUIRE(blockPtr->getValidatorPubKey() == reconstructedBlock.getValidatorPubKey());
      REQUIRE(blockPtr->isFinalized() == reconstructedBlock.isFinalized());

      REQUIRE(newBlock.getValidatorSig().get() == std::string(""));
      REQUIRE(newBlock.getPrevBlockHash().get() == std::string(""));
      REQUIRE(newBlock.getBlockRandomness().get() == std::string(""));
      REQUIRE(newBlock.getValidatorMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTxMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTimestamp() == 64545214243);
      REQUIRE(newBlock.getNHeight() == 6414363551);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
      REQUIRE(newBlock.getValidatorPubKey().get() == std::string(""));
      REQUIRE(newBlock.isFinalized() == false);
    }

    SECTION("Block with 40000 dynamically created transactions and 256 dynamically created validator transactions") {
      // There is 16 TxValidator transactions, but only 8 of them are used for block randomness.
      PrivKey blockValidatorPrivKey = PrivKey::random();
      Hash nPrevBlockHash = Hash::random();
      uint64_t timestamp = 230915972837111;
      uint64_t nHeight = 239178513;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
 
      std::vector<TxBlock> txs;

      for (uint64_t i = 0; i < 40000; ++i) {
        PrivKey txPrivKey = PrivKey::random();
        Address from = Secp256k1::toAddress(Secp256k1::toUPub(txPrivKey));
        Address to(Utils::randBytes(20), true);
        std::string data = Utils::randBytes(32);
        uint64_t chainId = Utils::bytesToUint32(Utils::randBytes(4));
        uint256_t nonce = Utils::bytesToUint32(Utils::randBytes(4));
        uint256_t value = Utils::bytesToUint64(Utils::randBytes(8));
        uint256_t gas = Utils::bytesToUint32(Utils::randBytes(4));
        uint256_t gasPrice = Utils::bytesToUint32(Utils::randBytes(4));
        txs.emplace_back(
          to,
          from,
          data,
          chainId,
          nonce,
          value,
          gas,
          gasPrice,
          txPrivKey
        );
      }
 
      // Create and append 32 randomSeeds
      std::vector<Hash> randomSeeds(128, Hash::random());
      std::string randomSeed; // Concatenated random seed of block.
      for (const auto &seed : randomSeeds) randomSeed += seed.get();

      std::vector<TxValidator> txValidators;

      // Create 64 TxValidator transactions, half for each type.
      for (const auto &seed : randomSeeds) {
        PrivKey txValidatorPrivKey = PrivKey::random();
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(txValidatorPrivKey));
        std::string hashTxData = Hex::toBytes("0xcfffe746") + Utils::sha3(seed.get()).get();
        txValidators.emplace_back(
          validatorAddress,
          hashTxData,
          8080,
          nHeight,
          txValidatorPrivKey
        );
        std::string seedTxData = Hex::toBytes("0x6fc5a2d6") + seed.get();
        txValidators.emplace_back(
          validatorAddress,
          seedTxData,
          8080,
          nHeight,
          txValidatorPrivKey
        );
      }

      // Append transactions to block.
      for (const auto &tx : txs) newBlock.appendTx(tx);
      for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);

      // Sign block with block validator private key.
      newBlock.finalize(blockValidatorPrivKey);

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock());

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getPrevBlockHash() == nPrevBlockHash);
      REQUIRE(reconstructedBlock.getBlockRandomness() == Utils::sha3(randomSeed));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Merkle(txValidators).getRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Merkle(txs).getRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(230915972837111));
      REQUIRE(reconstructedBlock.getNHeight() == uint64_t(239178513));
      REQUIRE(reconstructedBlock.getTxValidators().size() == 256);
      REQUIRE(reconstructedBlock.getTxs().size() == 40000);


      // Compare transactions with original transactions
      for (uint64_t i = 0; i < 40000; ++i) REQUIRE(reconstructedBlock.getTxs()[i] == txs[i]);
      for (uint64_t i = 0; i < 256; ++i) REQUIRE(reconstructedBlock.getTxValidators()[i] == txValidators[i]);

      // Compare created reconstructed block with created block
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

      // Compare created reconstructed block with block copy constructor
      REQUIRE(reconstructedBlock.getValidatorSig() == blockCopyConstructor.getValidatorSig());
      REQUIRE(reconstructedBlock.getPrevBlockHash() == blockCopyConstructor.getPrevBlockHash());
      REQUIRE(reconstructedBlock.getBlockRandomness() == blockCopyConstructor.getBlockRandomness());
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == blockCopyConstructor.getValidatorMerkleRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == blockCopyConstructor.getTxMerkleRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == blockCopyConstructor.getTimestamp());
      REQUIRE(reconstructedBlock.getNHeight() == blockCopyConstructor.getNHeight());
      REQUIRE(reconstructedBlock.getTxValidators() == blockCopyConstructor.getTxValidators());
      REQUIRE(reconstructedBlock.getTxs() == blockCopyConstructor.getTxs());
      REQUIRE(reconstructedBlock.getValidatorPubKey() == blockCopyConstructor.getValidatorPubKey());
      REQUIRE(reconstructedBlock.isFinalized() == blockCopyConstructor.isFinalized());

      std::shared_ptr<Block> blockPtr = std::make_shared<Block>(std::move(newBlock));

      // New block was moved, check blockPtr and newBlock.
      REQUIRE(blockPtr->getValidatorSig() == reconstructedBlock.getValidatorSig());
      REQUIRE(blockPtr->getPrevBlockHash() == reconstructedBlock.getPrevBlockHash());
      REQUIRE(blockPtr->getBlockRandomness() == reconstructedBlock.getBlockRandomness());
      REQUIRE(blockPtr->getValidatorMerkleRoot() == reconstructedBlock.getValidatorMerkleRoot());
      REQUIRE(blockPtr->getTxMerkleRoot() == reconstructedBlock.getTxMerkleRoot());
      REQUIRE(blockPtr->getTimestamp() == reconstructedBlock.getTimestamp());
      REQUIRE(blockPtr->getNHeight() == reconstructedBlock.getNHeight());
      REQUIRE(blockPtr->getTxValidators() == reconstructedBlock.getTxValidators());
      REQUIRE(blockPtr->getTxs() == reconstructedBlock.getTxs());
      REQUIRE(blockPtr->getValidatorPubKey() == reconstructedBlock.getValidatorPubKey());
      REQUIRE(blockPtr->isFinalized() == reconstructedBlock.isFinalized());

      REQUIRE(newBlock.getValidatorSig().get() == std::string(""));
      REQUIRE(newBlock.getPrevBlockHash().get() == std::string(""));
      REQUIRE(newBlock.getBlockRandomness().get() == std::string(""));
      REQUIRE(newBlock.getValidatorMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTxMerkleRoot().get() == std::string(""));
      REQUIRE(newBlock.getTimestamp() == 230915972837111);
      REQUIRE(newBlock.getNHeight() == 239178513);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
      REQUIRE(newBlock.getValidatorPubKey().get() == std::string(""));
      REQUIRE(newBlock.isFinalized() == false);
    }
  }
}