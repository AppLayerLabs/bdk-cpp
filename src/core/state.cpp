/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <evmone/evmone.h>

#include "state.h"

#include "../contract/contracthost.h" // contractmanager.h

#include "../utils/uintconv.h"

#include "blockchain.h"

State::State(Blockchain& blockchain) : blockchain_(blockchain), vm_(evmc_create_evmone())
{
  // The State constructor sets the blockchain machine state to a default.
  // This default can then be modified when connecting to cometbft, as cometbft supports
  //   passing an arbitrary binary state blob via genesis.json. But deserializing and executing
  //   that genesis state blob comes after the initializations that are done here, which are
  //   only the initializations that are mandatory for the state of any BDK contract machine.

  std::unique_lock lock(this->stateMutex_);

  // we will now fill in here the default machine state for simulation time 0
  height_ = 0;

  // ContractManager account
  auto& contractManagerAcc = *this->accounts_[ProtocolContractAddresses.at("ContractManager")];
  contractManagerAcc.nonce = 1;
  contractManagerAcc.contractType = ContractType::CPP;

  // Insert the contract manager into the contracts_ map.
  this->contracts_[ProtocolContractAddresses.at("ContractManager")] = std::make_unique<ContractManager>(
    //db, this->contracts_, this->dumpManager_ ,this->options_
    this->contracts_, blockchain_.opt()
  );

  // State sanity check, lets check if all found contracts in the accounts_ map really have code or are C++ contracts
  for (const auto& [addr, acc] : this->accounts_) contractSanityCheck(addr, *acc);

  // FIXME/TODO
  // While we don't remove these global variables and make them members of State, lock them globally, set them,
  //  and process a whole block into the State instance that's running contract code.
  ContractGlobals::coinbase_ = Address();
  ContractGlobals::blockHash_ = Hash();
  ContractGlobals::blockHeight_ = height_;
  ContractGlobals::blockTimestamp_ = 0;
}

State::~State() {
  evmc_destroy(this->vm_);
}

