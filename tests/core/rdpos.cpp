#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/rdpos.h"
#include "../../src/core/storage.h"
#include "../../src/utils/db.h"
#include "../../src/net/p2p/p2pmanagernormal.h"

#include <filesystem>
#include <utility>

const std::vector<Hash> validatorPrivKeys {
  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

// We initialize the blockchain database
// To make sure that if the genesis is changed within the main source code
// The tests will still work, as tests uses own genesis block.
void initialize(std::unique_ptr<DB>& db, 
                std::unique_ptr<Storage>& storage, 
                std::unique_ptr<P2P::ManagerBase>& p2p, 
                PrivKey& validatorKey, 
                std::unique_ptr<rdPoS>& rdpos,
                uint64_t serverPort,
                bool clearDb = true) {

  if (clearDb) {
    if (std::filesystem::exists("rdPoStests")) {
      std::filesystem::remove_all("rdPoStests");
    }
  }
  db = std::make_unique<DB>("rdPoStests");
  if (clearDb) {
    Block genesis(Hash(Utils::uint256ToBytes(0)), 1678887537000000, 0);
                                                  
    // Genesis Keys:
    // Private: 0xe89ef6409c467285bcae9f80ab1cfeb348  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),7cfe61ab28fb7d36443e1daa0c2867
    // Address: 0x00dead00665771855a34155f5e7405489df2c3c6
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")));
    db->put("latest", genesis.serializeBlock(), DBPrefix::blocks);
    db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    db->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);

    // Populate rdPoS DB with unique validators, not default.
    // PrivateKey: 0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8
    db->put(Utils::uint64ToBytes(0), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[0]))).get(), DBPrefix::validators);
    // PrivateKey: b254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66
    db->put(Utils::uint64ToBytes(1), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[1]))).get(), DBPrefix::validators);
    // PrivateKey: 8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da
    db->put(Utils::uint64ToBytes(2), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[2]))).get(), DBPrefix::validators);
    // PrivateKey: 9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745
    db->put(Utils::uint64ToBytes(3), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[3]))).get(), DBPrefix::validators);
    // PrivateKey: 0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739
    db->put(Utils::uint64ToBytes(4), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[4]))).get(), DBPrefix::validators);
    // PrivateKey: a69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7
    db->put(Utils::uint64ToBytes(5), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[5]))).get(), DBPrefix::validators);
    // PrivateKey: d9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5
    db->put(Utils::uint64ToBytes(6), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[6]))).get(), DBPrefix::validators);
    // PrivateKey: 426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f
    db->put(Utils::uint64ToBytes(7), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[7]))).get(), DBPrefix::validators);
  }

  storage = std::make_unique<Storage>(db);
  p2p = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), serverPort);
  validatorKey = PrivKey::random();
  rdpos = std::make_unique<rdPoS>(db, 8080, storage, p2p, validatorKey);
}

/*

    rdPoS(const std::unique_ptr<DB>& db, 
          const uint64_t& chainId,
          const std::unique_ptr<Storage>& storage,
          const std::unique_ptr<P2P::ManagerBase>& p2p,
          const PrivKey& validatorKey = PrivKey());

*/

