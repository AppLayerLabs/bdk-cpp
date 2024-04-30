/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/storage.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../blockchainwrapper.hpp"

#include <filesystem>
#include <utility>

const std::vector<Hash> validatorPrivKeysStorage {
  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

// Blockchain wrapper initializer for testing purposes.
// Defined in rdpos.cpp
TestBlockchainWrapper initialize(const std::vector<Hash>& validatorPrivKeys,
                                 const PrivKey& validatorKey,
                                 const uint64_t& serverPort,
                                 bool clearDb,
                                 const std::string& folderName);

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

FinalizedBlock createRandomBlock(uint64_t txCount, uint64_t validatorCount, uint64_t nHeight, Hash prevHash, const uint64_t& requiredChainId) {
  PrivKey blockValidatorPrivKey = PrivKey::random();
  Hash nPrevBlockHash = prevHash;
  uint64_t timestamp = 230915972837111; // Timestamp doesn't really matter.
  MutableBlock newBlock(nPrevBlockHash, timestamp, nHeight);

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
  FinalizedBlock finalBlock = newBlock.finalize(blockValidatorPrivKey, timestamp + 1);
  REQUIRE(finalBlock.getBlockRandomness() == Hash(Utils::sha3(randomSeed)));
  return finalBlock;
}

namespace TStorage {
  TEST_CASE("Storage Class", "[core][storage]") {
    SECTION("Simple Storage Startup") {

      auto blockchainWrapper = initialize(validatorPrivKeysStorage, PrivKey(), 8080, true, "StorageConstructor");
      // Chain should be filled with the genesis.
      REQUIRE(blockchainWrapper.storage.currentChainSize() == 1);
      auto genesis = blockchainWrapper.storage.latest();
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
    }

    SECTION("10 Blocks forward with destructor test") {
      // Create 10 Blocks, each with 100 dynamic transactions and 16 validator transactions
      std::vector<FinalizedBlock> blocks;
      {
        auto blockchainWrapper = initialize(validatorPrivKeysStorage, PrivKey(), 8080, true, "Storage10BlocksForwardDestructor");

        // Generate 10 blocks.
        for (uint64_t i = 0; i < 10; ++i) {
          auto latest = blockchainWrapper.storage.latest();
          FinalizedBlock newBlock = createRandomBlock(100, 16, latest->getNHeight() + 1, latest->getHash(), blockchainWrapper.options.getChainID());
          blocks.emplace_back(newBlock);
          blockchainWrapper.storage.pushBack(FinalizedBlock(newBlock));
        }

        REQUIRE(blockchainWrapper.storage.currentChainSize() == 11);
        // Check if the chain is filled with the correct blocks.
        for (uint64_t i = 0; i < 10; i++) {
          auto block = blockchainWrapper.storage.getBlock(i + 1);
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
        }
        /// We actually need to dump the state otherwise it WILL try to process the added blocks
        /// in the constructor of the State class.
        /// Dumping the state will say to the State class that there is no missing blocks and it will
        /// not try to process the blocks in the constructor.
        blockchainWrapper.state.saveToDB();
      }

      // Load DB again...
      auto blockchainWrapper = initialize(validatorPrivKeysStorage, PrivKey(), 8080, false, "Storage10BlocksForwardDestructor");
      // Required to initialize the same chain as before.
      auto latest = blockchainWrapper.storage.latest();
      REQUIRE(*latest == blocks[9]);
      for (uint64_t i = 0; i < 10; i++) {
        auto block = blockchainWrapper.storage.getBlock(i + 1);
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
      }
    }

    SECTION("2000 Blocks forward with N (0...16) dynamic normal txs and 32 validator txs, with SaveToDB and Tx Cache test") {
      // Create 2000 Blocks, each with 0 to 16 dynamic transactions and 32 validator transactions
      std::vector<std::pair<FinalizedBlock,std::vector<TxBlock>>> blocksWithTxs;
      {
        auto blockchainWrapper = initialize(validatorPrivKeysStorage, PrivKey(), 8080, true, "Storage2000BlocksForwardSaveToDBTxCache");

        // Generate 10 blocks.
        for (uint64_t i = 0; i < 2000; ++i) {
          auto latest = blockchainWrapper.storage.latest();
          uint64_t txCount = uint64_t(uint8_t(Utils::randBytes(1)[0]) % 16);
          FinalizedBlock newBlock = createRandomBlock(txCount, 16, latest->getNHeight() + 1, latest->getHash(), blockchainWrapper.options.getChainID());
          std::vector<TxBlock> txs = newBlock.getTxs();
          blocksWithTxs.emplace_back(std::make_pair(newBlock, txs));
          blockchainWrapper.storage.pushBack(std::move(newBlock));
        }

        REQUIRE(blockchainWrapper.storage.currentChainSize() == 2001);
        // Check if the chain is filled with the correct blocks.
        for (uint64_t i = 0; i < 2000; i++) {
          auto block = blockchainWrapper.storage.getBlock(i + 1);
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
        }
        /// Same as before, we need to dump the state to avoid processing the blocks in the constructor.
        blockchainWrapper.state.saveToDB();
      }
      // Load DB again...
      auto blockchainWrapper = initialize(validatorPrivKeysStorage, PrivKey(), 8080, false, "Storage2000BlocksForwardSaveToDBTxCache");
      // Required to initialize the same chain as before.
      auto latest = blockchainWrapper.storage.latest();
      REQUIRE(*latest == blocksWithTxs[1999].first);
      for (uint64_t i = 0; i < 2000; i++) {
        // blocksWithTxs doesn't include the genesis block, we have to skip it
        auto block = blockchainWrapper.storage.getBlock(i + 1);
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

        const auto& requiredBlockHash = requiredBlock.getHash();
        for (uint64_t ii = 0; ii < requiredTxs.size(); ii++) {
          auto txInfo = blockchainWrapper.storage.getTx(requiredTxs[ii].hash());
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
