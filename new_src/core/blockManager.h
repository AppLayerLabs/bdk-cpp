#ifndef BLOCKMANAGER_H
#define BLOCKMANAGER_H

#include <mutex>

#include "block.h"
#include "blockChain.h"
#include "state.h"
#include "../contract/contract.h"
#include "../net/P2PManager.h"
#include "../net/grpcclient.h"
#include "../utils/db.h"
#include "../utils/hash.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"

class Validator {
  private:
    Address add;
  public:
    Validator(const Address& add) : add(add) {}
    Validator(const Address&& add) : add(std::move(add)) {}
    Validator(const Validator& other) : add(other.add) {}
    Validator(Validator&& other) noexcept : add(std::move(other.add)) {}
    const Address& get() { return this->add; }
    const std::string hex() { return this->add.hex(); }
    bool operator==(const Validator& other) { return add == other.add; }
    bool operator!=(const Validator& other) { return add != other.add; }
};

class BlockManager : public Contract {
  private:
    std::vector<Validator> validatorList;
    std::vector<std::reference_wrapper<Validator>> randomList;
    std::unordered_map<Hash, Tx, SafeHash> validatorMempool;
    PrivKey validatorPrivKey;
    bool isValidator = false;
    bool isValidatorThreadRunning = false;
    std::mutex lock;
    RandomGen gen;
    const std::shared_ptr<DB> db;
    const std::shared_ptr<BlockChain> chain;
    const std::shared_ptr<P2PManager> p2p;
    const std::shared_ptr<gRPCClient> grpcClient;
    void loadFromDB();
    bool shuffle();
    void validatorLoop();
  public:
    enum TxType { addValidator, removeValidator, randomHash, randomSeed };
    static const uint32_t minValidators = 4;
    BlockManager(
      const std::shared_ptr<DB>& db, const std::shared_ptr<BlockChain>& chain,
      const std::shared_ptr<P2PManager>& p2p, const std::shared_ptr<gRPCClient>& grpcClient,
      const Address& add, const Address& owner, const PrivKey& privKey = ""
    );
    bool isValidator(const Validator& val);
    void saveToDB();
    bool validateBlock(const std::shared_ptr<const Block>& block);
    Hash processBlock(const std::shared_ptr<const Block>& block);
    void addValidatorTx(const Tx& tx);
    void finalizeBlock(const std::shared_ptr<Block> block);
    static Hash parseTxSeedList(const std::unordered_map<uint64_t, Tx, SafeHash> txs);
    static TxType getTxType(const Tx& tx);
    void startValidatorThread();
    std::unordered_map<Hash, Tx, SafeHash> getMempoolCopy();
    std::vector<std::reference_wrapper<Validator>> getRandomListCopy();
};

#endif  // BLOCKMANAGER_H
