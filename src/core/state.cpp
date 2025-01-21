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

bool State::validateTransaction(const TxBlock& tx, bool affectsMempool, MempoolModel *mm) {
  std::unique_lock lock(this->stateMutex_);
  return validateTransactionInternal(tx, affectsMempool, mm);
}

void State::removeTxFromMempoolModel(const TxBlock& tx) {
  // ALWAYS operates on mempoolModel_, irrespective of whether validateTransactionInternal()
  // is called with a custom/temporary mempool model.
  // REVIEW: Here we could reuse iterators from the caller in case validateTransactionInternal()
  // is operating on mempoolModel_, which would be slightly faster, but just repeating the lookup
  // is simpler and safer.
  auto fromIt = mempoolModel_.find(tx.getFrom());
  if (fromIt == mempoolModel_.end()) {
    return;
  }
  uint64_t nonce64 = static_cast<uint64_t>(tx.getNonce());
  auto& nonceMap = fromIt->second;
  auto nonceIt = nonceMap.find(nonce64);
  if (nonceIt == nonceMap.end()) {
    return;
  }
  auto& hashMap = nonceIt->second;
  auto hashIt   = hashMap.find(tx.hash());
  if (hashIt != hashMap.end()) {
    hashMap.erase(hashIt);
  }
  if (hashMap.empty()) {
    nonceMap.erase(nonceIt);
  }
  if (nonceMap.empty()) {
    mempoolModel_.erase(fromIt);
  }
}

void State::ejectTxFromMempoolModel(const TxBlock& tx) {
  // ALWAYS operates on mempoolModel_, irrespective of whether validateTransactionInternal()
  // is called with a custom/temporary mempool model.
  // REVIEW: Here we could reuse iterators from the caller in case validateTransactionInternal()
  // is operating on mempoolModel_, which would be slightly faster, but just repeating the lookup
  // is simpler and safer.
  auto fromIt = mempoolModel_.find(tx.getFrom());
  if (fromIt == mempoolModel_.end()) {
    return; // Not in mempool => nothing to do
  }
  uint64_t nonce64 = static_cast<uint64_t>(tx.getNonce());
  auto nonceIt = fromIt->second.find(nonce64);
  if (nonceIt == fromIt->second.end()) {
    return; // Not in mempool => nothing to do
  }
  auto& hashMap = nonceIt->second;
  auto hashIt = hashMap.find(tx.hash());
  if (hashIt == hashMap.end()) {
    return; // Not in mempool => nothing to do
  }
  auto& [existingCost, isEjected] = hashIt->second;
  isEjected = true;
  LOGXTRACE(
    "handleRejectedTx: set isEjected=true for tx " + tx.hash().hex().get()
  );
}

