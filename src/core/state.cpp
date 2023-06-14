#include "state.h"

State::State(
  const std::unique_ptr<DB>& db,
  const std::unique_ptr<Storage>& storage,
  const std::unique_ptr<rdPoS>& rdpos,
  const std::unique_ptr<P2P::ManagerNormal>& p2pManager,
  const std::unique_ptr<Options>& options
) : db(db), storage(storage), rdpos(rdpos), p2pManager(p2pManager), options(options),
contractManager(std::make_unique<ContractManager>(this, db, rdpos, options))
{
  std::unique_lock lock(this->stateMutex);
  auto accountsFromDB = db->getBatch(DBPrefix::nativeAccounts);
  if (accountsFromDB.empty()) {
    /// Initialize with 0x00dead00665771855a34155f5e7405489df2c3c6 with nonce 0.
    Address dev1(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true);
    /// See ~State for encoding
    uint256_t desiredBalance("1000000000000000000000");
    std::string value = Utils::uintToBytes(Utils::bytesRequired(desiredBalance)) + Utils::uintToBytes(desiredBalance) + '\x00';
    db->put(dev1.get(), value, DBPrefix::nativeAccounts);
    accountsFromDB = db->getBatch(DBPrefix::nativeAccounts);
  }

  for (auto const dbEntry : accountsFromDB) {
    std::string_view data(dbEntry.value);
    if (dbEntry.key.size() != 20) {
      Utils::logToDebug(Log::state, __func__, "Error when loading State from DB, address from DB size mismatch");
      throw std::runtime_error("Error when loading State from DB, address from DB size mismatch");
    }
    uint8_t balanceSize = Utils::fromBigEndian<uint8_t>(data.substr(0,1));
    if (data.size() + 1 < data.size()) {
      Utils::logToDebug(Log::state, __func__, "Error when loading State from DB, value from DB doesn't size mismatch on balanceSize");
      throw std::runtime_error("Error when loading State from DB, value from DB size mismatch on balanceSize");
    }

    uint256_t balance = Utils::fromBigEndian<uint256_t>(data.substr(1, balanceSize));
    uint8_t nonceSize = Utils::fromBigEndian<uint8_t>(data.substr(1 + balanceSize, 1));

    if (2 + balanceSize + nonceSize != data.size()) {
      Utils::logToDebug(Log::state, __func__, "Error when loading State from DB, value from DB doesn't size mismatch on nonceSize");
      throw std::runtime_error("Error when loading State from DB, value from DB size mismatch on nonceSize");
    }
    uint64_t nonce = Utils::fromBigEndian<uint64_t>(data.substr(2 + balanceSize, nonceSize));

    this->accounts.insert({Address(dbEntry.key, true), Account(std::move(balance), std::move(nonce))});
  }
}

State::~State() {
  /// DB is stored as following
  /// Under the DBPrefix::nativeAccounts
  /// Each key == Address
  /// Each Value == Balance + uint256_t (not exact bytes)
  /// Value == 1 Byte (Balance Size) + N Bytes (Balance) + 1 Byte (Nonce Size) + N Bytes (Nonce).
  /// Max size for Value = 32 Bytes, Max Size for Nonce = 8 Bytes.
  /// If the nonce equals to 0, it will be *empty*
  DBBatch accountsBatch;
  std::unique_lock lock(this->stateMutex);
  for (const auto& [address, account] : this->accounts) {
    // Serialize Balance.
    std::string serializedStr = (account.balance == 0) ? std::string(1, 0x00) : Utils::uintToBytes(Utils::bytesRequired(account.balance)) + Utils::uintToBytes(account.balance);
    // Serialize Account.
    serializedStr += (account.nonce == 0) ? std::string(1, 0x00) : Utils::uintToBytes(Utils::bytesRequired(account.nonce)) + Utils::uintToBytes(account.nonce);
    accountsBatch.puts.emplace_back(DBEntry(address.get(), std::move(serializedStr)));
  }

  this->db->putBatch(accountsBatch, DBPrefix::nativeAccounts);
}

TxInvalid State::validateTransactionInternal(const TxBlock& tx) const {
  /**
   * Rules for a transaction to be accepted within the current state:
   * Transaction value + txFee (gas * gasPrice) needs to be lower than account balance
   * Transaction nonce must match account nonce
   */

  /// Verify if transaction already exists within the mempool, if on mempool, it has been validated previously.
  if (this->mempool.contains(tx.hash())) {
    Utils::logToDebug(Log::state, __func__, "Transaction: " + tx.hash().hex().get() + " already in mempool");
    return TxInvalid::NotInvalid;
  }
  auto accountIt = this->accounts.find(tx.getFrom());
  if (accountIt == this->accounts.end()) {
    Utils::logToDebug(Log::state, __func__, "Account doesn't exist (0 balance and 0 nonce)");
    return TxInvalid::InvalidBalance;
  }
  const auto& accBalance = accountIt->second.balance;
  const auto& accNonce = accountIt->second.nonce;
  uint256_t txWithFees = tx.getValue() + (tx.getGasLimit() * tx.getMaxFeePerGas());
  if (txWithFees > accBalance) {
    Utils::logToDebug(Log::state, __func__,
                      "Transaction sender: " + tx.getFrom().hex().get() + " doesn't have balance to send transaction"
                      + " expected: " + txWithFees.str() + " has: " + accBalance.str());
    return TxInvalid::InvalidBalance;
  }
  // TODO: The blockchain is able to store higher nonce transactions until they are valid
  // Handle this case.
  if (accNonce != tx.getNonce()) {
    Utils::logToDebug(Log::state, __func__, "Transaction: " + tx.hash().hex().get() + " nonce mismatch, expected: " + std::to_string(accNonce)
                                            + " got: " + tx.getNonce().str());
    return TxInvalid::InvalidNonce;
  }



  return TxInvalid::NotInvalid;
}

