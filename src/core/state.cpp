/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <evmone/evmone.h>

#include "state.h"

#include "../contract/contracthost.h" // contractmanager.h

#include "../utils/uintconv.h"
#include "bytes/random.h"
#include "../net/http/jsonrpc/error.h"

State::State(
  const DB& db,
  Storage& storage,
  P2P::ManagerNormal& p2pManager,
  const uint64_t& snapshotHeight,
  const Options& options
) : vm_(evmc_create_evmone()),
  options_(options),
  storage_(storage),
  dumpManager_(storage_, options_, this->stateMutex_),
  dumpWorker_(options_, storage_, dumpManager_),
  p2pManager_(p2pManager),
  rdpos_(db, dumpManager_, storage, p2pManager, options),
  blockObservers_(vm_, dumpManager_, storage_, contracts_, accounts_, vmStorage_, options_)
{
  std::unique_lock lock(this->stateMutex_);
  if (snapshotHeight != 0) {
    Utils::safePrint("Loading state from snapshot height: " + std::to_string(snapshotHeight));
  }
  auto now = std::chrono::system_clock::now();

  if (auto accountsFromDB = db.getBatch(DBPrefix::nativeAccounts); accountsFromDB.empty()) {
    Utils::safePrint("No accounts found in DB, initializing genesis state");
    if (snapshotHeight != 0) {
      throw DynamicException("Snapshot height is higher than 0, but no accounts found in DB");
    }
    for (const auto& [addr, balance] : options_.getGenesisBalances()) {
      this->accounts_[addr]->balance = balance;
    }
    // Also append the ContractManager account
    auto& contractManagerAcc = *this->accounts_[ProtocolContractAddresses.at("ContractManager")];
    contractManagerAcc.nonce = 1;
    contractManagerAcc.contractType = ContractType::CPP;
  } else {

#ifdef BUILD_TESTNET
    // We gotta import the EVM Accounts code from the Account object itself
    // to the evmContracts_ map, so we can save up memory and avoid duplicated code
    if (!db.hasPrefix(DBPrefix::evmContracts)) {
      Utils::safePrint("No EVM Contracts found in DB, importing from Accounts...");
      for (const auto& dbEntry : accountsFromDB) {
        Address addr(dbEntry.key);
        this->accounts_.emplace(addr, dbEntry.value);
        auto& account = this->accounts_.at(addr);
        if (account->contractType == ContractType::EVM) {
          auto contractIt = this->evmContracts_.find(account->codeHash);
          if (contractIt == this->evmContracts_.end()) {
            if (dbEntry.value.size() < 73) {
              LOGERROR("Account " + addr.hex().get() + " is marked as EVM contract but has invalid serialized size");
              throw DynamicException("Account " + addr.hex().get() + " is marked as EVM contract but has invalid serialized size");
            }
            this->evmContracts_[account->codeHash] = std::make_shared<Bytes>(dbEntry.value.begin() + 73, dbEntry.value.end());
          } else {
            // Point the account code to the already existing code
            account->code = contractIt->second;
          }
        }
      }
    } else {
      for (const auto& dbEntry : accountsFromDB) {
        this->accounts_.emplace(Address(dbEntry.key), dbEntry.value);
      }
    }
#else
    for (const auto& dbEntry : accountsFromDB) {
      this->accounts_.emplace(Address(dbEntry.key), dbEntry.value);
    }
#endif
  }

  Utils::safePrint("Loaded " + std::to_string(this->accounts_.size()) + " accounts from DB");
  // Load all the EVM Storage Slot/keys from the DB
  for (const auto& dbEntry : db.getBatch(DBPrefix::vmStorage)) {
    Address addr(dbEntry.key | std::views::take(ADDRESS_SIZE));
    Hash hash(dbEntry.key | std::views::drop(ADDRESS_SIZE));

    this->vmStorage_.emplace(
      StorageKeyView(addr, hash),
      dbEntry.value);
  }
  // Load all EVM contracts from the DB
  for (const auto& dbEntry : db.getBatch(DBPrefix::evmContracts)) {
    Hash codeHash(dbEntry.key);
    this->evmContracts_[codeHash] = std::make_shared<Bytes>(dbEntry.value);
  }
  Utils::safePrint("Loaded " + std::to_string(this->evmContracts_.size()) + " unique EVM Contracts from DB");
  // Set the EVM Contract Accounts to point their respective code
  uint64_t evmContractAccounts = 0;
  for (auto& [address, account] : this->accounts_) {
    if (account->contractType == ContractType::EVM) {
      ++evmContractAccounts;
      auto it = this->evmContracts_.find(account->codeHash);
      if (it != this->evmContracts_.end()) {
        account->code = it->second;
      } else {
        LOGERROR("Account " + address.hex().get() + " is marked as EVM contract but code hash " + account->codeHash.hex().get() + " not found in DB");
        throw DynamicException("Account " + address.hex().get() + " is marked as EVM contract but code hash " + account->codeHash.hex().get() + " not found in DB");
      }
    }
  }
  Utils::safePrint("Loaded " + std::to_string(evmContractAccounts) + " EVM Contract accounts from DB");

  uint256_t realEVMContractsMemSize = 0;
  uint256_t previousEVMContractsMemSize = 0;
  for (const auto& [codeHash, code] : this->evmContracts_) {
    realEVMContractsMemSize += code->size() + (sizeof(std::shared_ptr<Bytes>) * code.use_count());
    // We also need to include the size of the shared_ptr reference count
    previousEVMContractsMemSize += code->size() * code.use_count();
  }

  uint256_t savedUpMem = previousEVMContractsMemSize - realEVMContractsMemSize;
  // Print in MB
  Utils::safePrint("EVM Contracts memory usage: " + (realEVMContractsMemSize / 1'000'000).str() + " MB"
    + " (Saved up to " + (savedUpMem / 1'000'000).str() + " MB by deduplicating code in accounts)"
  );


  auto latestBlock = this->storage_.latest();
  auto snapshotBlock = this->storage_.getBlock(snapshotHeight);
  if (snapshotBlock == nullptr) {
    throw DynamicException("Snapshot block not found!?");
  }
  Utils::safePrint("Initializing state at snapshot block " + snapshotBlock->getHash().hex().get()
    + " at height " + std::to_string(snapshotBlock->getNHeight())
  );
  ContractGlobals::coinbase_ = Secp256k1::toAddress(snapshotBlock->getValidatorPubKey());
  ContractGlobals::blockHash_ = snapshotBlock->getHash();
  ContractGlobals::blockHeight_ = snapshotBlock->getNHeight();
  ContractGlobals::blockTimestamp_ = snapshotBlock->getTimestamp();
  Utils::safePrint("Initializing the ContractManager...");
  // Insert the contract manager into the contracts_ map.
  this->contracts_[ProtocolContractAddresses.at("ContractManager")] = std::make_unique<ContractManager>(
    db, this->contracts_, this->dumpManager_ ,this->blockObservers_, this->options_
  );

  // State sanity check, lets check if all found contracts in the accounts_ map really have code or are C++ contracts
  Utils::safePrint("Performing contract sanity check on " + std::to_string(this->accounts_.size()) + " accounts...");
  for (const auto& [addr, acc] : this->accounts_) contractSanityCheck(addr, *acc);

  Utils::safePrint("Contract sanity check passed");
  if (snapshotHeight > this->storage_.latest()->getNHeight()) {
    LOGERROR("Snapshot height is higher than latest block, we can't load State! Crashing the program");
    throw DynamicException("Snapshot height is higher than latest block, we can't load State!");
  }

  // For each nHeight from snapshotHeight + 1 to latestBlock->getNHeight()
  // We need to process the block and update the state
  // We can't call processNextBlock here, as it will place the block again on the storage
  Utils::safePrint("Got latest block height: " + std::to_string(latestBlock->getNHeight()));
  std::unique_ptr<DBBatch> reindexedTxs = std::make_unique<DBBatch>();
  for (uint64_t nHeight = snapshotHeight + 1; nHeight <= latestBlock->getNHeight(); nHeight++) {
    auto block = this->storage_.getBlock(nHeight);
    ContractGlobals::coinbase_ = Secp256k1::toAddress(block->getValidatorPubKey());
    ContractGlobals::blockHash_ = block->getHash();
    ContractGlobals::blockHeight_ = block->getNHeight();
    ContractGlobals::blockTimestamp_ = block->getTimestamp();
    if (this->options_.getIndexingMode() == IndexingMode::RPC || this->options_.getIndexingMode() == IndexingMode::RPC_TRACE) {
      this->storage_.reindexTransactions(*block, *reindexedTxs);
    }
    if (reindexedTxs->getPuts().size() > 25000) {
      this->storage_.dumpToDisk(*reindexedTxs);
      reindexedTxs = std::make_unique<DBBatch>();
      LOGINFOP("Reindexing transactions at block " + block->getHash().hex().get() + " at height " + std::to_string(nHeight));
    }
    LOGINFOP("Processing block " + block->getHash().hex().get() + " at height " + std::to_string(nHeight));
    // Update contract globals based on (now) latest block
    const Hash blockHash = block->getHash();
    ContractGlobals::coinbase_ = Secp256k1::toAddress(block->getValidatorPubKey());
    ContractGlobals::blockHash_ = blockHash;
    ContractGlobals::blockHeight_ = block->getNHeight();
    ContractGlobals::blockTimestamp_ = block->getTimestamp();

    // Process transactions of the block within the current state
    uint64_t txIndex = 0;
    for (const auto& tx : block->getTxs()) {
      this->processTransaction(tx, blockHash, txIndex, block->getBlockRandomness());
      txIndex++;
    }
    blockObservers_.notify(*block);
    // Process rdPoS State
    this->rdpos_.processBlock(*block);
  }
  if (reindexedTxs->getPuts().size() > 0) {
    LOGINFOP("Reindexing remaining transactions");
    this->storage_.dumpToDisk(*reindexedTxs);
  }
  this->dumpManager_.pushBack(this);
  auto end = std::chrono::system_clock::now();
  double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - now).count();
  double elapsedInSeconds = elapsed / 1000;
  Utils::safePrint("State loaded in " + std::to_string(elapsedInSeconds) + " seconds");

}

