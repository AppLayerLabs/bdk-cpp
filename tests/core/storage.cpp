#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/storage.h"
#include "../../src/utils/db.h"

#include <filesystem>
#include <utility>


// Initialize db to be used in tests.
// DB here is the same

void initialize(std::unique_ptr<DB> &db, std::unique_ptr<Storage>& storage, bool clearDB = true) {
  if (clearDB) {
    if(std::filesystem::exists("blocksTests")) {
      std::filesystem::remove_all("blocksTests");
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  db = std::make_unique<DB>("blocksTests");
  storage = std::make_unique<Storage>(db);
}

// Helper functions to create data.

// Random transaction
TxBlock createRandomTx() {
  PrivKey txPrivKey = PrivKey::random();
  Address from = Secp256k1::toAddress(Secp256k1::toUPub(txPrivKey));
  Address to(Utils::randBytes(20), true);
  std::string data = Utils::randBytes(32);
  uint64_t chainId = Utils::bytesToUint32(Utils::randBytes(4));
  uint256_t nonce = Utils::bytesToUint32(Utils::randBytes(4));
  uint256_t value = Utils::bytesToUint64(Utils::randBytes(8));
  uint256_t gas = Utils::bytesToUint32(Utils::randBytes(4));
  uint256_t gasPrice = Utils::bytesToUint32(Utils::randBytes(4));
  return TxBlock(to, from, data, chainId, nonce, value, gas, gasPrice, txPrivKey);
}

// Random list of TxValidator transactions and the corresponding seedRandomness.
std::pair<std::vector<TxValidator>, std::string> createRandomTxValidatorList(uint64_t nHeight, uint64_t N) {
  std::pair<std::vector<TxValidator>, std::string> ret;
  std::string randomnessStr;
  ret.first.reserve(N);

  std::vector<Hash> seeds(N, Hash::random());
  for (const auto& seed : seeds) {
    ret.second += seed.get();
    PrivKey txValidatorPrivKey = PrivKey::random();
    Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(txValidatorPrivKey));
    std::string hashTxData = Hex::toBytes("0xcfffe746") + Utils::sha3(seed.get()).get();
    ret.first.emplace_back(
      validatorAddress,
      hashTxData,
      8080,
      nHeight,
      txValidatorPrivKey
    );
    std::string seedTxData = Hex::toBytes("0x6fc5a2d6") + seed.get();
    ret.first.emplace_back(
      validatorAddress,
      seedTxData,
      8080,
      nHeight,
      txValidatorPrivKey
    );
  }

  return ret;
}

Block createRandomBlock(uint64_t txCount, uint64_t validatorCount, uint64_t nHeight, Hash prevHash) {
  PrivKey blockValidatorPrivKey = PrivKey::random();
  Hash nPrevBlockHash = prevHash;
  uint64_t timestamp = 230915972837111; // Timestamp doesn't really matter.
  Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

  std::vector<TxBlock> txs;

  for (uint64_t i = 0; i < txCount; ++i) {
    txs.emplace_back(createRandomTx());
  }

  // Create and append 8 randomSeeds
  auto randomnessResult = createRandomTxValidatorList(nHeight, validatorCount);
  std::string randomSeed = randomnessResult.second;
  std::vector<TxValidator> txValidators = randomnessResult.first;

  // Append transactions to block.
  for (const auto &tx : txs) newBlock.appendTx(tx);
  for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);
  // Sign block with block validator private key.
  newBlock.finalize(blockValidatorPrivKey);
  REQUIRE(newBlock.getBlockRandomness() == Hash(Utils::sha3(randomSeed)));
  return newBlock;
}


namespace TStorage {
  TEST_CASE("Storage Class", "[core]") {
    SECTION("Simple Storage Startup") {
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      initialize(db, storage);
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
      REQUIRE(genesis->getTxValidators().size() == 0);1b
      REQUIRE(genesis->getTxs().size() == 0);
      REQUIRE(genesis->getValidatorPubKey() == UPubKey(Hex::toBytes("04eb4c1da10ca5f1e52d1cba87f627931b5a980dba6d910d6aa756db62fc71ea78db1a18a2c364fb348bb28e0b0a3c6563a0522626eecfe32cdab30746365f5747")));
      REQUIRE(Secp256k1::toAddress(genesis->getValidatorPubKey()) == Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true));
      REQUIRE(genesis->isFinalized() == true);
      db->close();
    }

    SECTION("10 Blocks forward with SaveToDB Test") {
      // Create 10 Blocks, each with 100 dynamic transactions and 16 validator transactions
      std::vector<Block> blocks;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        initialize(db, storage);

        // Generate 10 blocks.
        for (uint64_t i = 0; i < 10; ++i) {
          auto latest = storage->latest();
          Block newBlock = createRandomBlock(100, 16, latest->getNHeight() + 1, latest->hash());
          blocks.emplace_back(newBlock);
          storage->pushBack(Block(newBlock));
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
        storage->saveToDB();
        db->close();
      }

      // Load DB again...
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      initialize(db, storage, false);
      // Required to initialize the same chain as before.
      auto latest = storage->latest();
      REQUIRE(*latest == blocks[9]);
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
      db->close();
    }

    SECTION("2000 Blocks forward with N (0...16) dynamic normal txs and 32 validator txs, with SaveToDB Test") {
      // Create 2000 Blocks, each with 0 to 16 dynamic transactions and 32 validator transactions
      std::vector<Block> blocks;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        initialize(db, storage);

        // Generate 10 blocks.
        for (uint64_t i = 0; i < 2000; ++i) {
          auto latest = storage->latest();
          uint64_t txCount = uint64_t(uint8_t(Utils::randBytes(1)[0]) % 16);  
          Block newBlock = createRandomBlock(txCount, 16, latest->getNHeight() + 1, latest->hash());
          blocks.emplace_back(newBlock);
          storage->pushBack(Block(newBlock));
        }

        REQUIRE(storage->blockSize() == 2001);
        // Check if the chain is filled with the correct blocks.
        for (uint64_t i = 0; i < 2000; i++) {
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
        storage->saveToDB();
        db->close();
      }
      // Load DB again...
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      initialize(db, storage, false);
      // Required to initialize the same chain as before.
      auto latest = storage->latest();
      REQUIRE(*latest == blocks[1999]);
      for (uint64_t i = 0; i < 2000; i++) {
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
      db->close();
    }
  }
}