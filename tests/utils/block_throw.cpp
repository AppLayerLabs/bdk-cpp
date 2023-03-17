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
      try { Block b(str); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid Validator tx height") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"));

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
      newBlock.finalize(blockValidatorPrivKey);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      try { Block b(str); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid tx merkle root") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"));

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
      newBlock.finalize(blockValidatorPrivKey);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      str.replace(161, 2, "\xb0\x0b"); // Replace 2 bytes on tx merkle root to make it invalid and throw
      try { Block b(str); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid validator merkle root") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"));

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
      newBlock.finalize(blockValidatorPrivKey);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      str.replace(129, 2, "\xb0\x0b"); // Replace 2 bytes on validator merkle root to make it invalid and throw
      try { Block b(str); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid block randomness") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"));

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
      newBlock.finalize(blockValidatorPrivKey);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      str.replace(97, 2, "\xb0\x0b"); // Replace 2 bytes on block randomness to make it invalid and throw
      try { Block b(str); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid validator signature") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      uint64_t timestamp = 1678464099412509;
      uint64_t nHeight = 331653115;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock tx(Hex::toBytes("0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"));

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
      newBlock.finalize(blockValidatorPrivKey);

      bool catched = false;
      std::string str = newBlock.serializeBlock();
      // TODO: this doesn't seem to work, it doesn't throw
      Block b(str);
      std::cout << b.getValidatorSig().hex().get() << std::endl;
      str.replace(0, 2, "\xb0\x0b"); // Replace 2 bytes in Validator sig to make it invalid and throw
      Block b2(str);
      std::cout << b2.getValidatorSig().hex().get() << std::endl;
      std::cout << Secp256k1::verifySig(
        b.getValidatorSig().r(), b.getValidatorSig().s(), b.getValidatorSig().v()
      ) << std::endl;
      std::cout << Secp256k1::verifySig(
        b2.getValidatorSig().r(), b2.getValidatorSig().s(), b2.getValidatorSig().v()
      ) << std::endl;
      std::cout << b.getValidatorSig().r() << std::endl;
      std::cout << b.getValidatorSig().s() << std::endl;
      std::cout << b.getValidatorSig().v() << std::endl;
      std::cout << b2.getValidatorSig().r() << std::endl;
      std::cout << b2.getValidatorSig().s() << std::endl;
      std::cout << b2.getValidatorSig().v() << std::endl;
      try { Block b(str); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Finalizing and appending tx/Validator tx on already finalized block") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6"));
      uint64_t timestamp = 1678400201858;
      uint64_t nHeight = 92137812;
      Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);

      TxBlock txB(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"));
      TxValidator txV(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"));

      newBlock.finalize(validatorPrivKey);
      REQUIRE(!newBlock.finalize(validatorPrivKey));
      REQUIRE(!newBlock.appendTx(txB));
      REQUIRE(!newBlock.appendTxValidator(txV));
    }
  }
}