void State::processTransaction(const TxBlock& tx) {
  // Lock is already called by processNextBlock.
  // processNextBlock already calls validateTransaction in every tx, as it
  // calls validateNextBlock as a sanity check.
  // TODO: Contract calling, including "payable" functions.
  auto accountIt = this->accounts.find(tx.getFrom());
  auto& balance = accountIt->second.balance;
  auto& nonce = accountIt->second.nonce;
  try {
    uint256_t txValueWithFees = tx.getValue() + (
      tx.getGasLimit() * tx.getMaxFeePerGas()
    ); // This needs to change with payable contract functions
    balance -= txValueWithFees;
    this->accounts[tx.getTo()].balance += tx.getValue();
    if (this->contractManager->isContractCall(tx)) {
      if (this->contractManager->isPayable(tx.txToCallInfo())) this->processingPayable = true;
      this->contractManager->callContract(tx);
      this->processingPayable = false;
    }
  } catch (const std::exception& e) {
    Utils::logToDebug(Log::state, __func__,
      "Transaction: " + tx.hash().hex().get() + " failed to process, reason: " + e.what()
    );
    balance += tx.getValue();
  }
  nonce++;
}

void State::refreshMempool(const Block& block) {
  /// No need to lock mutex as function caller (this->processNextBlock) already lock mutex.
  /// Remove all transactions within the block that exists on the unordered_map.
  for (const auto& tx : block.getTxs()) {
    const auto it = this->mempool.find(tx.hash());
    if (it != this->mempool.end()) {
      this->mempool.erase(it);
    }
  }

  /// Copy mempool over
  auto mempoolCopy = this->mempool;
  this->mempool.clear();

  /// Verify if the transactions within the old mempool
  /// not added to the block are valid given the current state
  for (const auto& [hash, tx] : mempoolCopy) {
    /// Calls internal function which doesn't lock mutex.
    if (!this->validateTransactionInternal(tx)) {
      this->mempool.insert({hash, tx});
    }
  }
}

const uint256_t State::getNativeBalance(const Address &addr) const {
  std::shared_lock lock(this->stateMutex);
  auto it = this->accounts.find(addr);
  if (it == this->accounts.end()) return 0;
  return it->second.balance;
}


const uint64_t State::getNativeNonce(const Address& addr) const {
  std::shared_lock lock(this->stateMutex);
  auto it = this->accounts.find(addr);
  if (it == this->accounts.end()) return 0;
  return it->second.nonce;
}

const std::unordered_map<Address, Account, SafeHash> State::getAccounts() const {
  std::shared_lock lock(this->stateMutex);
  return this->accounts;
}

const std::unordered_map<Hash, TxBlock, SafeHash> State::getMempool() const {
  std::shared_lock lock(this->stateMutex);
  return this->mempool;
}

bool State::validateNextBlock(const Block& block) const {
  /**
   * Rules for a block to be accepted within the current state
   * Block nHeight must match latest nHeight + 1
   * Block nPrevHash must match latest hash
   * Block nTimestamp must be higher than latest block
   * Block has valid rdPoS transaction and signature based on current state.
   * All transactions within Block are valid (does not return false on validateTransaction)
   * Block constructor already checks if merkle roots within a block are valid.
   */

  auto latestBlock = this->storage->latest();
  if (block.getNHeight() != latestBlock->getNHeight() + 1) {
    Utils::logToDebug(Log::state, __func__, "Block nHeight doesn't match, expected "
                      + std::to_string(latestBlock->getNHeight() + 1) + " got " + std::to_string(block.getNHeight()));
    return false;
  }

  if (block.getPrevBlockHash() != latestBlock->hash()) {
    Utils::logToDebug(Log::state, __func__, "Block prevBlockHash doesn't match, expected " +
                      latestBlock->hash().hex().get() + " got: " + block.getPrevBlockHash().hex().get());
    return false;
  }

  if (latestBlock->getTimestamp() > block.getTimestamp()) {
    Utils::logToDebug(Log::state, __func__, "Block timestamp is lower than latest block, expected higher than " + std::to_string(latestBlock->getTimestamp())
                      + " got " + std::to_string(block.getTimestamp()));
    return false;
  }

  if (!this->rdpos->validateBlock(block)) {
    Utils::logToDebug(Log::state, __func__, "Invalid rdPoS in block");
    return false;
  }

  std::shared_lock verifyingBlockTxs(this->stateMutex);
  for (const auto& tx : block.getTxs()) {
    if (this->validateTransactionInternal(tx)) {
      Utils::logToDebug(Log::state, __func__, "Transaction " + tx.hash().hex().get() + " within block is invalid");
      return false;
    }
  }

  Utils::logToDebug(Log::state, __func__, "Block " + block.hash().hex().get() + " is valid. (Sanity Check Passed)");
  return true;
}