State::~State() { evmc_destroy(this->vm_); }

void State::contractSanityCheck(const Address& addr, const Account& acc) {
  switch (acc.contractType) {
    case ContractType::CPP: {
      if (this->contracts_.find(addr) == this->contracts_.end()) {
        LOGERROR("Contract " + addr.hex().get() + " is marked as C++ contract but doesn't have code");
        throw DynamicException("Contract " + addr.hex().get() + " is marked as C++ contract but doesn't have code");
      }
      break;
    }
    case ContractType::EVM: {
      if (acc.code == nullptr || acc.code->empty()) {
        LOGERROR("Contract " + addr.hex().get() + " is marked as EVM contract but doesn't have code");
        throw DynamicException("Contract " + addr.hex().get() + " is marked as EVM contract but doesn't have code");
      }
      break;
    }
    case ContractType::NOT_A_CONTRACT: {
      if (acc.code != nullptr && !acc.code->empty()) {
        LOGERROR("Contract " + addr.hex().get() + " is marked as not a contract but has code");
        throw DynamicException("Contract " + addr.hex().get() + " is marked as not a contract but has code");
      }
      break;
    }
    default:
      // TODO: this is a placeholder, contract types should be revised.
      // Also we can NOT remove NOT_A_CONTRACT for now because tests will complain about it.
      throw DynamicException("Invalid contract type");
      break;
  }
}