std::string State::getLogicalLocation() const {
  return blockchain_.getLogicalLocation();
}

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
      if (acc.code.empty()) {
        LOGERROR("Contract " + addr.hex().get() + " is marked as EVM contract but doesn't have code");
        throw DynamicException("Contract " + addr.hex().get() + " is marked as EVM contract but doesn't have code");
      }
      break;
    }
    case ContractType::NOT_A_CONTRACT: {
      if (!acc.code.empty()) {
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

/*
    TODO: checkpointing State

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
    stateBatch.push_back(storageKey, storageValue, DBPrefix::vmStorage);
  }

  return stateBatch;
}
*/

void State::addBalance(const Address& addr) {
  std::unique_lock lock(this->stateMutex_);
  this->accounts_[addr]->balance += uint256_t("1000000000000000000000");
}

Bytes State::ethCall(const evmc_message& callInfo) {
  // We actually need to lock uniquely here
  // As the contract host will modify (reverting in the end) the state.
  std::unique_lock lock(this->stateMutex_);
  const auto recipient(callInfo.recipient);
  const auto& accIt = this->accounts_.find(recipient);
  if (accIt == this->accounts_.end()) {
    return {};
  }
  const auto& acc = accIt->second;
  if (acc->isContract()) {
    int64_t leftOverGas = callInfo.gas;
    evmc_tx_context txContext;
    txContext.tx_gas_price = {};
    txContext.tx_origin = callInfo.sender;
    txContext.block_coinbase = ContractGlobals::getCoinbase().toEvmcAddress();
    txContext.block_number = static_cast<int64_t>(ContractGlobals::getBlockHeight());
    txContext.block_timestamp = static_cast<int64_t>(ContractGlobals::getBlockTimestamp());
    txContext.block_gas_limit = 10000000;
    txContext.block_prev_randao = {};
    txContext.chain_id = EVMCConv::uint256ToEvmcUint256(blockchain_.opt().getChainID());
    txContext.block_base_fee = {};
    txContext.blob_base_fee = {};
    txContext.blob_hashes = nullptr;
    txContext.blob_hashes_count = 0;
    // As we are simulating, the randomSeed can be anything
    Hash randomSeed = Hash::random();
    return ContractHost(
      this->vm_,
      //this->dumpManager_,
      this->blockchain_.storage(),
      randomSeed,
      txContext,
      this->contracts_,
      this->accounts_,
      this->vmStorage_,
      Hash(),
      0,
      Hash(),
      leftOverGas
    ).ethCallView(callInfo, acc->contractType);
  } else {
    return {};
  }
}

int64_t State::estimateGas(const evmc_message& callInfo) {
  std::unique_lock lock(this->stateMutex_);
  const Address to = callInfo.recipient;
  // ContractHost simulate already do all necessary checks
  // We just need to execute and get the leftOverGas
  ContractType type = ContractType::NOT_A_CONTRACT;
  if (auto accIt = this->accounts_.find(to); accIt != this->accounts_.end()) {
    type = accIt->second->contractType;
  }

  int64_t leftOverGas = callInfo.gas;
  Hash randomSeed = Hash::random();
  ContractHost(
    this->vm_,
    //this->dumpManager_,
    blockchain_.storage(),
    randomSeed,
    evmc_tx_context(),
    this->contracts_,
    this->accounts_,
    this->vmStorage_,
    Hash(),
    0,
    Hash(),
    leftOverGas
  ).simulate(callInfo, type);
  auto left = callInfo.gas - leftOverGas;
  if (left < 0) {
    left = 0;
  }
  return left;
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
  return it->second->code;
}

bool State::validateTransaction(const TxBlock& tx) const {
  std::unique_lock lock(this->stateMutex_);
  return validateTransactionInternal(tx);
}

bool State::validateTransactionInternal(const TxBlock& tx) const {
  // NOTE: Caller must have stateMutex_ locked.

  // REVIEW: It *could* make sense to check in a transaction cache if the transaction has been validated
  //         before, but that is not a given.
  //
  // Membership into a mempool is no longer related to whether a tx is valid
  //
  // Verify if transaction already exists within the mempool, if on mempool, it has been validated previously.
  //if (this->mempool_.contains(tx.hash())) {
  //  LOGTRACE("Transaction: " + tx.hash().hex().get() + " already in mempool");
  //  return TxStatus::ValidExisting;
  //}

  // NOTE: Signature of the originating account and other constraints are checked in TxBlock().

  // The transaction is invalid if the originator account simply doesn't exist.
  auto accountIt = this->accounts_.find(tx.getFrom());
  if (accountIt == this->accounts_.end()) {
    LOGERROR("Account doesn't exist (0 balance and 0 nonce)");
    return false;
  }
  const auto accBalance = accountIt->second->balance;
  const auto accNonce = accountIt->second->nonce;

  // The transaction is invalid if the originator account cannot pay for the transaction.
  if (
    uint256_t txWithFees = tx.getValue() + (tx.getGasLimit() * tx.getMaxFeePerGas());
    txWithFees > accBalance
  ) {
    LOGERROR("Transaction sender: " + tx.getFrom().hex().get()
      + " doesn't have balance to send transaction"
      + " expected: " + txWithFees.str() + " has: " + accBalance.str()
    );
    return false;
  }

  // The transaction is invalid if the account's next available nonce is different from the
  //   transaction's own nonce. We only support one transaction per account per block for now.
  //
  // TODO: If a block has multiple transactions for the same account within it, then
  //   transactions for nonces [account_nonce], [account_nonce+1], [account_nonce+2], etc.
  //   should all be considered individually valid.
  //   In other words, when checking if a *block* is valid, each transaction that is validated
  //   in sequence will create a speculative nonce cache that is specific to the block validation
  //   loop, which affects the result of checking whether a transaction is valid or not. The way
  //   to do this is to pass this nonce cache as an optional in/out parameter to this function,
  //   so it will do context-dependent validation of transactions.
  //   And finally, when preparing a block proposal (PrepareProposal), transactions should be
  //   reordered so that (a) [account_nonce], [account_nonce+1], [account_nonce+2] is respected
  //   and (b) if a nonce is skipped in a sequence, e.g. [account_nonce], [account_nonce+2], the
  //   proposal will exclude the transactions that have no way of being valid in that block (in
  //   that case, [account_nonce+2].
  if (accNonce != tx.getNonce()) {
    LOGERROR("Transaction: " + tx.hash().hex().get()
      + " nonce mismatch, expected: " + std::to_string(accNonce)
      + " got: " + tx.getNonce().str()
    );
    return false;
  }

  // If the originating account can pay for it and the nonce is valid, then it is valid (that is,
  // it can be included in a block because it does not invalidate it).
  return true;
}

bool State::validateNextBlock(const FinalizedBlock& block) const {
  // NOTE: We don't have to validate the low-level block at consensus level since that's
  // now done by CometBFT. We just need to check if the transactions make sense for our
  // machine and its current state.

  // The block must be proposing up dates for the next machine simulation time (height + 1)
  if (block.getNHeight() != this->height_ + 1) {
    LOGERROR(
      "Block height doesn't match, expected " + std::to_string(this->height_ + 1) +
      " got " + std::to_string(block.getNHeight())
    );
    return false;
  }

  // Validate all transactions
  // TODO: context-aware tx validation
  for (const auto& tx : block.getTxs()) {
    if (!validateTransactionInternal(tx)) {
      LOGERROR("Transaction " + tx.hash().hex().get() + " within block is invalid");
      return false;
    }
  }

  LOGTRACE("Block " + block.getHash().hex().get() + " is valid. (Sanity Check Passed)");
  return true;
}

void State::processBlock(const FinalizedBlock& block, std::vector<bool>& succeeded, std::vector<uint64_t>& gasUsed) {
  std::unique_lock lock(this->stateMutex_);

  // NOTE:Block validation has to be done separately by the caller
  //
  // Sanity check - if it passes, the block is valid and will be processed
  //BlockValidationStatus vStatus = this->validateNextBlockInternal(block);
  //if (vStatus != BlockValidationStatus::valid) {
  //  return vStatus;
  //}

  // Although block validation should have already been done, ensure
  // that the block height is the expected one.
  if (block.getNHeight() != height_ + 1) {
    throw DynamicException(
      "State::processBlock(): current height is " +
      std::to_string(height_) + " and block height is " +
      std::to_string(block.getNHeight())
    );
  }

  // FIXME/TODO: Whoops! We have a problem with the coinbase = validator-address idea.
  //   Validator keys are ed25519 now, while the account structure of the
  //   Eth-compatible machine is using secp256k1. You can't "pay" the validator key
  //   directly.
  //ContractGlobals::coinbase_ = Secp256k1::toAddress(block.getValidatorPubKey());

  // TODO: randomHash needs a secure random gen protocol between all validators
  Hash randomHash; // 0

  const Hash blockHash = block.getHash();
  const uint64_t blockHeight = block.getNHeight();

  // Update contract globals based on (now) latest block
  ContractGlobals::blockHash_ = blockHash;
  ContractGlobals::blockHeight_ = blockHeight;
  ContractGlobals::blockTimestamp_ = block.getTimestamp();

  // Process transactions of the block within the current state
  uint64_t txIndex = 0;
  for (auto const& tx : block.getTxs()) {
    bool txSucceeded;
    uint64_t txGasUsed;
    this->processTransaction(tx, txIndex, blockHash, randomHash, txSucceeded, txGasUsed);
    succeeded.push_back(txSucceeded);
    gasUsed.push_back(txGasUsed);
    txIndex++;
  }

  // Update the state height after processing
  height_ = block.getNHeight();

  // REVIEW: may need to store on disk block metadata (not the entire block, but header info, hashes, etc.
  //   especially the ones that are computed on our side -- if we don't want to have to recompute this later,
  //   when the node is brought online and all the RAM caches of this info are empty (also a possibility -- just
  //   recompute this info later).
}

void State::processTransaction(
  const TxBlock& tx, const uint64_t& txIndex, const Hash& blockHash, const Hash& randomnessHash,
  bool& succeeded, uint64_t& gasUsed
) {
  // NOTE: Caller must have stateMutex_ locked.

  Account& accountFrom = *this->accounts_[tx.getFrom()];
  Account& accountTo = *this->accounts_[tx.getTo()];
  auto leftOverGas = int64_t(tx.getGasLimit());
  auto& fromNonce = accountFrom.nonce;
  auto& fromBalance = accountFrom.balance;

  // NOTE: All validation must have already been done.
  // The errors below only trigger if we have failed to properly validate the transaction.
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

  // Advance contract state using the given TxBlock.
  try {
    evmc_tx_context txContext;
    txContext.tx_gas_price = EVMCConv::uint256ToEvmcUint256(tx.getMaxFeePerGas());
    txContext.tx_origin = tx.getFrom().toEvmcAddress();
    txContext.block_coinbase = ContractGlobals::getCoinbase().toEvmcAddress();
    txContext.block_number = ContractGlobals::getBlockHeight();
    txContext.block_timestamp = ContractGlobals::getBlockTimestamp();
    txContext.block_gas_limit = 10000000;
    txContext.block_prev_randao = {};
    txContext.chain_id = EVMCConv::uint256ToEvmcUint256(this->blockchain_.opt().getChainID());
    txContext.block_base_fee = {};
    txContext.blob_base_fee = {};
    txContext.blob_hashes = nullptr;
    txContext.blob_hashes_count = 0;
    Hash randomSeed(UintConv::uint256ToBytes((randomnessHash.toUint256() + txIndex)));
    ContractHost host(
      this->vm_,
      //this->dumpManager_,
      blockchain_.storage(),
      randomSeed,
      txContext,
      this->contracts_,
      this->accounts_,
      this->vmStorage_,
      tx.hash(),
      txIndex,
      blockHash,
      leftOverGas
    );

    host.execute(tx.txToMessage(), accountTo.contractType);

    const auto& addTxData = host.getAddTxData();
    succeeded = addTxData.succeeded;
    gasUsed = addTxData.gasUsed;

  } catch (std::exception& e) {
    LOGERRORP("Transaction: " + tx.hash().hex().get() + " failed to process, reason: " + e.what());
  }
  if (leftOverGas < 0) {
    leftOverGas = 0; // We don't want to """refund""" gas due to negative gas
  }
  ++fromNonce;
  auto usedGas = tx.getGasLimit() - leftOverGas;
  fromBalance -= (usedGas * tx.getMaxFeePerGas());
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