void State::processNextBlock(Block&& block) {
  /// Sanity Check.
  if (!this->validateNextBlock(block)) {
    Utils::logToDebug(Log::state, __func__, "Sanity check failed, blockchain is trying to append a invalid block, throwing.");
    throw std::runtime_error("Invalid block detected during processNextBlock sanity check.");
  }

  std::unique_lock lock(this->stateMutex);
  /// Process transactions of the block within the current state.
  for (auto const& tx : block.getTxs()) {
    this->processTransaction(tx);
  }

  /// Process rdPoS State
  this->rdpos->processBlock(block);

  /// Refresh the mempool based on the block transactions;
  this->refreshMempool(block);

  Utils::logToDebug(Log::state, __func__, "Block " + block.hash().hex().get() + " processed successfully.) block bytes: " + Hex::fromBytes(block.serializeBlock()).get());
  Utils::safePrint("Block: " + block.hash().hex().get() + " height: " + std::to_string(block.getNHeight()) + " was added to the blockchain");
  for (const auto& tx : block.getTxs()) {
    Utils::safePrint("Transaction: " + tx.hash().hex().get() + " was accepted in the blockchain");
  }
  /// Move block to storage.
  this->storage->pushBack(std::move(block));
  return;
}

void State::fillBlockWithTransactions(Block& block) const {
  std::shared_lock lock(this->stateMutex);
  for (const auto& [hash, tx] : this->mempool) {
    block.appendTx(tx);
  }
  return;
}

TxInvalid State::validateTransaction(const TxBlock& tx) const {
  std::shared_lock lock(this->stateMutex);
  return this->validateTransactionInternal(tx);
}

TxInvalid State::addTx(TxBlock&& tx) {
  auto TxInvalid = this->validateTransaction(tx);
  if (TxInvalid) return TxInvalid;
  std::unique_lock lock(this->stateMutex);
  auto txHash = tx.hash();
  this->mempool.insert({txHash, std::move(tx)});
  Utils::safePrint("Transaction: " + tx.hash().hex().get() + " was added to the mempool");
  return TxInvalid;
}

bool State::addValidatorTx(const TxValidator& tx) {
  std::unique_lock lock(this->stateMutex);
  return this->rdpos->addValidatorTx(tx);
}

bool State::isTxInMempool(const Hash& txHash) const {
  std::shared_lock lock(this->stateMutex);
  return this->mempool.contains(txHash);
}

std::unique_ptr<TxBlock> State::getTxFromMempool(const Hash &txHash) const {
  std::shared_lock lock(this->stateMutex);
  auto it = this->mempool.find(txHash);
  if (it == this->mempool.end()) return nullptr;
  return std::make_unique<TxBlock>(it->second);
}

void State::addBalance(const Address& addr) {
  std::unique_lock lock(this->stateMutex);
  this->accounts[addr].balance += uint256_t("1000000000000000000000");
}

std::string State::ethCall(const ethCallInfo& callInfo) {
  std::shared_lock lock(this->stateMutex);
  auto &address = std::get<1>(callInfo);
  if (this->contractManager->isContractAddress(address)) {
    return this->contractManager->callContract(callInfo);
  } else {
    return "";
  }
}

bool State::estimateGas(const ethCallInfo& callInfo) {
  std::shared_lock lock(this->stateMutex);
  auto [from, to, gasLimit, gasPrice, value, data] = callInfo;

  /// Check balance/gasLimit/gasPrice if available.
  if (from) {
    if (value) {
      uint256_t totalGas = 0;
      if (gasLimit && gasPrice) {
        totalGas = gasLimit * gasPrice;
      }
      auto it = this->accounts.find(from);
      if (it == this->accounts.end()) return false;
      if (it->second.balance < value + totalGas) return false;
    }
  }

  if (this->contractManager->isContractAddress(to)) {
    contractManager->validateCallContractWithTx(callInfo);
  }

  return true;
}

void State::processContractPayable(std::unordered_map<Address, uint256_t, SafeHash>& payableMap) {
  if (!this->processingPayable) throw std::runtime_error(
    "Uh oh, contracts are going haywire! Cannot change State while not processing a payable contract."
  );
  for (const auto& [address, amount] : payableMap) this->accounts[address].balance = amount;
}

std::vector<std::pair<std::string, Address>> State::getContracts() const {
  std::shared_lock lock(this->stateMutex);
  return this->contractManager->getContracts();
}

