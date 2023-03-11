#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/storage.h"
#include "../../src/utils/db.h"

#include <filesystem>


// TODO: for the love of god fix this:
// if the dbName is the same, the tests will fail because the db will overlap, the LOCK file is not being deleted it seems.

void initialize(std::unique_ptr<DB> &db, std::unique_ptr<Storage>& storage, std::string dbName) {
  if(std::filesystem::exists(dbName)) {
    std::filesystem::remove_all(dbName);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  db = std::make_unique<DB>(dbName);
  storage = std::make_unique<Storage>(db);
}

namespace TStorage {
  TEST_CASE("Storage Class", "[core]") {
    SECTION("Simple Storage Startup") {
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      initialize(db, storage, "simpleStorage");
      // Chain should be filled with the genesis.
      REQUIRE(storage->blockSize() == 1);
      auto genesis = storage->latest();
      REQUIRE(genesis->getValidatorSig() == Signature(Hex::toBytes("e543e00583d25a668712ccbb9d2604778acb2057a580a19ad50479779e684e7e4f4c5792e1e9cfd73b1c69ab897dac6ea8b4509f902283e7ecc724e76a1a68d401")));
      REQUIRE(genesis->getPrevBlockHash() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(genesis->getBlockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(genesis->getValidatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(genesis->getTxMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(genesis->getTimestamp() == uint64_t(1656356645000000));
      REQUIRE(genesis->getNHeight() == uint64_t(0));
      REQUIRE(genesis->getTxValidators().size() == 0);
      REQUIRE(genesis->getTxs().size() == 0);
      REQUIRE(genesis->getValidatorPubKey() == UPubKey(Hex::toBytes("04eb4c1da10ca5f1e52d1cba87f627931b5a980dba6d910d6aa756db62fc71ea78db1a18a2c364fb348bb28e0b0a3c6563a0522626eecfe32cdab30746365f5747")));
      REQUIRE(Secp256k1::toAddress(genesis->getValidatorPubKey()) == Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true));
      REQUIRE(genesis->isFinalized() == true);

    }

    SECTION("10 Blocks forward") {
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      initialize(db, storage, "10BlocksForward");

      // Create 10 Blocks, each with 100 dynamic transactions and 16 validator transactions
      std::vector<Block> blocks;
      for (uint64_t i = 0; i < 10; i++) {
        PrivKey blockValidatorPrivKey = PrivKey::random();
        Hash nPrevBlockHash = (i == 0) ? storage->latest()->hash() : blocks[i - 1].hash();
        uint64_t timestamp = 230915972837111; // Timestamp doesn't really matter.
        uint64_t nHeight = (i == 0) ? storage->latest()->getNHeight() + 1 : blocks[i - 1].getNHeight() + 1;
        Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
  
        std::vector<TxBlock> txs;

        for (uint64_t i = 0; i < 100; ++i) {
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
  
        // Create and append 8 randomSeeds
        std::vector<Hash> randomSeeds(8, Hash::random());
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
        blocks.emplace_back(std::move(newBlock));
      }


      // Append blocks to the chain.
      for (const auto &block : blocks) {
        storage->pushBack(Block(block));
      }

      REQUIRE(storage->blockSize() == 11);
      // Check if the chain is filled with the correct blocks.
      for (uint64_t i = 0; i < 10; i++) {
        auto block = storage->getBlock(i + 1);
        REQUIRE(block->getValidatorSig() == blocks[i].getValidatorSig());
        REQUIRE(block->getPrevBlockHash() == blocks[i].getPrevBlockHash());
        REQUIRE(block->getBlockRandomness() == blocks[i].getBlockRandomness());
        REQUIRE(block->getValidatorMerkleRoot() == blocks[i].getValidatorMerkleRoot());
        REQUIRE(block->getTxMerkleRoot() == blocks[i].getTxMerkleRoot());
        REQUIRE(block->getTimestamp() == blocks[i].getTimestamp());
        REQUIRE(block->getNHeight() == blocks[i].getNHeight());
        REQUIRE(block->getTxs() == blocks[i].getTxs());
        REQUIRE(block->getTxValidators() == blocks[i].getTxValidators());
        REQUIRE(block->getTxValidators().size() == blocks[i].getTxValidators().size());
        REQUIRE(block->getTxs().size() == blocks[i].getTxs().size());
        REQUIRE(block->getValidatorPubKey() == blocks[i].getValidatorPubKey());
        REQUIRE(block->isFinalized() == blocks[i].isFinalized());
      }
    }
  }
}