DBBatch State::dump() const {
  // DB is stored as following
  // Under the DBPrefix::nativeAccounts
  // Each key == Address
  // Each Value == Account.serialize()
  DBBatch stateBatch;
  for (const auto& [address, account] : this->accounts_) {
    stateBatch.push_back(address, account->serialize(), DBPrefix::nativeAccounts);
  }
  // There is also the need to dump the vmStorage_ map
  for (const auto& [storageKey, storageValue] : this->vmStorage_) {
    const auto key = Utils::makeBytes(bytes::join(storageKey.first, storageKey.second));
    stateBatch.push_back(key, storageValue, DBPrefix::vmStorage);
  }
  // Also make sure to dump all the EVM Contracts
  for (const auto& [codeHash, code] : this->evmContracts_) {
    stateBatch.push_back(codeHash, *code, DBPrefix::evmContracts);
  }

  return stateBatch;
}

TxStatus State::validateTransactionInternal(const TxBlock& tx) const {
  /**
   * Rules for a transaction to be accepted within the current state:
   * Transaction value + txFee (gas * gasPrice) needs to be lower than account balance
   * Transaction nonce must match account nonce
   */

  // Verify if transaction already exists within the mempool, if on mempool, it has been validated previously.
  if (this->mempool_.contains(tx.hash())) {
    LOGTRACE("Transaction: " + tx.hash().hex().get() + " already in mempool");
    return TxStatus::ValidExisting;
  }
  auto accountIt = this->accounts_.find(tx.getFrom());
  if (accountIt == this->accounts_.end()) {
    LOGERROR("Account doesn't exist (0 balance and 0 nonce)");
    return TxStatus::InvalidBalance;
  }
  const auto& accBalance = accountIt->second->balance;
  const auto& accNonce = accountIt->second->nonce;
  if (
    uint256_t txWithFees = tx.getValue() + (tx.getGasLimit() * tx.getMaxFeePerGas());
    txWithFees > accBalance
  ) {
    LOGERROR("Transaction sender: " + tx.getFrom().hex().get()
      + " doesn't have balance to send transaction"
      + " expected: " + txWithFees.str() + " has: " + accBalance.str()
    );
    return TxStatus::InvalidBalance;
  }
  // TODO: The blockchain is able to store higher nonce transactions until they are valid. Handle this case.
  if (accNonce != tx.getNonce()) {
    LOGERROR("Transaction: " + tx.hash().hex().get()
      + " nonce mismatch, expected: " + std::to_string(accNonce)
      + " got: " + tx.getNonce().str()
    );
    return TxStatus::InvalidNonce;
  }
  return TxStatus::ValidNew;
}

