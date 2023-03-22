#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/core/rdpos.h"
#include "../../src/core/storage.h"
#include "../../src/core/state.h"
#include "../../src/utils/db.h"
#include "../../src/net/p2p/p2pmanagernormal.h"
#include "../../src/net/p2p/p2pmanagerdiscovery.h"

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
                std::unique_ptr<P2P::ManagerNormal>& p2p,
                std::unique_ptr<rdPoS>& rdpos,
                std::unique_ptr<State>& state,
                PrivKey validatorKey,
                uint64_t serverPort,
                bool clearDb = true,
                std::string dbPrefix = "") {
  std::string dbName = dbPrefix + "stateTests";
  if (clearDb) {
    if (std::filesystem::exists(dbName)) {
      std::filesystem::remove_all(dbName);
    }
  }
  db = std::make_unique<DB>(dbName);
  if (clearDb) {
    Block genesis(Hash(Utils::uint256ToBytes(0)), 1678887537000000, 0);

    // Genesis Keys:
    // Private: 0xe89ef6409c467285bcae9f80ab1cfeb348  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),7cfe61ab28fb7d36443e1daa0c2867
    // Address: 0x00dead00665771855a34155f5e7405489df2c3c6
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")),1678887538000000);
    db->put("latest", genesis.serializeBlock(), DBPrefix::blocks);
    db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    db->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);

    // Populate rdPoS DB with unique validators, not default.
    for (uint64_t i = 0; i < validatorPrivKeys.size(); ++i) {
      db->put(Utils::uint64ToBytes(i), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[i]))).get(),
              DBPrefix::validators);
    }
    // Populate State DB with one address.
    /// Initialize with 0x00dead00665771855a34155f5e7405489df2c3c6 with nonce 0.
    Address dev1(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true);
    /// See ~State for encoding
    uint256_t desiredBalance("1000000000000000000000");
    std::string value = Utils::uintToBytes(Utils::bytesRequired(desiredBalance)) + Utils::uintToBytes(desiredBalance) + '\x00';
    db->put(dev1.get(), value, DBPrefix::nativeAccounts);
  }

  storage = std::make_unique<Storage>(db);
  p2p = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), serverPort, rdpos);
  rdpos = std::make_unique<rdPoS>(db, 8080, storage, p2p, validatorKey);
  state = std::make_unique<State>(db, storage, rdpos, p2p);
}

namespace TState {
  TEST_CASE("State Class", "[core][state]") {
    SECTION("State Class Constructor/Destructor", "[state]") {
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, true, "stateConstructorTest");
        REQUIRE(state->getNativeBalance(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true)) ==
                uint256_t("1000000000000000000000"));
      }
      // Wait a little until everyone has been destructed.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      //// Check if opening the state loads successfully from DB.
      initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, false, "stateConstructorTest");
      REQUIRE(state->getNativeBalance(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true)) ==
              uint256_t("1000000000000000000000"));
      REQUIRE(state->getNativeNonce(Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true)) == 0);
    }

    SECTION("State Class addBalance to random Addresses") {
      std::vector<std::pair<Address,uint256_t>> addresses;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, true, "stateAddBalanceTest");

        for (uint64_t i = 0; i < 1024; ++i) {
          std::pair<Address,uint256_t> randomAddress = std::make_pair(Address(Utils::randBytes(20), true), uint256_t("1000000000000000000"));
          state->addBalance(randomAddress.first);
          addresses.push_back(randomAddress);
        }

        for (const auto& [address, expectedBalance] : addresses) {
          REQUIRE(state->getNativeBalance(address) == expectedBalance);
          REQUIRE(state->getNativeNonce(address) == 0);
        }
      }
      // Wait until destructors are called.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      // Load everything back from DB.
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, false, "stateAddBalanceTest");
      for (const auto& [address, expectedBalance] : addresses) {
        REQUIRE(state->getNativeBalance(address) == expectedBalance);
        REQUIRE(state->getNativeNonce(address) == 0);
      }
    }
    SECTION("Test Simple block on State (No Transactions only rdPoS") {
      std::unique_ptr<Block> latestBlock = nullptr;
      {
        std::unique_ptr<DB> db;
        std::unique_ptr<Storage> storage;
        std::unique_ptr<P2P::ManagerNormal> p2p;
        std::unique_ptr<rdPoS> rdpos;
        std::unique_ptr<State> state;
        initialize(db, storage, p2p, rdpos, state, validatorPrivKeys[0], 8080, true, "stateAddBalanceTest");


      }
    }
  }
}