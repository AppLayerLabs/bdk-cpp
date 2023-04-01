#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/tx.h"
#include "../../src/utils/block.h"

#include "../../src/utils/ecdsa.h"

using Catch::Matchers::Equals;

namespace TBlock {
  TEST_CASE("Block Class (Throw)", "[utils][block]") {
    SECTION("Block with invalid size") {
      bool catched = false;
      std::string_view str(Hex::toBytes(
        "0x9890a27da5231bd842529fa107a6e137e807fb8086f6c740d39a37681e1394317e2b38f540f3a9ed7f0b4f6835fc67613dcb52d2e8b3afa193840441902cc030f2febfaa0a1edd774318d1fe6e3bf1aec16082457f7a66f7fd4bef8ddded9b76d7b9da8a2d15d02eae1743ddcfb9e34fe0374ceaec6e96fb8489d16c6886441697610af9744109384ae774b20eb22cce3677a4c836f57ca30eafc308af2d04cf93ada88ad0fb6968ce6ea1556cc24af1234b8b2d93a0e37a417f53148662659ccdbaa2ed5233d712a2ea93ea0a08e360c72018fa10a8d7"
      ));
      REQUIRE(str.length() < 217);
      try { Block b(str, 8080); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid Validator tx height") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("f8ca8798a75ba3d89ae187d95d7e1944fa07842dd8416e9413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e823f44a0ac5a548f098186dc197b2416cb7188bfe4955b7b554098f5986ad92c3e948f3ea01513ee3cd51b18d289682ab68a9f6d379d0aae8eda3d27f45669fce3da6786a4"), 8080);

      for (uint64_t i = 0; i < 64; i++) newBlock.appendTx(tx);

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
          validatorAddress, data, 8080, nHeight + 1, txValidatorPrivKey
        ); // nHeight here is wrong on purpose, +1 so it can throw
      }

      // Create 8 TxValidator transactions with type 0x6fc5a2d6 (random seed)
      for (const auto &seed : randomSeeds) {
        std::string data = Hex::toBytes("0x6fc5a2d6") + seed.get();
        txValidators.emplace_back(
          validatorAddress, data, 8080, nHeight + 1, txValidatorPrivKey
        ); // nHeight here is wrong on purpose, +1 so it can throw
      }

      // Append transactions to block.
      for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);

