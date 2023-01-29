#include "state.h"

bool State::saveToDB() {
  this->stateLock.lock();
  DBBatch accBatch;
  for (auto& acc : this->nativeAccounts) {
    accBatch.puts.emplace_back(acc.first.get(),
      Utils::uint256ToBytes(acc.second.balance)
      + Utils::uint32ToBytes(acc.second.nonce)
    );
  }
  bool ret = this->db->putBatch(accBatch, DBPrefix::nativeAccounts);
  this->stateLock.unlock();
  return ret;
}

bool State::loadFromDB() {
  this->stateLock.lock();
  std::vector<DBEntry> accs = this->db->getBatch(DBPrefix::nativeAccounts);
  if (accs.empty()) {
    Address dev("0x21B782f9BF82418A42d034517CB6Bf00b4C17612", true); // Ita's address
    Address dev2("0xb3Dc9ed7f450d188c9B5a44f679a1dDBb4Cbd6D2", true); // Supra's address
    Address dev3("0x12e7742c063Dff92dA0439430DFe8A05ce0d297e", true); // Ita's office
    Address dev4("0xaE33707325C17CD37331278ccb74d2Ba9bFa6c92", true); // Ita's laptop
    this->db->put(dev.get(),
      Utils::uint256ToBytes(uint256_t("100000000000000000000"))
      + Utils::uint32ToBytes(0), DBPrefix::nativeAccounts
    );
    Utils::logToDebug(Log::state, __func__, "Added balance to " + dev.hex());
    this->db->put(dev2.get(),
      Utils::uint256ToBytes(uint256_t("100000000000000000000"))
      + Utils::uint32ToBytes(0), DBPrefix::nativeAccounts
    );
    Utils::logToDebug(Log::state, __func__, "Added balance to " + dev2.hex());
    this->db->put(dev3.get(),
      Utils::uint256ToBytes(uint256_t("100000000000000000000"))
      + Utils::uint32ToBytes(0), DBPrefix::nativeAccounts
    );
    Utils::logToDebug(Log::state, __func__, "Added balance to " + dev3.hex());
    this->db->put(dev4.get(),
      Utils::uint256ToBytes(uint256_t("100000000000000000000"))
      + Utils::uint32ToBytes(0), DBPrefix::nativeAccounts
    );
    Utils::logToDebug(Log::state, __func__, "Added balance to " + dev4.hex());
    accs = this->db->getBatch(DBPrefix::nativeAccounts);
  }
  for (const DBEntry acc : accs) {
    Address address(acc.key, false);
    this->nativeAccounts[address].balance = Utils::bytesToUint256(acc.value.substr(0,32));
    this->nativeAccounts[address].nonce = Utils::bytesToUint32(acc.value.substr(32,4));
  }
  this->stateLock.unlock();
  return true;
}

bool State::processNewTx(const TxBlock& tx) {
  bool isContractCall = false;
  Utils::logToDebug(Log::state, __func__, "Processing new tx from: "
    + tx.getFrom().hex() + " with a value of " + boost::lexical_cast<std::string>(tx.getValue())
  );
  // Remove tx from mempool if found there
  if (this->mempool.count(tx.hash()) != 0) this->mempool.erase(tx.hash());

  // Update balances and nonce.
  this->nativeAccounts[tx.getFrom()].balance -= tx.getValue();
  this->nativeAccounts[tx.getFrom()].balance -= uint256_t(tx.getGasPrice() * tx.getGas());
  this->nativeAccounts[tx.getTo()].balance += tx.getValue();
  this->nativeAccounts[tx.getFrom()].nonce++;

  // TODO: Handle contract calls.
  return true;
}

uint256_t State::getNativeBalance(const Address& add) {
  this->stateLock.lock();
  uint256_t ret = this->nativeAccount[add].balance;
  this->stateLock.unlock();
  return ret;
};

uint256_t State::getNativeNonce(const Address& add) {
  this->stateLock.lock();
  uint256_t ret = this->nativeAccount[add].nonce;
  this->stateLock.unlock();
  return ret;
};