void State::processTransaction(
  const TxBlock& tx, const Hash& blockHash, const uint64_t& txIndex, const Hash& randomnessHash
) {
  // Lock is already called by processNextBlock.
  // processNextBlock already calls validateTransaction in every tx,
  // as it calls validateNextBlock as a sanity check.
  Account& accountFrom = *this->accounts_[tx.getFrom()];
  auto& fromNonce = accountFrom.nonce;
  auto& fromBalance = accountFrom.balance;
  if (fromBalance < (tx.getValue() + tx.getGasLimit() * tx.getMaxFeePerGas())) {
    LOGERROR("Transaction sender: " + tx.getFrom().hex().get() + " doesn't have balance to send transaction");
    throw DynamicException("Transaction sender doesn't have balance to send transaction");
    return;
  }
  if (fromNonce != tx.getNonce()) {
    LOGERROR("Transaction: " + tx.hash().hex().get() + " nonce mismatch, expected: "
      + std::to_string(fromNonce) + " got: " + tx.getNonce().str()
    );
    throw DynamicException("Transaction nonce mismatch");
    return;
  }

  Gas gas(uint64_t(tx.getGasLimit()));
  TxAdditionalData txData{.hash = tx.hash()};

  try {
    const Hash randomSeed(UintConv::uint256ToBytes((static_cast<uint256_t>(randomnessHash) + txIndex)));

    ExecutionContext context = ExecutionContext::Builder{}
      .storage(this->vmStorage_)
      .accounts(this->accounts_)
      .contracts(this->contracts_)
      .evmContracts(this->evmContracts_)
      .blockHash(blockHash)
      .txHash(tx.hash())
      .txOrigin(tx.getFrom())
      .blockCoinbase(ContractGlobals::getCoinbase())
      .txIndex(txIndex)
      .blockNumber(ContractGlobals::getBlockHeight())
      .blockTimestamp(ContractGlobals::getBlockTimestamp())
      .blockGasLimit(10'000'000)
      .txGasPrice(tx.getMaxFeePerGas())
      .chainId(this->options_.getChainID())
      .build();

    ContractHost host(
      this->vm_,
      this->dumpManager_,
      this->storage_,
      randomSeed,
      context,
      &blockObservers_);

    std::visit([&] (auto&& msg) {
      if constexpr (concepts::CreateMessage<decltype(msg)>) {
        txData.contractAddress = host.execute(std::forward<decltype(msg)>(msg));
      } else {
        host.execute(std::forward<decltype(msg)>(msg));
      }
    }, tx.toMessage(gas));

    txData.succeeded = true;
  } catch (const std::exception& e) {
    txData.succeeded = false;
    LOGERRORP("Transaction: " + tx.hash().hex().get() + " failed to process, reason: " + e.what());
  }

  ++fromNonce;
  txData.gasUsed = uint64_t(tx.getGasLimit() - uint256_t(gas));

  if (storage_.getIndexingMode() != IndexingMode::DISABLED) {
    storage_.putTxAdditionalData(txData);
  }

  fromBalance -= (txData.gasUsed * tx.getMaxFeePerGas());
}

