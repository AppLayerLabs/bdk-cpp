/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/storage.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"

#include <filesystem>
#include <utility>

// Initialize db to be used in tests.
// DB here is the same
void initialize(std::unique_ptr<DB> &db, std::unique_ptr<Storage>& storage, std::unique_ptr<Options>& options, bool clearDB = true) {
  std::string testDumpPath = Utils::getTestDumpPath();
  if (clearDB) {
    if (std::filesystem::exists(testDumpPath + "/blocksTests")) {
      std::filesystem::remove_all(testDumpPath + "/blocksTests");
    }
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  db = std::make_unique<DB>(testDumpPath + "/blocksTests/db");
  std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
  options = std::make_unique<Options>(
    testDumpPath + "/blocksTests",
    "OrbiterSDK/cpp/linux_x86-64/0.1.2",
    1,
    8080,
    8080,
    9999,
    discoveryNodes
  );
  storage = std::make_unique<Storage>(db, options);
}

// Helper functions to create data.

// Random transaction
TxBlock createRandomTx(const uint64_t& requiredChainId) {
  PrivKey txPrivKey = PrivKey::random();
  Address from = Secp256k1::toAddress(Secp256k1::toUPub(txPrivKey));
  Address to(Utils::randBytes(20));
  Bytes data = Utils::randBytes(32);
  uint64_t chainId = requiredChainId;
  uint256_t nonce = Utils::bytesToUint32(Utils::randBytes(4));
  uint256_t value = Utils::bytesToUint64(Utils::randBytes(8));
  uint256_t maxGasPerFee = Utils::bytesToUint32(Utils::randBytes(4));
  uint256_t maxPriorityFeePerGas = Utils::bytesToUint32(Utils::randBytes(4));
  uint256_t gasLimit = Utils::bytesToUint32(Utils::randBytes(4));

  return TxBlock(to, from, data, chainId, nonce, value, maxPriorityFeePerGas, maxGasPerFee, gasLimit, txPrivKey);
}

// Random list of TxValidator transactions and the corresponding seedRandomness.
std::pair<std::vector<TxValidator>, Bytes> createRandomTxValidatorList(uint64_t nHeight, uint64_t N, const uint64_t& requiredChainId) {
  std::pair<std::vector<TxValidator>, Bytes> ret;
  Bytes randomnessStr;
  ret.first.reserve(N);

  std::vector<Hash> seeds(N, Hash::random());
  for (const auto& seed : seeds) {
    Utils::appendBytes(ret.second, seed);
    PrivKey txValidatorPrivKey = PrivKey::random();
    Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(txValidatorPrivKey));
    Bytes hashTxData = Hex::toBytes("0xcfffe746");
    Utils::appendBytes(hashTxData, Utils::sha3(seed.get()));
    ret.first.emplace_back(
      validatorAddress,
      hashTxData,
      requiredChainId,
      nHeight,
      txValidatorPrivKey
    );
    Bytes seedTxData = Hex::toBytes("0x6fc5a2d6");
    Utils::appendBytes(seedTxData, seed.get());
    ret.first.emplace_back(
      validatorAddress,
      seedTxData,
      requiredChainId,
      nHeight,
      txValidatorPrivKey
    );
  }

  return ret;
}

Block createRandomBlock(uint64_t txCount, uint64_t validatorCount, uint64_t nHeight, Hash prevHash, const uint64_t& requiredChainId) {
  PrivKey blockValidatorPrivKey = PrivKey::random();
  Hash nPrevBlockHash = prevHash;
  uint64_t timestamp = 230915972837111; // Timestamp doesn't really matter.
  Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

  std::vector<TxBlock> txs;

  for (uint64_t i = 0; i < txCount; ++i) {
    txs.emplace_back(createRandomTx(requiredChainId));
  }

  // Create and append 8 randomSeeds
  auto randomnessResult = createRandomTxValidatorList(nHeight, validatorCount, requiredChainId);
  Bytes randomSeed = randomnessResult.second;
  std::vector<TxValidator> txValidators = randomnessResult.first;

  // Append transactions to block.
  for (const auto &tx : txs) newBlock.appendTx(tx);
  for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);
  // Sign block with block validator private key.
  newBlock.finalize(blockValidatorPrivKey, timestamp + 1);
  REQUIRE(newBlock.getBlockRandomness() == Hash(Utils::sha3(randomSeed)));
  return newBlock;
}