bool State::validateTransactionInternal(const TxBlock& tx, bool affectsMempool, MempoolModel *mm) {
  // NOTE: Caller must have stateMutex_ locked.
  // NOTE: Signature of the originating account and other constraints are checked in TxBlock().

  // Set mm to &mempoolModel_ if mm == nullptr, so the code can use mm below.
  // All mempool model operations will be using the mm pointer, except for removeTxFromMempoolModel()
  //  and ejectTxFromMempoolModel() which *always* operate on mempoolModel_ and not the given temporary.
  if (mm == nullptr) {
    mm = &mempoolModel_;
  }

  // Verify if transaction already exists within the mempool, if on mempool, it has been validated previously.
  bool alreadyInMempool = false;  // Flag to indicate if we found the tx in mempool and it's valid
  auto fromIt = mm->find(tx.getFrom());
  if (fromIt != mm->end()) {
    uint64_t nonce64 = static_cast<uint64_t>(tx.getNonce());
    auto nonceIt = fromIt->second.find(nonce64);
    if (nonceIt != fromIt->second.end()) {
      const auto& hashMap = nonceIt->second;
      auto hashIt = hashMap.find(tx.hash());
      if (hashIt != hashMap.end()) {
        // Found an entry for (from, nonce, txHash)
        const std::pair<uint256_t, bool>& txEntry = hashIt->second;
        if (txEntry.second) {
          // Tx is marked as ejected
          LOGXTRACE(
            "Transaction: " + tx.hash().hex().get() +
            " is already in mempool but marked ejected. Failing immediately."
          );
          // If rejecting the transaction will affect the mempool, then we can
          // update our internal mempool model to exclude it.
          if (affectsMempool) {
            removeTxFromMempoolModel(tx);
          }
          return false;
        }
        // Otherwise, it's in mempool & not ejected => set flag
        LOGXTRACE(
          "Transaction: " + tx.hash().hex().get() +
          " is already in mempool and not ejected."
        );
        alreadyInMempool = true;
        // We will still re-validate the transaction; keep going.
      }
    }
  }

  // The transaction is invalid if the originator account simply doesn't exist.
  auto accountIt = this->accounts_.find(tx.getFrom());
  if (accountIt == this->accounts_.end()) {
    LOGERROR("Account doesn't exist (0 balance and 0 nonce)");
    if (affectsMempool) {
       if (alreadyInMempool) {
        removeTxFromMempoolModel(tx);
      }
    } else {
      if (alreadyInMempool) {
        ejectTxFromMempoolModel(tx);
      }
    }
    return false;
  }
  const uint256_t accBalance = accountIt->second->balance;
  const uint256_t accNonce = accountIt->second->nonce;

  const uint256_t txNonce = tx.getNonce();
  const uint64_t txNonceUint64 = static_cast<uint64_t>(txNonce);

  // If a block has multiple transactions for the same account within it, then
  //   transactions for nonces [account_nonce], [account_nonce+1], [account_nonce+2], etc.
  //   should all be considered individually valid.
  //   In other words, when checking if a *block* is valid, each transaction that is validated
  //   in sequence will create a speculative nonce cache that is specific to the block validation
  //   loop, which affects the result of checking whether a transaction is valid or not.
  // And finally, when preparing a block proposal (PrepareProposal), transactions should be
  //   reordered so that (a) [account_nonce], [account_nonce+1], [account_nonce+2] is respected
  //   and (b) if a nonce is skipped in a sequence, e.g. [account_nonce], [account_nonce+2], the
  //   proposal will exclude the transactions that have no way of being valid in that block (in
  //   that case, [account_nonce+2].

  // The transaction is invalid if the tx nonce is in the past.
  if (txNonce < accNonce) {
    LOGERROR("Transaction: " + tx.hash().hex().get()
      + " nonce mismatch, expected: " + accNonce.str()
      + " got: " + txNonce.str()
    );
    if (affectsMempool) {
       if (alreadyInMempool) {
        removeTxFromMempoolModel(tx);
      }
    } else {
      if (alreadyInMempool) {
        ejectTxFromMempoolModel(tx);
      }
    }
    return false;
  }

  // If the transaction is in the future, we need to find intermediary nonces already in
  // the RAM-based mempool model, otherwise we just reject them (client must just [re]send
  // the transactions with the missing nonces first).
  // Also, we need the sum of the costs of all these transactions that needs to be added to
  // the cost of *this* transaction to see if it can be afforded.
  uint256_t costOfAllPreviousTxs = 0;
  if (txNonce > accNonce) {
    if (fromIt == mm->end()) {
      LOGXTRACE(
        "Transaction: " + tx.hash().hex().get() +
        " nonce is in the future but no mempool entries exist for that account."
      );
      if (affectsMempool) {
        if (alreadyInMempool) {
          removeTxFromMempoolModel(tx);
        }
      } else {
        if (alreadyInMempool) {
          ejectTxFromMempoolModel(tx);
        }
      }
      return false;
    }

    // For each intermediary nonce, collect the maximum cost among all non-ejected txs
    const uint64_t accNonceUint64 = static_cast<uint64_t>(accNonceUint64);
    for (uint64_t inonce = accNonceUint64; inonce < txNonceUint64; ++inonce) {
      auto nonceIt = fromIt->second.find(inonce);
      if (nonceIt == fromIt->second.end()) {
        // No transaction for this nonce => can't fill the gap
        LOGXTRACE(
          "Transaction: " + tx.hash().hex().get() +
          " is missing intermediate nonce " + std::to_string(inonce)
        );
        if (affectsMempool) {
          if (alreadyInMempool) {
            removeTxFromMempoolModel(tx);
          }
        } else {
          if (alreadyInMempool) {
            ejectTxFromMempoolModel(tx);
          }
        }
        return false;
      }

      const auto& hashMap = nonceIt->second;
      bool foundValidTx = false;
      uint256_t maxCostForNonce = 0;

      // Among all txs for this (from, inonce), pick the max cost of any non-ejected one
      // This is a worst-case estimation: we assume the tx that will be picked will be
      // the worst in terms of cost.
      // REVIEW: instead, we could try to guess which one has the largest fee and thus
      // would be the actually prioritized one, and then use the cost (which includes
      // transfer value) when estimating the total cost of all txs within a nonce sequence.
      for (const auto& [hash, costEjectPair] : hashMap) {
        const auto& [cost, isEjected] = costEjectPair;
        if (!isEjected) {
          foundValidTx = true;
          if (cost > maxCostForNonce) {
            maxCostForNonce = cost;
          }
        }
      }

      if (!foundValidTx) {
        // All transactions for that nonce are ejected => no valid coverage for the gap
        LOGXTRACE(
          "Transaction: " + tx.hash().hex().get() +
          " cannot be validated because nonce " + std::to_string(inonce) +
          " is present, but all are ejected."
        );
        if (affectsMempool) {
          if (alreadyInMempool) {
            removeTxFromMempoolModel(tx);
          }
        } else {
          if (alreadyInMempool) {
            ejectTxFromMempoolModel(tx);
          }
        }
        return false;
      }

      costOfAllPreviousTxs += maxCostForNonce;
    }
  }

  // The transaction is invalid if the originator account cannot pay for the transaction
  // and all earlier transactions with earlier nonces for the same from account, if any.
  // (considering the worst case total cost at each nonce: value transferred + max fee).
  const uint256_t txWithFees = tx.getValue() + (tx.getGasLimit() * tx.getMaxFeePerGas());
  if ((txWithFees + costOfAllPreviousTxs) > accBalance) {
    LOGXTRACE(
      "Transaction sender: " + tx.getFrom().hex().get() +
      " doesn't have enough balance to send transaction. Required: " +
      (txWithFees + costOfAllPreviousTxs).str() + ", has: " + accBalance.str()
    );
    if (affectsMempool) {
       if (alreadyInMempool) {
        removeTxFromMempoolModel(tx);
      }
    } else {
      if (alreadyInMempool) {
        ejectTxFromMempoolModel(tx);
      }
    }
    return false;
  }

  // We are going to accept it, so make sure it is added to the mempool model.
  if (!alreadyInMempool) {
    auto& nonceMap = (*mm)[tx.getFrom()];
    auto& hashMap = nonceMap[txNonceUint64];
    hashMap[tx.hash()] = std::make_pair(txWithFees, false);
    LOGXTRACE(
      "Transaction: " + tx.hash().hex().get() +
      " added to mempool model (from=" + tx.getFrom().hex().get() +
      ", nonce=" + txNonce.str() +
      ", cost=" + txWithFees.str() + ")."
    );
  }

  // If the originating account can pay for it and the nonce is valid, then it is valid (that is,
  // it can be included in a block because it does not invalidate it).
  return true;
}