void State::refreshMempool(const FinalizedBlock& block) {
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
    if (isTxStatusValid(this->validateTransactionInternal(tx))) {
      this->mempool_.insert({hash, tx});
    }
  }
}

uint256_t State::getNativeBalance(const Address &addr) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->accounts_.find(addr);
  if (it == this->accounts_.end()) return 0;
  return it->second->balance;
}

uint64_t State::getNativeNonce(const Address& addr) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->accounts_.find(addr);
  if (it == this->accounts_.end()) return 0;
  return it->second->nonce;
}

std::vector<TxBlock> State::getMempool() const {
  std::shared_lock lock(this->stateMutex_);
  std::vector<TxBlock> mempoolCopy;
  mempoolCopy.reserve(this->mempool_.size());
  for (const auto& [hash, tx] : this->mempool_) {
    mempoolCopy.emplace_back(tx);
  }
  return mempoolCopy;
}

BlockValidationStatus State::validateNextBlockInternal(const FinalizedBlock& block) const {
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
    LOGERROR("Block nHeight doesn't match, expected " + std::to_string(latestBlock->getNHeight() + 1)
      + " got " + std::to_string(block.getNHeight())
    );
    return BlockValidationStatus::invalidWrongHeight;
  }

  if (block.getPrevBlockHash() != latestBlock->getHash()) {
    LOGERROR("Block prevBlockHash doesn't match, expected " + latestBlock->getHash().hex().get()
      + " got: " + block.getPrevBlockHash().hex().get()
    );
    return BlockValidationStatus::invalidErroneous;
  }

  if (latestBlock->getTimestamp() > block.getTimestamp()) {
    LOGERROR("Block timestamp is lower than latest block, expected higher than "
      + std::to_string(latestBlock->getTimestamp()) + " got " + std::to_string(block.getTimestamp())
    );
    return BlockValidationStatus::invalidErroneous;
  }

  if (!this->rdpos_.validateBlock(block)) {
    LOGERROR("Invalid rdPoS in block");
    return BlockValidationStatus::invalidErroneous;
  }

  for (const auto& tx : block.getTxs()) {
    if (!isTxStatusValid(this->validateTransactionInternal(tx))) {
      LOGERROR("Transaction " + tx.hash().hex().get() + " within block is invalid");
      return BlockValidationStatus::invalidErroneous;
    }
  }

  LOGTRACE("Block " + block.getHash().hex().get() + " is valid. (Sanity Check Passed)");
  return BlockValidationStatus::valid;
}

bool State::validateNextBlock(const FinalizedBlock& block) const {
  std::shared_lock lock(this->stateMutex_);
  return validateNextBlockInternal(block) == BlockValidationStatus::valid;
}

void State::processNextBlock(FinalizedBlock&& block) {
  if (tryProcessNextBlock(std::move(block)) != BlockValidationStatus::valid) {
    LOGERROR("Sanity check failed - blockchain is trying to append a invalid block, throwing");
    throw DynamicException("Invalid block detected during processNextBlock sanity check");
  }
}

