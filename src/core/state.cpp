/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "state.h"
#include <evmone/evmone.h>
#include "../contract/contracthost.h"

State::State(
  DB& db,
  Storage& storage,
  P2P::ManagerNormal& p2pManager,
  const Options& options
) : vm_(evmc_create_evmone()), db_(db), storage_(storage), p2pManager_(p2pManager), options_(options),
    rdpos_(db, storage, p2pManager, options, *this),
    eventManager_(db, options_) {
  std::unique_lock lock(this->stateMutex_);
  auto accountsFromDB = db_.getBatch(DBPrefix::nativeAccounts);
  if (accountsFromDB.empty()) {
    for (const auto& [account, balance] : options_.getGenesisBalances()) {
      // Initialize all accounts within options genesis balances.
      Bytes value = Utils::uintToBytes(Utils::bytesRequired(balance));
      Utils::appendBytes(value,Utils::uintToBytes(balance));
      value.insert(value.end(), 0x00);
      db_.put(account.get(), value, DBPrefix::nativeAccounts);
    }
    accountsFromDB = db_.getBatch(DBPrefix::nativeAccounts);
  }

  for (const auto& dbEntry : accountsFromDB) {
    BytesArrView data(dbEntry.value);
    if (dbEntry.key.size() != 20) {
      Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Error when loading State from DB, address from DB size mismatch");
      throw DynamicException("Error when loading State from DB, address from DB size mismatch");
    }
    uint8_t balanceSize = Utils::fromBigEndian<uint8_t>(data.subspan(0,1));
    if (data.size() + 1 < data.size()) {
      Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Error when loading State from DB, value from DB doesn't size mismatch on balanceSize");
      throw DynamicException("Error when loading State from DB, value from DB size mismatch on balanceSize");
    }

    uint256_t balance = Utils::fromBigEndian<uint256_t>(data.subspan(1, balanceSize));
    uint8_t nonceSize = Utils::fromBigEndian<uint8_t>(data.subspan(1 + balanceSize, 1));

    if (2 + balanceSize + nonceSize != data.size()) {
      Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Error when loading State from DB, value from DB doesn't size mismatch on nonceSize");
      throw DynamicException("Error when loading State from DB, value from DB size mismatch on nonceSize");
    }
    uint64_t nonce = Utils::fromBigEndian<uint64_t>(data.subspan(2 + balanceSize, nonceSize));

    this->accounts_.insert({Address(dbEntry.key), Account(std::move(balance), std::move(nonce))});
  }
  auto latestBlock = this->storage_.latest();

  // Create the ContractManager
  this->contracts_[ProtocolContractAddresses.at("ContractManager")] = std::make_unique<ContractManager>(
    db, this->contracts_, this->options_
  );
  ContractGlobals::coinbase_ = Secp256k1::toAddress(latestBlock->getValidatorPubKey());
  ContractGlobals::blockHash_ = latestBlock->hash();
  ContractGlobals::blockHeight_ = latestBlock->getNHeight();
  ContractGlobals::blockTimestamp_ = latestBlock->getTimestamp();
}

State::~State() {
  // DB is stored as following
  // Under the DBPrefix::nativeAccounts
  // Each key == Address
  // Each Value == Balance + uint256_t (not exact bytes)
  // Value == 1 Byte (Balance Size) + N Bytes (Balance) + 1 Byte (Nonce Size) + N Bytes (Nonce).
  // Max size for Value = 32 Bytes, Max Size for Nonce = 8 Bytes.
  // If the nonce equals to 0, it will be *empty*
  DBBatch accountsBatch;
  std::unique_lock lock(this->stateMutex_);
  evmc_destroy(this->vm_);
  for (const auto& [address, account] : this->accounts_) {
    // Serialize Balance.
    Bytes serializedBytes;
    if (account.balance == 0) {
      serializedBytes = Bytes(1, 0x00);
    } else {
      serializedBytes = Utils::uintToBytes(Utils::bytesRequired(account.balance));
      Utils::appendBytes(serializedBytes, Utils::uintToBytes(account.balance));
    }
    // Serialize Account.
    if (account.nonce == 0) {
      Utils::appendBytes(serializedBytes, Bytes(1, 0x00));
    } else {
      Utils::appendBytes(serializedBytes, Utils::uintToBytes(Utils::bytesRequired(account.nonce)));
      Utils::appendBytes(serializedBytes, Utils::uintToBytes(account.nonce));
    }
    accountsBatch.push_back(address.get(), serializedBytes, DBPrefix::nativeAccounts);
  }

  this->db_.putBatch(accountsBatch);
}