namespace TRdPoS {
  TEST_CASE("rdPoS Class", "[core][rdpos]") {
    SECTION("rdPoS class Startup") {
      std::set<Validator> validatorsList;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerBase> p2p;
        PrivKey validatorKey;
        std::unique_ptr<rdPoS> rdpos;
        initialize(db, storage, p2p, validatorKey, rdpos, 8080);

        auto validators = rdpos->getValidators();
        REQUIRE(rdpos->getValidators().size() == 8);
        REQUIRE(validators.contains(Address(Hex::toBytes("1531bfdf7d48555a0034e4647fa46d5a04c002c3"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("e3dff2cc3f367df7d0254c834a0c177064d7c7f5"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("24e10d8ebe80abd3d3fddd89a26f08f3888d1380"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("b5f7152a2589c6cc2535c5facedfc853194d60a5"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("098ff62812043f5106db718e5c4349111de3b6b4"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("50d2ce9815e0e2354de7834f6fdd4d6946442a24"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("7c2b2a0a75e10b49e652d99bba8afee3a6bc78dd"), true)));
        REQUIRE(validators.contains(Address(Hex::toBytes("6e67067edc1b4837b67c0b1def689eddee257521"), true)));
        REQUIRE(rdpos->getBestRandomSeed() == Hash()); // Genesis blocks randomness is 0.

        auto randomList = rdpos->getRandomList();
        validatorsList = rdpos->getValidators();
        REQUIRE(randomList.size() == 8);
        for (const auto& i : randomList) {
          REQUIRE(validatorsList.contains(i));
        }

        // Check ordering of random list. deterministic.
        REQUIRE(randomList[0] == Address(Hex::toBytes("50d2ce9815e0e2354de7834f6fdd4d6946442a24"), true));
        REQUIRE(randomList[1] == Address(Hex::toBytes("6e67067edc1b4837b67c0b1def689eddee257521"), true));
        REQUIRE(randomList[2] == Address(Hex::toBytes("24e10d8ebe80abd3d3fddd89a26f08f3888d1380"), true));
        REQUIRE(randomList[3] == Address(Hex::toBytes("7c2b2a0a75e10b49e652d99bba8afee3a6bc78dd"), true));
        REQUIRE(randomList[4] == Address(Hex::toBytes("1531bfdf7d48555a0034e4647fa46d5a04c002c3"), true));
        REQUIRE(randomList[5] == Address(Hex::toBytes("b5f7152a2589c6cc2535c5facedfc853194d60a5"), true));
        REQUIRE(randomList[6] == Address(Hex::toBytes("e3dff2cc3f367df7d0254c834a0c177064d7c7f5"), true));
        REQUIRE(randomList[7] == Address(Hex::toBytes("098ff62812043f5106db718e5c4349111de3b6b4"), true));
      }
      
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerBase> p2p;
      PrivKey validatorKey;
      std::unique_ptr<rdPoS> rdpos;
      initialize(db, storage, p2p, validatorKey, rdpos, 8080, false);

      auto validators = rdpos->getValidators();
      REQUIRE(validators == validatorsList);
    }

    SECTION ("rdPoS validateBlock(), one block from genesis") {
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerBase> p2p;
      PrivKey validatorKey;
      std::unique_ptr<rdPoS> rdpos;
      initialize(db, storage, p2p, validatorKey, rdpos, 8080);

      auto validators = rdpos->getValidators();
      auto randomList = rdpos->getRandomList();

      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the validators in the order of the random list, limited to rdPoS::minValidators.
      orderedPrivKeys.reserve(4);
      for (const auto& privKey : validatorPrivKeys) {
        if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
          blockSignerPrivKey = privKey;
          break;
        }
      }

      for (uint64_t i = 1; i < rdPoS::minValidators + 1; i++) {
        for (const auto& privKey : validatorPrivKeys) {
          if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[i]) {
            orderedPrivKeys.push_back(privKey);
            break;
          }
        }
      }

      // By now we should have randomList[0] privKey in blockSignerPrivKey and the rest in orderedPrivKeys, ordered by the random list.
      // We can proceed with creating the block, transactions have to be **ordered** by the random list.

      // Create a block with 8 TxValidator transactions, 2 for each validator, in order (randomHash and random)
      uint64_t newBlocknHeight = storage->latest()->getNHeight() + 1;
      uint64_t newBlockTimestamp = storage->latest()->getTimestamp() + 100000;
      Hash newBlockPrevHash = storage->latest()->hash();
      Block block(newBlockPrevHash, newBlockTimestamp, newBlocknHeight);
      std::vector<TxValidator> randomHashTxs;
      std::vector<TxValidator> randomTxs;

      std::vector<Hash> randomSeeds(orderedPrivKeys.size(), Hash::random());
      for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
        std::string hashTxData = Hex::toBytes("0xcfffe746") + Utils::sha3(randomSeeds[i].get()).get();
        std::string randomTxData = Hex::toBytes("0x6fc5a2d6") + randomSeeds[i].get();
        randomHashTxs.emplace_back(
          validatorAddress,
          hashTxData,
          8080,
          newBlocknHeight,
          orderedPrivKeys[i]
        );
        randomTxs.emplace_back(
          validatorAddress,
          randomTxData,
          8080,
          newBlocknHeight,
          orderedPrivKeys[i]
        );
      }
      // Append the transactions to the block.
      for (const auto& tx : randomHashTxs) {
        rdpos->addValidatorTx(tx);
        block.appendTxValidator(tx);
      }
      for (const auto& tx : randomTxs) {
        rdpos->addValidatorTx(tx);
        block.appendTxValidator(tx);
      }

      // Check rdPoS mempool.
      auto rdPoSmempool = rdpos->getMempool();
      REQUIRE(rdpos->getMempool().size() == 8);
      for (const auto& tx : randomHashTxs) {
        REQUIRE(rdPoSmempool.contains(tx.hash()));
      }
      for (const auto& tx : randomTxs) {
        REQUIRE(rdPoSmempool.contains(tx.hash()));
      }
      
      // Finalize the block
      block.finalize(blockSignerPrivKey);

      // Validate the block on rdPoS
      REQUIRE(rdpos->validateBlock(block));
    }
  }
}