bool State::validateNextBlock(const FinalizedBlock& block) {
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

  // Validate all transactions using the current machine state and starting with a blank
  //  mempool model, so all transactions have to be valid as they are seen in the sequence
  //  they are given in the block.
  MempoolModel mm;
  for (const auto& txPtr : block.getTxs()) {
    const auto& tx = *txPtr;
    if (!validateTransactionInternal(tx, false, &mm)) {
      LOGERROR("Transaction " + tx.hash().hex().get() + " within block is invalid");
      return false;
    }
  }

  LOGTRACE("Block " + block.getHash().hex().get() + " is valid. (Sanity Check Passed)");
  return true;
}

void State::processBlock(const FinalizedBlock& block, std::vector<bool>& succeeded, std::vector<uint64_t>& gasUsed) {
  std::unique_lock lock(this->stateMutex_);

  // NOTE: Block should already have been validated by the caller.

  // Although block validation should have already been done, ensure
  // that the block height is the expected one.
  if (block.getNHeight() != height_ + 1) {
    throw DynamicException(
      "State::processBlock(): current height is " +
      std::to_string(height_) + " and block height is " +
      std::to_string(block.getNHeight())
    );
  }

  // The coinbase address that gets all the block fees, etc. is the block proposer.
  // Address derivation schemes (from the same Secp256k1 public key) differ between CometBFT and Eth.
  // So we need to map CometBFT Address to CometValidatorUpdate (a validator public key)
  //   and then use the validator public key to compute the correct Eth Address.
  Address proposerEthAddr = blockchain_.validatorCometAddressToEthAddress(block.getProposerAddr());
  ContractGlobals::coinbase_ = proposerEthAddr;
  LOGTRACE("Coinbase set to: " + proposerEthAddr.hex().get() + " (CometBFT Addr: " + block.getProposerAddr().hex().get() + ")");

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
  for (auto const& txPtr : block.getTxs()) {
    const TxBlock& tx = *txPtr;
    bool txSucceeded;
    uint64_t txGasUsed;
    this->processTransaction(tx, txIndex, blockHash, randomHash, txSucceeded, txGasUsed);
    succeeded.push_back(txSucceeded);
    gasUsed.push_back(txGasUsed);
    txIndex++;
  }

  // Update the state height after processing
  height_ = block.getNHeight();
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
  // Since we processed a transaction (that is in a finalized block, naturally), that means
  // this transaction is now gone from the mempool. Update the mempool model.
  removeTxFromMempoolModel(tx);
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