TxInvalid State::validateTransactionInternal(const TxBlock& tx) const {
  /**
   * Rules for a transaction to be accepted within the current state:
   * Transaction value + txFee (gas * gasPrice) needs to be lower than account balance
   * Transaction nonce must match account nonce
   */

  // Verify if transaction already exists within the mempool, if on mempool, it has been validated previously.
  if (this->mempool_.contains(tx.hash())) {
    Logger::logToDebug(LogType::INFO, Log::state, __func__, "Transaction: " + tx.hash().hex().get() + " already in mempool");
    return TxInvalid::NotInvalid;
  }
  auto accountIt = this->accounts_.find(tx.getFrom());
  if (accountIt == this->accounts_.end()) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Account doesn't exist (0 balance and 0 nonce)");
    return TxInvalid::InvalidBalance;
  }
  const auto& accBalance = accountIt->second.balance;
  const auto& accNonce = accountIt->second.nonce;
  uint256_t txWithFees = tx.getValue() + (tx.getGasLimit() * tx.getMaxFeePerGas());
  if (txWithFees > accBalance) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__,
                      "Transaction sender: " + tx.getFrom().hex().get() + " doesn't have balance to send transaction"
                      + " expected: " + txWithFees.str() + " has: " + accBalance.str());
    return TxInvalid::InvalidBalance;
  }
  // TODO: The blockchain is able to store higher nonce transactions until they are valid. Handle this case.
  if (accNonce != tx.getNonce()) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Transaction: " + tx.hash().hex().get() + " nonce mismatch, expected: " + std::to_string(accNonce)
                                            + " got: " + tx.getNonce().str());
    return TxInvalid::InvalidNonce;
  }
  return TxInvalid::NotInvalid;
}

void State::processTransaction(const TxBlock& tx,
                               const Hash& blockHash,
                               const uint64_t& txIndex) {
  // Lock is already called by processNextBlock.
  // processNextBlock already calls validateTransaction in every tx,
  // as it calls validateNextBlock as a sanity check.
  auto accountIt = this->accounts_.find(tx.getFrom());
  const auto& accountTo = this->accounts_[tx.getTo()];
  int64_t leftOverGas = int64_t(tx.getGasLimit());
  auto& nonce = accountIt->second.nonce;
  auto& balance = accountIt->second.balance;
  if (balance < (tx.getValue() + tx.getGasLimit() * tx.getMaxFeePerGas())) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Transaction sender: " + tx.getFrom().hex().get() + " doesn't have balance to send transaction");
    throw DynamicException("Transaction sender doesn't have balance to send transaction");
    return;
  }
  if (nonce != tx.getNonce()) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Transaction: " + tx.hash().hex().get() + " nonce mismatch, expected: " + std::to_string(nonce)
                                            + " got: " + tx.getNonce().str());
    throw DynamicException("Transaction nonce mismatch");
    return;
  }
  try {
    evmc_tx_context txContext;
    txContext.tx_gas_price = Utils::uint256ToEvmcUint256(tx.getMaxFeePerGas());
    txContext.tx_origin = tx.getFrom().toEvmcAddress();
    txContext.block_coinbase = ContractGlobals::getCoinbase().toEvmcAddress();
    txContext.block_number = ContractGlobals::getBlockHeight();
    txContext.block_timestamp = ContractGlobals::getBlockTimestamp();
    txContext.block_gas_limit = 10000000;
    txContext.block_prev_randao = {};
    txContext.chain_id = Utils::uint256ToEvmcUint256(this->options_.getChainID());
    txContext.block_base_fee = {};
    txContext.blob_base_fee = {};
    txContext.blob_hashes = nullptr;
    txContext.blob_hashes_count = 0;
    ContractHost host(
      this->vm_,
      this->eventManager_,
      this->storage_,
      txContext,
      this->contracts_,
      this->accounts_,
      this->vmStorage_,
      tx.hash(),
      txIndex,
      blockHash,
      leftOverGas
    );

    host.execute(tx.txToCallInfo(), accountTo.contractType);

  } catch (const std::exception& e) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Transaction: " + tx.hash().hex().get() + " failed to execute: " + e.what());
    throw DynamicException("Transaction failed to execute: " + std::string(e.what()));
  }

  /// It is most probably that the account iterator is invalidated after the host.execute call.
  /// So we need to find the account again.
  accountIt = this->accounts_.find(tx.getFrom());
  accountIt->second.nonce++;
  auto usedGas = tx.getGasLimit() - leftOverGas;
  accountIt->second.balance -= usedGas * tx.getMaxFeePerGas();
  nonce++;
}

