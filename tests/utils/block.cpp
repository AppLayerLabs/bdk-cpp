#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/tx.h"
#include "../../src/utils/block.h"

using Catch::Matchers::Equals;

namespace TBlock {
  TEST_CASE("Block Class", "[utils][block]") {
    SECTION("Block creation with no transactions") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6"));
      uint64_t timestamp = 1678400201858;
      uint64_t nHeight = 92137812;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      newBlock.finalize(validatorPrivKey, timestamp+1);

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock(), 8080);

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getValidatorSig() == Signature(Hex::toBytes("18395ff0c8ee38a250b9e7aeb5733c437fed8d6ca2135fa634367bb288a3830a3c624e33401a1798ce09f049fb6507adc52b085d0a83dacc43adfa519c1228e701")));
      REQUIRE(reconstructedBlock.getPrevBlockHash() == Hash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6")));
      REQUIRE(reconstructedBlock.getBlockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(1678400201859));
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

      REQUIRE(newBlock.getTimestamp() == 1678400201859);
      REQUIRE(newBlock.getNHeight() == 92137812);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
      REQUIRE(newBlock.isFinalized() == false);
    }

    SECTION("Block creation with 10 transactions") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be"));
      uint64_t timestamp = 1678400843315;
      uint64_t nHeight = 100;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
      TxBlock tx(Hex::toBytes("0x02f874821f9080849502f900849502f900825208942e951aa58c8b9b504a97f597bbb2765c011a8802880de0b6b3a764000080c001a0f56fe87778b4420d3b0f8eba91d28093abfdbea281a188b8516dd8411dc223d7a05c2d2d71ad3473571ff637907d72e6ac399fe4804641dbd9e2d863586c57717d"), 1);

      for (uint64_t i = 0; i < 10; i++) newBlock.appendTx(tx);

      newBlock.finalize(validatorPrivKey, timestamp+1);

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock(), 1);

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getValidatorSig() == Signature(Hex::toBytes("7932f2e62d9b7f81ae7d2673d88d9c7ca3aa101c3cd22d76c8ca9063de9126db350c0aa08470cf1a65652bfe1e16f8210af0ecef4f36fe3e01c93b71e75cabd501")));
      REQUIRE(reconstructedBlock.getPrevBlockHash() == Hash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be")));
      REQUIRE(reconstructedBlock.getBlockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Hash(Hex::toBytes("658285e815d4134cc842f23c4e93e07b96e7831e3c22acc9c5db289720d8851e")));
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(1678400843316));
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

      REQUIRE(newBlock.getTimestamp() == 1678400843316);
      REQUIRE(newBlock.getNHeight() == 100);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
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

      TxBlock tx(Hex::toBytes("0x02f874821f9080849502f900849502f900825208942e951aa58c8b9b504a97f597bbb2765c011a8802880de0b6b3a764000080c001a0f56fe87778b4420d3b0f8eba91d28093abfdbea281a188b8516dd8411dc223d7a05c2d2d71ad3473571ff637907d72e6ac399fe4804641dbd9e2d863586c57717d"), 1);

      for (uint64_t i = 0; i < 64; i++) newBlock.appendTx(tx);

      // Create and append 8
      std::vector<Hash> randomSeeds(8, Hash::random());
      Bytes randomSeed; // Concatenated random seed of block.
      for (const auto &seed : randomSeeds) randomSeed.insert(randomSeed.end(), seed.get().begin(), seed.get().end());

      std::vector<TxValidator> txValidators;
      Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(txValidatorPrivKey));

      // Create 8 TxValidator transactions with type 0xcfffe746 (random hash)
      for (const auto &seed : randomSeeds) {
        Bytes data = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(data, Utils::sha3(seed.get()));
        txValidators.emplace_back(
          validatorAddress,
          data,
          8080,
          nHeight,
          txValidatorPrivKey
        );
      }

      // Create 8 TxValidator transactions with type 0x6fc5a2d6 (random seed)
      for (const auto &seed : randomSeeds) {
        Bytes data = Hex::toBytes("0x6fc5a2d6");
        Utils::appendBytes(data, seed);
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
      newBlock.finalize(blockValidatorPrivKey, timestamp+1);

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock(), 8080);

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getPrevBlockHash() == Hash(Hex::toBytes("7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b")));
      REQUIRE(reconstructedBlock.getBlockRandomness() == Utils::sha3(randomSeed));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Merkle(txValidators).getRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Hash(Hex::toBytes("39ba30dc64127c507fe30e2310890667cfbc9fd247ddd8841e5e0573d8dcca9e")));
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(1678464099412510));
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

      REQUIRE(newBlock.getTimestamp() == 1678464099412510);
      REQUIRE(newBlock.getNHeight() == 331653115);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
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
        Address to(Utils::randBytes(20));
        Bytes data = Utils::randBytes(32);
        uint64_t chainId = 8080;
        uint256_t nonce = Utils::bytesToUint32(Utils::randBytes(4));
        uint256_t value = Utils::bytesToUint64(Utils::randBytes(8));
        uint256_t gasLimit = Utils::bytesToUint32(Utils::randBytes(4));
        uint256_t maxFeePerGas = Utils::bytesToUint32(Utils::randBytes(4));
        txs.emplace_back(
          to,
          from,
          data,
          chainId,
          nonce,
          value,
          maxFeePerGas,
          maxFeePerGas,
          gasLimit,
          txPrivKey
        );
      }

      // Create and append 32 randomSeeds
      std::vector<Hash> randomSeeds(32, Hash::random());
      Bytes randomSeed; // Concatenated random seed of block.
      for (const auto &seed : randomSeeds) randomSeed.insert(randomSeed.end(), seed.get().begin(), seed.get().end());

      std::vector<TxValidator> txValidators;

      // Create 64 TxValidator transactions, half for each type.
      for (const auto &seed : randomSeeds) {
        PrivKey txValidatorPrivKey = PrivKey::random();
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(txValidatorPrivKey));
        Bytes hashTxData = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(hashTxData, Utils::sha3(seed.get()));
        txValidators.emplace_back(
          validatorAddress,
          hashTxData,
          8080,
          nHeight,
          txValidatorPrivKey
        );
        Bytes seedTxData = Hex::toBytes("0x6fc5a2d6");
        Utils::appendBytes(seedTxData, seed);
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
      newBlock.finalize(blockValidatorPrivKey, timestamp+1);

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock(), 8080);

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getPrevBlockHash() == nPrevBlockHash);
      REQUIRE(reconstructedBlock.getBlockRandomness() == Utils::sha3(randomSeed));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Merkle(txValidators).getRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Merkle(txs).getRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(64545214244));
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

      REQUIRE(newBlock.getTimestamp() == 64545214244);
      REQUIRE(newBlock.getNHeight() == 6414363551);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
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

      // Create 40000 Transactions with parallelization to speed up tests
      uint64_t nCores = std::thread::hardware_concurrency();
      std::mutex txLock;
      std::vector<uint64_t> nJobPerCore(nCores, 40000 / nCores);
      nJobPerCore.back() += 40000 % nCores;
      std::vector<std::future<bool>> futures;
      futures.reserve(nCores);
      for (uint64_t i = 0; i < nCores; ++i) {
        uint64_t coreJobs = nJobPerCore[i];
        futures.push_back(std::async(std::launch::async, [&txs, &txLock, coreJobs, i] {
          for (uint64_t j = 0; j < coreJobs; ++j) {
            PrivKey txPrivKey = PrivKey::random();
            Address from = Secp256k1::toAddress(Secp256k1::toUPub(txPrivKey));
            Address to(Utils::randBytes(20));
            Bytes data = Utils::randBytes(32);
            uint64_t chainId = 8080;
            uint256_t nonce = Utils::bytesToUint32(Utils::randBytes(4));
            uint256_t value = Utils::bytesToUint64(Utils::randBytes(8));
            uint256_t gasLimit = Utils::bytesToUint32(Utils::randBytes(4));
            uint256_t maxFeePerGas = Utils::bytesToUint32(Utils::randBytes(4));
            auto tx = TxBlock(
              to,
              from,
              data,
              chainId,
              nonce,
              value,
              maxFeePerGas,
              maxFeePerGas,
              gasLimit,
              txPrivKey
            );
            std::unique_lock lock(txLock);
            txs.emplace_back(std::move(tx));
          }
          return true;
        }));
      }

      for (auto &future : futures) future.get();


      // Create and append 32 randomSeeds
      std::vector<Hash> randomSeeds(128, Hash::random());
      Bytes randomSeed; // Concatenated random seed of block.
      for (const auto &seed : randomSeeds) Utils::appendBytes(randomSeed, seed.get());

      std::vector<TxValidator> txValidators;

      // Create 64 TxValidator transactions, half for each type.
      for (const auto &seed : randomSeeds) {
        PrivKey txValidatorPrivKey = PrivKey::random();
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(txValidatorPrivKey));
        Bytes hashTxData = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(hashTxData, Utils::sha3(seed.get()));
        txValidators.emplace_back(
          validatorAddress,
          hashTxData,
          8080,
          nHeight,
          txValidatorPrivKey
        );
        Bytes seedTxData = Hex::toBytes("0x6fc5a2d6");
        Utils::appendBytes(seedTxData, seed);
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
      newBlock.finalize(blockValidatorPrivKey, timestamp+1);

      Block blockCopyConstructor(newBlock);
      Block reconstructedBlock(newBlock.serializeBlock(), 8080);

      // Check within reconstructed block
      REQUIRE(reconstructedBlock.getPrevBlockHash() == nPrevBlockHash);
      REQUIRE(reconstructedBlock.getBlockRandomness() == Utils::sha3(randomSeed));
      REQUIRE(reconstructedBlock.getValidatorMerkleRoot() == Merkle(txValidators).getRoot());
      REQUIRE(reconstructedBlock.getTxMerkleRoot() == Merkle(txs).getRoot());
      REQUIRE(reconstructedBlock.getTimestamp() == uint64_t(230915972837112));
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

      REQUIRE(newBlock.getTimestamp() == 230915972837112);
      REQUIRE(newBlock.getNHeight() == 239178513);
      REQUIRE(newBlock.getTxValidators().size() == 0);
      REQUIRE(newBlock.getTxs().size() == 0);
      REQUIRE(newBlock.isFinalized() == false);
    }
  }
}
