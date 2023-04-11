#include "net/p2p/p2pmanagerdiscovery.h"
#include "net/p2p/p2pmanagernormal.h"
#include "core/rdpos.h"
#include "core/storage.h"
#include "core/state.h"
#include "utils/options.h"
#include "iostream"


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
void initializeFullChain(std::unique_ptr<DB>& db,
                std::unique_ptr<Storage>& storage,
                std::unique_ptr<P2P::ManagerNormal>& p2p,
                std::unique_ptr<rdPoS>& rdpos,
                std::unique_ptr<State>& state,
                std::unique_ptr<Options>& options,
                PrivKey validatorKey,
                uint64_t serverPort,
                bool clearDb,
                std::string folderName) {
  std::string dbName = folderName + "/db";
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
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")), 1678887538000000);
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
  std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
  if (!validatorKey) {
    options = std::make_unique<Options>(
      folderName,
      "OrbiterSDK/cpp/linux_x86-64/0.0.1",
      1,
      8080,
      serverPort,
      9999,
      discoveryNodes
    );
  } else {
    options = std::make_unique<Options>(
      folderName,
      "OrbiterSDK/cpp/linux_x86-64/0.0.1",
      1,
      8080,
      serverPort,
      9999,
      discoveryNodes,
      validatorKey
    );
  }

  storage = std::make_unique<Storage>(db, options);
  p2p = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), rdpos, options, storage, state);
  rdpos = std::make_unique<rdPoS>(db, storage, p2p, options, state);
  state = std::make_unique<State>(db, storage, rdpos, p2p);
}



/**
* This executable contains a discovery node for the given Options default chain
*/
int main() {
  // Local binary path + /blockchain
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/discoveryNode");
  const std::unique_ptr<Options> options(std::make_unique<Options>(Options::fromFile(blockchainPath)));

  // Initialize 1 Discovery node and 100 Normal nodes
  std::cout << "Initializing Discovery..." << std::endl;
  P2P::ManagerDiscovery p2p(boost::asio::ip::address::from_string("127.0.0.1"), options);
  std::cout << "Initializing Normals..." << std::endl;
  std::vector<std::unique_ptr<DB>> dbVec;
  std::vector<std::unique_ptr<Storage>> storageVec;
  std::vector<std::unique_ptr<P2P::ManagerNormal>> p2pVec;
  std::vector<std::unique_ptr<rdPoS>> rdposVec;
  std::vector<std::unique_ptr<State>> stateVec;
  std::vector<std::unique_ptr<Options>> optsVec;
  for (int i = 0; i < 100; i++) {
    std::unique_ptr<DB> db;
    std::unique_ptr<Storage> storage;
    std::unique_ptr<P2P::ManagerNormal> p2p;
    std::unique_ptr<rdPoS> rdpos;
    std::unique_ptr<State> state;
    std::unique_ptr<Options> options;
    initializeFullChain(db, storage, p2p, rdpos, state, options,
      PrivKey(), 8080 + i, true, "p2pNormal" + std::to_string(i)
    );
    dbVec.emplace_back(std::move(db));
    storageVec.emplace_back(std::move(storage));
    p2pVec.emplace_back(std::move(p2p));
    rdposVec.emplace_back(std::move(rdpos));
    stateVec.emplace_back(std::move(state));
    optsVec.emplace_back(std::move(options));
  }

  // Light them all up
  std::cout << "Starting P2P Discovery Server..." << std::endl;
  p2p.startServer();
  std::cout << "Starting P2P Normal Servers..." << std::endl;
  for (int i = 0; i < 100; i++) {
    p2pVec[i]->startServer();
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "Connecting all Normals to Discovery..." << std::endl;
  for (int i = 0; i < 100; i++) {
    p2pVec[i]->connectToServer("127.0.0.1", options->getWsPort());
  }
  std::cout << "Starting Discovery for Discovery..." << std::endl;
  p2p.startDiscovery();
  std::cout << "Starting Discovery for Normals..." << std::endl;
  for (int i = 0; i < 100; i++) {
    p2pVec[i]->startDiscovery();
  }

  // Wait 15-20s and tell everyone to stop
  std::cout << "Waiting 15s..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(15));
  std::cout << "Stopping Discovery..." << std::endl;
  p2p.stop();
  std::cout << "Stopping Normals..." << std::endl;
  for (int i = 0; i < 100; i++) {
    p2pVec[i]->stop();
  }
  std::cout << "I slep forever now" << std::endl;

  // Sleep Forever
  std::this_thread::sleep_for(std::chrono::hours(1000000));
}