bool State::validateNewBlock(Block& block) {
  // Check block header, previous hash, validation and transactions within.
  // The block will be rejected if invalid transactions are included in it
  // (e.g. invalid signatures, account having min balance for min fees)
  const std::shared_ptr<const Block> best = this->storage->latest();
  if (best->getBlockHash() != block.getPrevBlockHash()) {
    Utils::logToDebug(Log::state, __func__, "Block previous hash does not match.");
    Utils::logToDebug(Log::state, __func__, "Block previous hash: " + block.prevBlockHash().get());
    Utils::logToDebug(Log::state, __func__, "Best block hash: " + best->getBlockHash().get());
    return false;
  }

  if (block.getNHeight() != (best->getNHeight() + 1)) {
    Utils::logToDebug(Log::state, __func__, "Block height does not match.");
    Utils::logToDebug(Log::state, __func__, "Block height: " + std::to_string(block.nHeight()));
    Utils::logToDebug(Log::state, __func__, "Best block height: " + std::to_string(best->nHeight()));
    return false;
  }

  if (!this->rdpos->validateBlock(block)) {
    Utils::logToDebug(Log::state, __func__, "Block validation failed: Validators do not match");
    return false;
  }

  for (const auto &tx : block.getTxs()) {
    if (!this->validateTransactionForBlock(tx.second)) {
      Utils::logToDebug(Log::state, __func__, "Block rejected due to invalid tx");
      return false;
    }
  }

  Utils::logToDebug(Log::state, __func__,
    "Validated block " + Utils::bytesToHex(block->getBlockHash().get())
    + " at height " + boost::lexical_cast<std::string>(block->getNHeight())
  );
  return true;
}

void State::processNewBlock(Block&& block) {
  // Check block previous hash
  this->stateLock.lock();
  Utils::logToDebug(Log::state, __func__, "Processing new block "
    + Utils::bytesToHex(block.getBlockHash().get())
    + " at height " + boost::lexical_cast<std::string>(block.getNHeight())
  );
  for (const auto& tx : block.getTxs()) this->processNewTx(tx.second);

  // Process block and append to chain
  this->rdpos->processBlock(block);
  this->storage->push_back(std::move(block));
  this->mempool.clear();
  this->stateLock.unlock();
}

