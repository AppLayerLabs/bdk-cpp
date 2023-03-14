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
  randomGen(Hash()) {
    if (validatorKey) isValidator = true;
    // Load information from DB.
    // DB is stored as following
    // DBPrefix::validators -> validators mapping (addresses)
    // DBPrefix::rdPoS -> misc: used for randomness currently.
    auto validators = db->get(DBPrefix::validators, "");
    

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