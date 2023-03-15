#include "rdpos.h"
#include "storage.h"

rdPoS::rdPoS(const std::unique_ptr<DB>& db, 
  const uint64_t& chainId,
  const std::unique_ptr<Storage>& storage,
  const std::unique_ptr<P2P::ManagerBase>& p2p,
  const PrivKey& validatorKey) :

  Contract(ProtocolContractAddresses.at("rdPoS"), chainId, db),
  worker(std::make_unique<rdPoSWorker>(*this)),
  storage(storage),
  p2p(p2p),
  validatorKey(std::make_optional<PrivKey>(validatorKey)),
  randomGen(Hash()) 
{
  std::unique_lock lock(this->mutex);
  Utils::logToDebug(Log::rdPoS, __func__, "Initializing rdPoS.");
  if (validatorKey) isValidator = true;
  // Initialize blockchain.
  initializeBlockchain();
  // Load information from DB.
  // DB is stored as following
  // DBPrefix::validators -> validators mapping (addresses)
  // DBPrefix::rdPoS -> misc: used for randomness currently.
  // DB Order doesn't matter, Validators are stored in a set (sorted by default)
  auto validatorsDb = db->getBatch(DBPrefix::validators);
  if (validatorsDb.size() == 0) {
    // No validators in DB, this should have been initialized by Storage.
    Utils::logToDebug(Log::rdPoS, __func__, "No validators in DB, cannot proceed.");
    throw std::runtime_error("No validators in DB.");
  }
  Utils::logToDebug(Log::rdPoS, __func__, "Found " + std::to_string(validatorsDb.size()) + " validators in DB");
  // TODO: check if no index is missing from DB.
  for (const auto& validator : validatorsDb) {
    this->validators.insert(Validator(Address(validator.value, true)));
  }

  // Load latest randomness from DB.
  this->bestRandomSeed = storage->latest()->getBlockRandomness();

  // Populate the random list
  randomGen.setSeed(bestRandomSeed);
  randomList = std::vector<Validator>(this->validators.begin(), this->validators.end());

  randomGen.shuffle(randomList);
}

rdPoS::~rdPoS() {
  std::unique_lock lock(this->mutex);
  DBBatch validatorsBatch;
  Utils::logToDebug(Log::rdPoS, __func__, "Descontructing rdPoS, saving to DB.");
  // Save validators to DB.
  uint64_t index = 0;
  for (const auto &validator : validators) {
    validatorsBatch.puts.emplace_back(Utils::uint64ToBytes(index), validator.get());
    ++index;
  }
}

void rdPoS::initializeBlockchain() {
  auto validatorsDb = db->getBatch(DBPrefix::validators);
  if (validatorsDb.size() == 0) {
    Utils::logToDebug(Log::rdPoS,__func__, "No validators in DB, initializing.");
    // TODO: CHANGE THIS ON PUBLIC!!! THOSE PRIVATE KEYS SHOULD ONLY BE USED FOR LOCAL TESTING
    // 0xba5e6e9dd9cbd263969b94ee385d885c2d303dfc181db2a09f6bf19a7ba26759
    this->db->put(Utils::uint64ToBytes(0), Address(Hex::toBytes("0x7588b0f553d1910266089c58822e1120db47e572"), true).get(), DBPrefix::validators);
    // 0xfd84d99aa18b474bf383e10925d82194f1b0ca268e7a339032679d6e3a201ad4
    this->db->put(Utils::uint64ToBytes(1), Address(Hex::toBytes("0xcabf34a268847a610287709d841e5cd590cc5c00"), true).get(), DBPrefix::validators);
    // 0x66ce71abe0b8acd92cfd3965d6f9d80122aed9b0e9bdd3dbe018230bafde5751
    this->db->put(Utils::uint64ToBytes(2), Address(Hex::toBytes("0x5fb516dc2cfc1288e689ed377a9eebe2216cf1e3"), true).get(), DBPrefix::validators);
    // 0x856aeb3b9c20a80d1520a2406875f405d336e09475f43c478eb4f0dafb765fe7
    this->db->put(Utils::uint64ToBytes(3), Address(Hex::toBytes("0x795083c42583842774febc21abb6df09e784fce5"), true).get(), DBPrefix::validators);
    // 0x81f288dd776f4edfe256d34af1f7d719f511559f19115af3e3d692e741faadc6
    this->db->put(Utils::uint64ToBytes(4), Address(Hex::toBytes("0xbec7b74f70c151707a0bfb20fe3767c6e65499e0"), true).get(), DBPrefix::validators);
  }
}

Hash rdPoS::parseTxSeedList(const std::vector<TxValidator>& txs) {
  std::string seed;
  if (txs.size() == 0) return Hash();
  for (const TxValidator& tx : txs) {
    if (tx.getData().substr(0,4) == Hex::toBytes("0x6fc5a2d6")) {
      seed += tx.getData().substr(4,32);
    }
  }
  return Utils::sha3(seed);
}