const std::shared_ptr<const Block> State::createNewBlock() {
  Utils::logToDebug(Log::state, __func__, "Creating new block");
  Hash bestHash = this->snowmanVM->getPreferredBlockHash();
  if (bestHash.empty()) {
    Utils::logToDebug(Log::state, __func__, "No preferred block found");
    return nullptr;
  }
  Utils::logToDebug(Log::state, __func__,
    std::string("Got preference: ") + Utils::bytesToHex(bestHash.get())
  );
  std::shared_ptr<const Block> bestBlock = this->storage->getBlock(bestHash);
  if (bestBlock == nullptr) {
    Utils::LogPrint(Log::state, __func__, "Preferred block does not exist");
    return nullptr;
  }
  Utils::logToDebug(Log::state, __func__, "Got best block");

  std::shared_ptr<Block> newBestBlock = std::make_shared<Block>(
    bestBlock->getBlockHash(),
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count(),
    bestBlock->getNHeight() + 1
  );

  this->stateLock.lock();
  for (auto& tx : this->mempool) newBestBlock->appendTx(tx.second);
  this->stateLock.unlock();

  // Order things up, first 4 txs are randomHash, last 4 txs are random itself
  auto valMempool = this->rdpos->getMempoolCopy();
  auto valRandomList = this->rdpos->getRandomListCopy();
  std::vector<TxValidator> valTxs;
  for (const auto& tx : valMempool) {
    Utils::logToDebug(Log::state, __func__,
      std::string("TX: ") + tx.second.hash().hex()
      + ", FROM: " + tx.second.from().hex()
      + ", TYPE: " + std::to_string(this->rdpos->getTxType(tx.second))
    );
  }

  // Reorder validator transactions.
  // The mempool is an unordered map but it is required for the block
  // to have the validator transactions ordered.
  // In the current code we are ordering it like this:
  // First, append in order validator[1...4] (randomList) randomHash transactions
  // Then append in order validator[1...4] (randomList) randomSeed transactions
  while (valTxs.size() < this->rdpos->minValidators * 2) {
    for (const auto& tx : valMempool) {
      if (valTxs.size() < this->rdpos->minValidators) { // Index the randomHash
        if (
          tx.second.from() == valRandomList[valTxs.size() + 1].get().get() &&
          this->rdpos->getTxType(tx.second) == rdPoS::TxType::randomHash
        ) { // Skip [0] as it is us
          valTxs.push_back(tx.second);
          Utils::logToDebug(Log::state, __func__, "Indexing validator hash tx");
        }
      } else { // Index the randomSeed
        if (
          tx.second.from() == valRandomList[valTxs.size() - this->rdpos->minValidators + 1].get().get() &&
          this->rdpos->getTxType(tx.second) == rdPoS::TxType::randomSeed
        ) {
          valTxs.push_back(tx.second);
          Utils::logToDebug(Log::state, __func__, "Indexing validator seed tx");
        }
      }
      if (valTxs.size() == this->rdpos->minValidators * 2) break;
    }
  }

  // Append Validator txs, sign and finalize the block
  for (const auto& i : valTxs) newBestBlock->appendValidatorTx(i);
  this->rdpos->finalizeBlock(newBestBlock);
  Utils::logToDebug(Log::state, __func__,
    std::string("Block created, signature: ") + newBestBlock->signature().hex()
  );
  return newBestBlock;
}

bool State::validateTxForBlock(const TxBlock& tx) {
  // Txs are assumed to be always verified - see utils/tx.h for more details
  bool ret = true;
  this->stateLock.lock();
  if (!this->mempool.count(tx.hash())) { // Ignore if tx is already in mempool
    if (this->nativeAccounts.count(tx.getFrom()) == 0) {
      ret = false;  // No account = zero balance = can't pay fees
    } else {
      Account acc = this->nativeAccount.find(tx.getFrom())->second;
      if (acc.balance < tx.getValue() || acc.nonce != tx.getNonce()) {
        ret = false; // Insufficient balance or invalid nonce
      }
    }
  }
  this->stateLock.unlock();
  return ret;
}

const std::pair<int, string> State::validateTxForRPC(const TxBlock& tx) {
  // TODO: Handle error conditions to report at RPC level:
  // https://www.jsonrpc.org/specification#error_object
  // https://eips.ethereum.org/EIPS/eip-1474#error-codes
  int err = 0;
  std::string errMsg;

  this->stateLock.lock();
  if (this->mempool.count(tx.hash())) { // Not really considered a failure
    errMsg = "Transaction already exists in mempool";
  } else if (this->nativeAccounts.count(tx.getFrom()) == 0) { // No account = zero balance = can't pay fees
    err = -32003; errMsg = "Insufficient balance - required: "
      + boost::lexical_cast<std::string>(tx.getValue()) + ", available: 0";
  } else {
    Account acc = this->nativeAccounts.find(tx.getFrom())->second;
    if (acc.balance < tx.getValue()) {
      err = -32002; errMsg = "Insufficient balance - required: "
        + boost::lexical_cast<std::string>(tx.getValue()) + ", available: "
        + boost::lexical_cast<std::string>(acc.balance);
    } else if (acc.nonce != tx.getNonce()) {
      err = -32001; errMsg = "Invalid nonce";
    }
  }
  this->stateLock.unlock();

  if (err != 0) {
    errMsg.insert(0, "Tx rejected: ");
    Utils::logToDebug(Log::state, __func__, errMsg);
  } else {
    this->stateLock.lock();
    Hash txHash = tx.hash();
    this->mempool[txHash] = tx;
    this->stateLock.unlock();
  }
  return std::make_pair(err, errMsg);
}