BlockValidationStatus State::tryProcessNextBlock(FinalizedBlock&& block) {
  std::unique_lock lock(this->stateMutex_);

  // Sanity check - if it passes, the block is valid and will be processed
  BlockValidationStatus vStatus = this->validateNextBlockInternal(block);
  if (vStatus != BlockValidationStatus::valid) {
    return vStatus;
  }

  // Update contract globals based on (now) latest block
  const Hash blockHash = block.getHash();
  ContractGlobals::coinbase_ = Secp256k1::toAddress(block.getValidatorPubKey());
  ContractGlobals::blockHash_ = blockHash;
  ContractGlobals::blockHeight_ = block.getNHeight();
  ContractGlobals::blockTimestamp_ = block.getTimestamp();

  // Process transactions of the block within the current state
  uint64_t txIndex = 0;
  for (auto const& tx : block.getTxs()) {
    this->processTransaction(tx, blockHash, txIndex, block.getBlockRandomness());
    txIndex++;
  }

  // Process rdPoS State
  this->rdpos_.processBlock(block);

  // Refresh the mempool based on the block transactions
  this->refreshMempool(block);
  LOGINFO("Block " + block.getHash().hex().get() + " processed successfully.");
  Utils::safePrint("Block: " + block.getHash().hex().get() + " height: " + std::to_string(block.getNHeight()) + " was added to the blockchain");
  for (const auto& tx : block.getTxs()) {
    Utils::safePrint("Transaction: " + tx.hash().hex().get() + " was accepted in the blockchain");
  }

  blockObservers_.notify(block);

  // Move block to storage
  this->storage_.pushBlock(std::move(block));
  return vStatus; // BlockValidationStatus::valid
}

TxStatus State::validateTransaction(const TxBlock& tx) const {
  std::shared_lock lock(this->stateMutex_);
  return this->validateTransactionInternal(tx);
}

TxStatus State::addTx(TxBlock&& tx) {
  const auto txResult = this->validateTransaction(tx);
  if (txResult != TxStatus::ValidNew) return txResult;
  std::unique_lock lock(this->stateMutex_);
  auto txHash = tx.hash();
  this->mempool_.insert({txHash, std::move(tx)});
  LOGTRACE("Transaction: " + txHash.hex().get() + " was added to the mempool");
  return txResult; // should be TxStatus::ValidNew
}

TxStatus State::addValidatorTx(const TxValidator& tx) {
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
  this->accounts_[addr]->balance += uint256_t("1000000000000000000000");
}

Bytes State::ethCall(EncodedStaticCallMessage& msg) {
  // We actually need to lock uniquely here
  // As the contract host will modify (reverting in the end) the state.
  std::unique_lock lock(this->stateMutex_);
  const auto& accIt = this->accounts_.find(msg.to());
  if (accIt == this->accounts_.end()) {
    return {};
  }
  const auto& acc = accIt->second;
  try {
    if (acc->isContract()) {
      ExecutionContext context = ExecutionContext::Builder{}
      .storage(this->vmStorage_)
      .accounts(this->accounts_)
      .contracts(this->contracts_)
      .evmContracts(this->evmContracts_)
      .blockHash(Hash())
      .txHash(Hash())
      .txOrigin(msg.from())
      .blockCoinbase(ContractGlobals::getCoinbase())
      .txIndex(0)
      .blockNumber(ContractGlobals::getBlockHeight())
      .blockTimestamp(ContractGlobals::getBlockTimestamp())
      .blockGasLimit(10'000'000)
      .txGasPrice(0)
      .chainId(this->options_.getChainID())
      .build();

      // As we are simulating, the randomSeed can be anything
      const Hash randomSeed = bytes::random();

      return ContractHost(
        this->vm_,
        this->dumpManager_,
        this->storage_,
        randomSeed,
        context
      ).execute(msg);
    } else {
      return {};
    }
  } catch (VMExecutionError& e) {
    throw;
  } catch (std::exception& e) {
    throw VMExecutionError(-32603, std::string("Internal error: ") + e.what(), Bytes());
  }
}