void State::refreshMempool(const Block& block) {
  // No need to lock mutex as function caller (this->processNextBlock) already lock mutex.
  // Remove all transactions within the block that exists on the unordered_map.
  for (const auto& tx : block.getTxs()) {
    const auto it = this->mempool_.find(tx.hash());
    if (it != this->mempool_.end()) {
      this->mempool_.erase(it);
    }
  }

  // Copy mempool over
  auto mempoolCopy = this->mempool_;
  this->mempool_.clear();

  // Verify if the transactions within the old mempool
  // not added to the block are valid given the current state
  for (const auto& [hash, tx] : mempoolCopy) {
    // Calls internal function which doesn't lock mutex.
    if (!this->validateTransactionInternal(tx)) {
      this->mempool_.insert({hash, tx});
    }
  }
}

uint256_t State::getNativeBalance(const Address &addr) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->accounts_.find(addr);
  if (it == this->accounts_.end()) return 0;
  return it->second.balance;
}

uint64_t State::getNativeNonce(const Address& addr) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->accounts_.find(addr);
  if (it == this->accounts_.end()) return 0;
  return it->second.nonce;
}

std::unordered_map<Address, Account, SafeHash> State::getAccounts() const {
  std::shared_lock lock(this->stateMutex_);
  return this->accounts_;
}

std::unordered_map<Hash, TxBlock, SafeHash> State::getMempool() const {
  std::shared_lock lock(this->stateMutex_);
  return this->mempool_;
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
  auto latestBlock = this->storage_.latest();
  if (block.getNHeight() != latestBlock->getNHeight() + 1) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__,
      "Block nHeight doesn't match, expected " + std::to_string(latestBlock->getNHeight() + 1)
      + " got " + std::to_string(block.getNHeight())
    );
    return false;
  }

  if (block.getPrevBlockHash() != latestBlock->hash()) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__,
      "Block prevBlockHash doesn't match, expected " + latestBlock->hash().hex().get()
      + " got: " + block.getPrevBlockHash().hex().get()
    );
    return false;
  }

  if (latestBlock->getTimestamp() > block.getTimestamp()) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__,
      "Block timestamp is lower than latest block, expected higher than "
      + std::to_string(latestBlock->getTimestamp()) + " got " + std::to_string(block.getTimestamp())
    );
    return false;
  }

  if (!this->rdpos_.validateBlock(block)) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__, "Invalid rdPoS in block");
    return false;
  }

  std::shared_lock verifyingBlockTxs(this->stateMutex_);
  for (const auto& tx : block.getTxs()) {
    if (this->validateTransactionInternal(tx)) {
      Logger::logToDebug(LogType::ERROR, Log::state, __func__,
        "Transaction " + tx.hash().hex().get() + " within block is invalid"
      );
      return false;
    }
  }

  Logger::logToDebug(LogType::INFO, Log::state, __func__,
    "Block " + block.hash().hex().get() + " is valid. (Sanity Check Passed)"
  );
  return true;
}

void State::processNextBlock(Block&& block) {
  // Sanity check - if it passes, the block is valid and will be processed
  if (!this->validateNextBlock(block)) {
    Logger::logToDebug(LogType::ERROR, Log::state, __func__,
      "Sanity check failed - blockchain is trying to append a invalid block, throwing"
    );
    throw DynamicException("Invalid block detected during processNextBlock sanity check");
  }

  std::unique_lock lock(this->stateMutex_);

  // Update contract globals based on (now) latest block
  const Hash blockHash = block.hash();
  ContractGlobals::coinbase_ = Secp256k1::toAddress(block.getValidatorPubKey());
  ContractGlobals::blockHash_ = blockHash;
  ContractGlobals::blockHeight_ = block.getNHeight();
  ContractGlobals::blockTimestamp_ = block.getTimestamp();

  // Process transactions of the block within the current state
  uint64_t txIndex = 0;
  for (auto const& tx : block.getTxs()) {
    this->processTransaction(tx, blockHash, txIndex);
    txIndex++;
  }

  // Process rdPoS State
  this->rdpos_.processBlock(block);

  // Refresh the mempool based on the block transactions
  this->refreshMempool(block);
  Logger::logToDebug(LogType::INFO, Log::state, __func__, "Block " + block.hash().hex().get() + " processed successfully.");
  Utils::safePrint("Block: " + block.hash().hex().get() + " height: " + std::to_string(block.getNHeight()) + " was added to the blockchain");
  for (const auto& tx : block.getTxs()) {
    Utils::safePrint("Transaction: " + tx.hash().hex().get() + " was accepted in the blockchain");
  }

  // Move block to storage
  this->storage_.pushBack(std::move(block));
}

void State::fillBlockWithTransactions(Block& block) const {
  std::shared_lock lock(this->stateMutex_);
  for (const auto& [hash, tx] : this->mempool_) block.appendTx(tx);
}

TxInvalid State::validateTransaction(const TxBlock& tx) const {
  std::shared_lock lock(this->stateMutex_);
  return this->validateTransactionInternal(tx);
}