namespace TStorage {
  TEST_CASE("Storage Class", "[core][storage]") {
    SECTION("Simple Storage Startup") {
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<Options> options;
      initialize(db, storage, options);
      // Chain should be filled with the genesis.
      REQUIRE(storage->currentChainSize() == 1);
      auto genesis = storage->latest();
      REQUIRE(genesis->getValidatorSig() == Signature(Hex::toBytes("7f31ae12a792653ea222f66bd9a6b8b0c72cb2e6ba952ba3706de01a71e6b5d63030de6302f1d2fe85a22d2122b90a11ad9f7cc7bf5c517049bf170dede9370600")));
      REQUIRE(genesis->getPrevBlockHash() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(genesis->getBlockRandomness() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(genesis->getValidatorMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(genesis->getTxMerkleRoot() == Hash(Hex::toBytes("0000000000000000000000000000000000000000000000000000000000000000")));
      REQUIRE(genesis->getTimestamp() == uint64_t(1656356646000000));
      REQUIRE(genesis->getNHeight() == uint64_t(0));
      REQUIRE(genesis->getTxValidators().size() == 0);
      REQUIRE(genesis->getTxs().size() == 0);
      REQUIRE(genesis->getValidatorPubKey() == UPubKey(Hex::toBytes("04eb4c1da10ca5f1e52d1cba87f627931b5a980dba6d910d6aa756db62fc71ea78db1a18a2c364fb348bb28e0b0a3c6563a0522626eecfe32cdab30746365f5747")));
      REQUIRE(Secp256k1::toAddress(genesis->getValidatorPubKey()) == Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")));
      REQUIRE(genesis->isFinalized() == true);
    }

    SECTION("10 Blocks forward with destructor test") {
      // Create 10 Blocks, each with 100 dynamic transactions and 16 validator transactions
      std::vector<Block> blocks;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<Options> options;
        initialize(db, storage, options);

        // Generate 10 blocks.
        for (uint64_t i = 0; i < 10; ++i) {
          auto latest = storage->latest();
          Block newBlock = createRandomBlock(100, 16, latest->getNHeight() + 1, latest->hash(), options->getChainID());
          blocks.emplace_back(newBlock);
          storage->pushBack(Block(newBlock));
        }

        REQUIRE(storage->currentChainSize() == 11);
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

      // Load DB again...
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<Options> options;
      initialize(db, storage, options, false);
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
    }

    SECTION("2000 Blocks forward with N (0...16) dynamic normal txs and 32 validator txs, with SaveToDB and Tx Cache test") {
      // Create 2000 Blocks, each with 0 to 16 dynamic transactions and 32 validator transactions
      std::vector<std::pair<Block,std::vector<TxBlock>>> blocksWithTxs;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<Options> options;
        initialize(db, storage, options);

        // Generate 10 blocks.
        for (uint64_t i = 0; i < 2000; ++i) {
          auto latest = storage->latest();
          uint64_t txCount = uint64_t(uint8_t(Utils::randBytes(1)[0]) % 16);
          Block newBlock = createRandomBlock(txCount, 16, latest->getNHeight() + 1, latest->hash(), options->getChainID());
          std::vector<TxBlock> txs = newBlock.getTxs();
          blocksWithTxs.emplace_back(std::make_pair(newBlock, txs));
          storage->pushBack(std::move(newBlock));
        }

        REQUIRE(storage->currentChainSize() == 2001);
        // Check if the chain is filled with the correct blocks.
        for (uint64_t i = 0; i < 2000; i++) {
          auto block = storage->getBlock(i + 1);
          const auto& [requiredBlock, requiredTxs] = blocksWithTxs[i];
          REQUIRE(block->getValidatorSig() == requiredBlock.getValidatorSig());
          REQUIRE(block->getPrevBlockHash() == requiredBlock.getPrevBlockHash());
          REQUIRE(block->getBlockRandomness() == requiredBlock.getBlockRandomness());
          REQUIRE(block->getValidatorMerkleRoot() == requiredBlock.getValidatorMerkleRoot());
          REQUIRE(block->getTxMerkleRoot() == requiredBlock.getTxMerkleRoot());
          REQUIRE(block->getTimestamp() == requiredBlock.getTimestamp());
          REQUIRE(block->getNHeight() == requiredBlock.getNHeight());
          REQUIRE(block->getTxs() == requiredBlock.getTxs());
          REQUIRE(block->getTxValidators() == requiredBlock.getTxValidators());
          REQUIRE(block->getTxValidators().size() == requiredBlock.getTxValidators().size());
          REQUIRE(block->getTxs().size() == requiredBlock.getTxs().size());
          REQUIRE(block->getValidatorPubKey() == requiredBlock.getValidatorPubKey());
          REQUIRE(block->isFinalized() == requiredBlock.isFinalized());
        }
      }
      // Load DB again...
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<Options> options;
      initialize(db, storage, options, false);
      // Required to initialize the same chain as before.
      auto latest = storage->latest();
      REQUIRE(*latest == blocksWithTxs[1999].first);
      for (uint64_t i = 0; i < 2000; i++) {
        // blocksWithTxs doesn't include the genesis block, we have to skip it
        auto block = storage->getBlock(i + 1);
        const auto& [requiredBlock, requiredTxs] = blocksWithTxs[i];
        REQUIRE(block->getValidatorSig() == requiredBlock.getValidatorSig());
        REQUIRE(block->getPrevBlockHash() == requiredBlock.getPrevBlockHash());
        REQUIRE(block->getBlockRandomness() == requiredBlock.getBlockRandomness());
        REQUIRE(block->getValidatorMerkleRoot() == requiredBlock.getValidatorMerkleRoot());
        REQUIRE(block->getTxMerkleRoot() == requiredBlock.getTxMerkleRoot());
        REQUIRE(block->getTimestamp() == requiredBlock.getTimestamp());
        REQUIRE(block->getNHeight() == requiredBlock.getNHeight());
        REQUIRE(block->getTxs() == requiredBlock.getTxs());
        REQUIRE(block->getTxValidators() == requiredBlock.getTxValidators());
        REQUIRE(block->getTxValidators().size() == requiredBlock.getTxValidators().size());
        REQUIRE(block->getTxs().size() == requiredBlock.getTxs().size());
        REQUIRE(block->getValidatorPubKey() == requiredBlock.getValidatorPubKey());
        REQUIRE(block->isFinalized() == requiredBlock.isFinalized());

        const auto& requiredBlockHash = requiredBlock.hash();
        for (uint64_t ii = 0; ii < requiredTxs.size(); ii++) {
          auto txInfo = storage->getTx(requiredTxs[ii].hash());
          const auto& [tx, blockHash, blockIndex, blockHeight] = txInfo;
          REQUIRE(blockHash == requiredBlockHash);
          REQUIRE(blockIndex == ii);
          REQUIRE(blockHeight == i + 1);
          REQUIRE(tx->hash() == requiredTxs[ii].hash());
        }
      }
    }
  }
}