      // Sign block with block validator private key.
      newBlock.finalize(blockValidatorPrivKey, timestamp+1);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      try { Block b(str, 8080); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid tx merkle root") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("f8ca8798a75ba3d89ae187d95d7e1944fa07842dd8416e9413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e823f44a0ac5a548f098186dc197b2416cb7188bfe4955b7b554098f5986ad92c3e948f3ea01513ee3cd51b18d289682ab68a9f6d379d0aae8eda3d27f45669fce3da6786a4"), 8080);

      for (uint64_t i = 0; i < 64; i++) newBlock.appendTx(tx);

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
          validatorAddress, data, 8080, nHeight, txValidatorPrivKey
        );
      }

      // Create 8 TxValidator transactions with type 0x6fc5a2d6 (random seed)
      for (const auto &seed : randomSeeds) {
        std::string data = Hex::toBytes("0x6fc5a2d6") + seed.get();
        txValidators.emplace_back(
          validatorAddress, data, 8080, nHeight, txValidatorPrivKey
        );
      }

      // Append transactions to block.
      for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);

      // Sign block with block validator private key.
      newBlock.finalize(blockValidatorPrivKey, timestamp+1);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      str.replace(161, 2, "\xb0\x0b"); // Replace 2 bytes on tx merkle root to make it invalid and throw
      try { Block b(str, 8080); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid validator merkle root") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("f8ca8798a75ba3d89ae187d95d7e1944fa07842dd8416e9413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e823f44a0ac5a548f098186dc197b2416cb7188bfe4955b7b554098f5986ad92c3e948f3ea01513ee3cd51b18d289682ab68a9f6d379d0aae8eda3d27f45669fce3da6786a4"), 8080);

      for (uint64_t i = 0; i < 64; i++) newBlock.appendTx(tx);

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
          validatorAddress, data, 8080, nHeight, txValidatorPrivKey
        );
      }

      // Create 8 TxValidator transactions with type 0x6fc5a2d6 (random seed)
      for (const auto &seed : randomSeeds) {
        std::string data = Hex::toBytes("0x6fc5a2d6") + seed.get();
        txValidators.emplace_back(
          validatorAddress, data, 8080, nHeight, txValidatorPrivKey
        );
      }

      // Append transactions to block.
      for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);

      // Sign block with block validator private key.
      newBlock.finalize(blockValidatorPrivKey, timestamp+1);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      str.replace(129, 2, "\xb0\x0b"); // Replace 2 bytes on validator merkle root to make it invalid and throw
      try { Block b(str, 8080); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid block randomness") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("f8ca8798a75ba3d89ae187d95d7e1944fa07842dd8416e9413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e823f44a0ac5a548f098186dc197b2416cb7188bfe4955b7b554098f5986ad92c3e948f3ea01513ee3cd51b18d289682ab68a9f6d379d0aae8eda3d27f45669fce3da6786a4"), 8080);

      for (uint64_t i = 0; i < 64; i++) newBlock.appendTx(tx);

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
          validatorAddress, data, 8080, nHeight, txValidatorPrivKey
        );
      }

      // Create 8 TxValidator transactions with type 0x6fc5a2d6 (random seed)
      for (const auto &seed : randomSeeds) {
        std::string data = Hex::toBytes("0x6fc5a2d6") + seed.get();
        txValidators.emplace_back(
          validatorAddress, data, 8080, nHeight, txValidatorPrivKey
        );
      }

      // Append transactions to block.
      for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);

      // Sign block with block validator private key.
      newBlock.finalize(blockValidatorPrivKey, timestamp+1);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      str.replace(97, 2, "\xb0\x0b"); // Replace 2 bytes on block randomness to make it invalid and throw
      try { Block b(str, 8080); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid validator signature") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("f8ca8798a75ba3d89ae187d95d7e1944fa07842dd8416e9413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e823f44a0ac5a548f098186dc197b2416cb7188bfe4955b7b554098f5986ad92c3e948f3ea01513ee3cd51b18d289682ab68a9f6d379d0aae8eda3d27f45669fce3da6786a4"), 8080);

      for (uint64_t i = 0; i < 64; i++) newBlock.appendTx(tx);

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
          validatorAddress, data, 8080, nHeight, txValidatorPrivKey
        );
      }

      // Create 8 TxValidator transactions with type 0x6fc5a2d6 (random seed)
      for (const auto &seed : randomSeeds) {
        std::string data = Hex::toBytes("0x6fc5a2d6") + seed.get();
        txValidators.emplace_back(
          validatorAddress, data, 8080, nHeight, txValidatorPrivKey
        );
      }

      // Append transactions to block.
      for (const auto &txValidator : txValidators) newBlock.appendTxValidator(txValidator);

      // Sign block with block validator private key.
      newBlock.finalize(blockValidatorPrivKey, timestamp+1);

      // TODO: this doesn't seem to work, it doesn't throw
      bool catchedR = false;
      bool catchedS = false;
      bool catchedV = false;
      std::string strR = newBlock.serializeBlock();
      std::string strS = newBlock.serializeBlock();
      std::string strV = newBlock.serializeBlock();
      // Make signature R over Secp256k1::ecConst, making signature invalid.
      // Which equals to 0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141
      strR.replace(0, 16, "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"); // Replace 2 bytes in Validator sig to make it invalid and throw
      // Make signature S over Secp256k1::ecConst, making signature invalid
      strS.replace(32, 16, "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff");
      // Make signature V over 1, making signature invalid
      strV.replace(64,1, "\xff");
      try { Block b(strR, 8080); } catch (std::exception& e) { catchedR = true; }
      try { Block b(strS, 8080); } catch (std::exception& e) { catchedS = true; }
      try { Block b(strV, 8080); } catch (std::exception& e) { catchedV = true; }
      REQUIRE(catchedR == true);
      REQUIRE(catchedS == true);
      REQUIRE(catchedV == true);
    }

    SECTION("Finalizing and appending tx/Validator tx on already finalized block") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6"));
      uint64_t timestamp = 1678400201858;
      uint64_t nHeight = 92137812;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock txB(Hex::toBytes("f8ca8798a75ba3d89ae187d95d7e1944fa07842dd8416e9413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e823f44a0ac5a548f098186dc197b2416cb7188bfe4955b7b554098f5986ad92c3e948f3ea01513ee3cd51b18d289682ab68a9f6d379d0aae8eda3d27f45669fce3da6786a4"), 8080);
      TxValidator txV(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"), 8080);

      newBlock.finalize(validatorPrivKey, timestamp+1);
      REQUIRE(!newBlock.finalize(validatorPrivKey, timestamp+1));
      REQUIRE(!newBlock.appendTx(txB));
      REQUIRE(!newBlock.appendTxValidator(txV));
    }
  }
}