TxInvalid State::addTx(TxBlock&& tx) {
  auto TxInvalid = this->validateTransaction(tx);
  if (TxInvalid) return TxInvalid;
  std::unique_lock lock(this->stateMutex_);
  auto txHash = tx.hash();
  this->mempool_.insert({txHash, std::move(tx)});
  Utils::safePrint("Transaction: " + tx.hash().hex().get() + " was added to the mempool");
  return TxInvalid;
}

bool State::addValidatorTx(const TxValidator& tx) {
  std::unique_lock lock(this->stateMutex_);
  return this->rdpos_.addValidatorTx(tx);
}

bool State::isTxInMempool(const Hash& txHash) const {
  std::shared_lock lock(this->stateMutex_);
  return this->mempool_.contains(txHash);
}

std::unique_ptr<TxBlock> State::getTxFromMempool(const Hash &txHash) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->mempool_.find(txHash);
  if (it == this->mempool_.end()) return nullptr;
  return std::make_unique<TxBlock>(it->second);
}

void State::addBalance(const Address& addr) {
  std::unique_lock lock(this->stateMutex_);
  this->accounts_[addr].balance += uint256_t("1000000000000000000000");
}

Bytes State::ethCall(const ethCallInfo& callInfo) {
  // We actually need to lock uniquely here
  // As the contract host will modify (reverting in the end) the state.
  std::unique_lock lock(this->stateMutex_);
  const auto &address = std::get<1>(callInfo);
  const auto& accIt = this->accounts_.find(address);
  if (accIt == this->accounts_.end()) {
    return {};
  }
  const auto& acc = accIt->second;
  if (acc.isContract()) {
    int64_t leftOverGas = int64_t(std::get<2>(callInfo));
    evmc_tx_context txContext;
    txContext.tx_gas_price = Utils::uint256ToEvmcUint256(std::get<3>(callInfo));
    txContext.tx_origin = std::get<0>(callInfo).toEvmcAddress();
    txContext.block_coinbase = ContractGlobals::getCoinbase().toEvmcAddress();
    txContext.block_number = ContractGlobals::getBlockHeight();
    txContext.block_timestamp = ContractGlobals::getBlockTimestamp();
    txContext.block_gas_limit = 10000000;
    txContext.block_prev_randao = {};
    txContext.chain_id = Utils::uint256ToEvmcUint256(this->options_.getChainID());
    txContext.block_base_fee = {};
    txContext.blob_base_fee = {};
    txContext.blob_hashes = nullptr;
    txContext.blob_hashes_count = 0;
    ContractHost host(
      this->vm_,
      this->eventManager_,
      this->storage_,
      txContext,
      this->contracts_,
      this->accounts_,
      this->vmStorage_,
      Hash(),
      0,
      Hash(),
      leftOverGas
    );
    return host.ethCallView(callInfo, acc.contractType);
  } else {
    return {};
  }
}

int64_t State::estimateGas(const ethCallInfo& callInfo) {
  std::unique_lock lock(this->stateMutex_);
  const auto& [from, to, gasLimit, gasPrice, value, functor, data, fullData] = callInfo;
  // ContractHost simulate already do all necessary checks
  // We just need to execute and get the leftOverGas
  ContractType type = ContractType::NOT_A_CONTRACT;
  auto accIt = this->accounts_.find(to);
  if (accIt != this->accounts_.end()) {
    type = accIt->second.contractType;
  }

  int64_t leftOverGas = int64_t(gasLimit);
  ContractHost(
    this->vm_,
    this->eventManager_,
    this->storage_,
    evmc_tx_context(),
    this->contracts_,
    this->accounts_,
    this->vmStorage_,
    Hash(),
    0,
    Hash(),
    leftOverGas
  ).simulate(callInfo, type);


  return (int64_t(gasLimit) - leftOverGas);
}

std::vector<std::pair<std::string, Address>> State::getContracts() const {
  std::shared_lock lock(this->stateMutex_);
  std::vector<std::pair<std::string, Address>> contracts;
  for (const auto& [address, contract] : this->contracts_) {
    contracts.emplace_back(contract->getContractName(), address);
  }
  return contracts;
}

std::vector<Event> State::getEvents(
  const uint64_t& fromBlock, const uint64_t& toBlock,
  const Address& address, const std::vector<Hash>& topics
) const {
  std::shared_lock lock(this->stateMutex_);
  return this->eventManager_.getEvents(fromBlock, toBlock, address, topics);
}

std::vector<Event> State::getEvents(
  const Hash& txHash, const uint64_t& blockIndex, const uint64_t& txIndex
) const {
  std::shared_lock lock(this->stateMutex_);
  return this->eventManager_.getEvents(txHash, blockIndex, txIndex);
}