int64_t State::estimateGas(EncodedMessageVariant msg) {
  std::unique_lock lock(this->stateMutex_);
  auto latestBlock = this->storage_.latest();
  try {
    std::unique_ptr<ExecutionContext> context;
    const EncodedCallMessage* callMessage = std::get_if<EncodedCallMessage>(&msg);
    const EncodedCreateMessage* createMessage = nullptr;
    if (callMessage) {
      context = ExecutionContext::Builder{}
        .storage(this->vmStorage_)
        .accounts(this->accounts_)
        .contracts(this->contracts_)
        .evmContracts(this->evmContracts_)
        .blockHash(latestBlock->getHash())
        .txHash(Hash())
        .txOrigin(callMessage->from())
        .blockCoinbase(Secp256k1::toAddress(latestBlock->getValidatorPubKey()))
        .txIndex(0)
        .blockNumber(latestBlock->getNHeight())
        .blockTimestamp(latestBlock->getTimestamp())
        .blockGasLimit(10'000'000)
        .txGasPrice(0)
        .chainId(this->options_.getChainID())
        .buildPtr();
    } else {
      createMessage = std::get_if<EncodedCreateMessage>(&msg);
      if (createMessage == nullptr) {
        throw DynamicException("Invalid message type for gas estimation");
      }
      context = ExecutionContext::Builder{}
        .storage(this->vmStorage_)
        .accounts(this->accounts_)
        .contracts(this->contracts_)
        .blockHash(latestBlock->getHash())
        .evmContracts(this->evmContracts_)
        .txHash(Hash())
        .txOrigin(createMessage->from())
        .blockCoinbase(Secp256k1::toAddress(latestBlock->getValidatorPubKey()))
        .txIndex(0)
        .blockNumber(latestBlock->getNHeight())
        .blockTimestamp(latestBlock->getTimestamp())
        .blockGasLimit(10'000'000)
        .txGasPrice(0)
        .chainId(this->options_.getChainID())
        .buildPtr();
    }

    const Hash randomSeed = bytes::random();
    ContractHost host(
      this->vm_,
      this->dumpManager_,
      this->storage_,
      randomSeed,
      *context
    );
    return std::visit([&host] (auto&& msg) {
      const Gas& gas = msg.gas();
      const int64_t initialGas(gas);
      host.simulate(std::forward<decltype(msg)>(msg));
      return int64_t((initialGas - int64_t(gas)) * 1.15);
    }, std::move(msg));
  } catch (VMExecutionError& e) {
    throw;
  } catch (std::exception& e) {
    throw VMExecutionError(-32603, std::string("Internal error: ") + e.what(), Bytes());
  }
}

std::vector<std::pair<std::string, Address>> State::getCppContracts() const {
  std::shared_lock lock(this->stateMutex_);
  std::vector<std::pair<std::string, Address>> contracts;
  for (const auto& [address, contract] : this->contracts_) {
    contracts.emplace_back(contract->getContractName(), address);
  }
  return contracts;
}

std::vector<Address> State::getEvmContracts() const {
  std::shared_lock lock(this->stateMutex_);
  std::vector<Address> contracts;
  for (const auto& [addr, acc] : this->accounts_) {
    if (acc->contractType == ContractType::EVM) contracts.emplace_back(addr);
  }
  return contracts;
}

Bytes State::getContractCode(const Address &addr) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->accounts_.find(addr);
  if (it == this->accounts_.end()) {
    return {};
  }
  // If its a PRECOMPILE contract, we need to return "PrecompileContract-CONTRACTNAME"
  // yes, inside a Bytes object, not a string object.
  if (it->second->contractType == ContractType::CPP) {
    auto contractIt = this->contracts_.find(addr);
    if (contractIt == this->contracts_.end()) {
      return {};
    }
    std::string precompileContract = "PrecompileContract-";
    precompileContract.append(contractIt->second->getContractName());
    return {precompileContract.begin(), precompileContract.end()};
  }
  if (it->second->code == nullptr) {
    return {};
  }
  return *it->second->code;
}