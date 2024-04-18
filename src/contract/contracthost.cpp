#include "contracthost.h"
#include "dynamiccontract.h"

void ContractHost::transfer(const Address& from, const Address& to, const uint256_t& value) {
  // the from account **Must exist** on the unordered_map.
  // unordered_map references to values are valid **until** you insert a new element
  // So we can safely take a reference from it and create a reference from the to account.
  auto& fromAccount = accounts_[from];
  auto& toAccount = accounts_[to];
  auto& fromBalance = fromAccount.balance;
  auto& toBalance = toAccount.balance;
  if (fromBalance < value) {
    throw DynamicException("ContractHost transfer: insufficient funds");
  }
  this->stack_.registerBalance(from, fromBalance);
  this->stack_.registerBalance(to, toBalance);
  fromBalance -= value;
  toBalance += value;
}

ContractHost::~ContractHost() {
  if (!this->mustRevert_) {
    // When the execution is complete and we need to commit the changes to the storage we must do the following steps:
    // - Commit all the SafeBase variables
    // - Commit all the emitted events to the EventManager
    // We only need to commit the C++ stack, EVM operates directly on the storage (only requiring reverts)
    // There is no permanent temporary storage for the EVM stack like on the SaveBase variables
    // Instead, we use a journaling system with ContractStack to store the original values of the this->storage_
    // TODO: Maybe we should apply the same logic to the C++ stack as well somehow
    for (auto& var : this->stack_.getUsedVars()) {
      var.get().commit();
    }

    for (auto&& event : this->stack_.getEvents()) {
      this->eventManager_.registerEvent(std::move(event));
    }
  } else {
    // When reverting, we must revert all the changes, that means:
    // - Revert all the SafeBase variables
    // - Remove newly created contracts (if any)
    // - Revert all the storage changes
    // - Revert all balance changes
    // - Revert all nonce changes (contracts creating contracts)
    // First, lets revert all the SafeBase variables
    for (auto& var : this->stack_.getUsedVars()) {
      var.get().revert();
    }
    // Then lets clear off newly created contracts
    for (const auto& address : this->stack_.getContracts()) {
      this->accounts_.erase(address);
      this->contracts_.erase(address);
    }
    // Thirdly, revert all storage changes, erasing them if the key was (0x00)
    for (const auto& [key, value] : this->stack_.getStorage()) {
      if (value == Hash()) {
        this->vmStorage_.erase(key);
      } else {
        this->vmStorage_[key] = value;
      }
    }
    // Fourtly, revert all balance changes
    for (const auto& [address, balance] : this->stack_.getBalance()) {
      // Make sure we don't create a new account if it doesn't exist (deleted as it was a newly created contract)
      auto accountIt = this->accounts_.find(address);
      if (accountIt != this->accounts_.end()) {
        accountIt->second.balance = balance;
      }
    }
    // Finally, revert nonce changes
    for (const auto& [address, nonce] : this->stack_.getNonce()) {
      // Make sure we don't create a new account if it doesn't exist (deleted as it was a newly created contract)
      auto accountIt = this->accounts_.find(address);
      if (accountIt != this->accounts_.end()) {
        accountIt->second.nonce = nonce;
      }
    }
  }
}

Address ContractHost::deriveContractAddress(const uint64_t& nonce, const Address& address) {
  // Contract address is last 20 bytes of sha3 ( rlp ( tx from address + tx nonce ) )
  uint8_t rlpSize = 0xc0;
  rlpSize += 20;
  // As we don't have actually access to the nonce, we will use the number of contracts existing in the chain
  rlpSize += (nonce < 0x80)
    ? 1 : 1 + Utils::bytesRequired(nonce);
  Bytes rlp;
  rlp.insert(rlp.end(), rlpSize);
  rlp.insert(rlp.end(), address.cbegin(), address.cend());
  rlp.insert(rlp.end(), (nonce < 0x80)
    ? (char)nonce
    : (char)0x80 + Utils::bytesRequired(nonce)
  );
  return {Utils::sha3(rlp).view(12)};
}

void ContractHost::createEVMContract(evmc_message& msg, const Address& contractAddr) {
// Create a new contract
  auto result = evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
                 evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
                  nullptr, 0);
  this->leftoverGas_ = result.gas_left; // gas_left is not linked with leftoverGas_, we need to link it.
  this->leftoverGas_ -= 10000; // We take 10k instead of calculating based on contract size, regardless if we succeed or not
  if (result.status_code) {
    // Set the leftOverGas_ to the gas left after the execution
    throw DynamicException("Error when creating EVM contract, EVM status code: " +
      std::string(evmc_status_code_to_string(result.status_code)) + " bytes: " +
      Hex::fromBytes(Utils::cArrayToBytes(result.output_data, result.output_size)).get());
  }

  if (result.output_size > 50000) {
    throw DynamicException("ContractHost createEVMContract: contract code too large");
  }

  this->registerNewEVMContract(contractAddr, result.output_data, result.output_size);
}

void ContractHost::execute(const ethCallInfo& tx, const ContractType& type) {
  const auto& [from, to, gasLimit, gasPrice, value, functor, data, fullData] = tx;
  if (type == ContractType::NOT_A_CONTRACT) {
    throw DynamicException("ContractHost execute: trying to execute a non-contract");
  }
  /// Obligatory = take out 21000 gas from the transaction
  this->deduceGas(21000);
  if (value) {
    this->transfer(from, to, value);
  }

  if (to == Address()) {
    // If the destination address of the transaction is 0x00, it means that we are creating a new contract
    auto contractAddress = this->deriveContractAddress(this->getNonce(from), from);
    if (this->accounts_.contains(contractAddress)) {
      throw DynamicException("ContractHost create/execute: contract already exists");
    }
    evmc_message msg;
    msg.kind = evmc_call_kind::EVMC_CREATE;
    msg.flags = 0;
    msg.gas = static_cast<int64_t>(this->leftoverGas_);
    msg.recipient = contractAddress.toEvmcAddress();
    msg.sender = from.toEvmcAddress();
    msg.input_data = fullData.data();
    msg.input_size = fullData.size();
    msg.value = Utils::uint256ToEvmcUint256(value);
    msg.create2_salt = {};
    msg.depth = 1;
    msg.code_address = to.toEvmcAddress();
    this->createEVMContract(msg, contractAddress);
    return;
  }

  try {
    switch (type) {
      case ContractType::CPP: {
        auto contractIt = this->contracts_.find(to);
        if (contractIt == this->contracts_.end()) {
          throw DynamicException("contract not found");
        }
        contractIt->second->ethCall(tx, this);
        break;
      }
      case ContractType::EVM: {
        // Execute a EVM contract.
        evmc_message msg;
        msg.kind = evmc_call_kind::EVMC_CALL;
        msg.flags = 0;
        msg.gas = static_cast<int64_t>(this->leftoverGas_);
        msg.recipient = to.toEvmcAddress();
        msg.sender = from.toEvmcAddress();
        msg.input_data = fullData.data();
        msg.input_size = fullData.size();
        msg.value = Utils::uint256ToEvmcUint256(value);
        msg.create2_salt = {};
        msg.depth = 1;
        msg.code_address = to.toEvmcAddress();
        /// TODO: We have a problem here
        /// as you might know, using unordered_map::operator[] and then insert a new element (e.g. the contract call
        /// creates another contract, effectively creating a new account on the accounts_ unordered_map)
        /// it may **invalidate** all the references to the elements of the unordered_map
        /// that **includes** this->accounts_[to].code.data() and this->accounts_[to].code.size()
        /// so we must find a way to fix this
        auto result = evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
                   evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
                    this->accounts_[to].code.data(), this->accounts_[to].code.size());

        if (result.status_code) {
          // Set the leftOverGas_ to the gas left after the execution
          this->leftoverGas_ = result.gas_left;
          throw DynamicException("Error when executing EVM contract, EVM status code: " +
            std::string(evmc_status_code_to_string(result.status_code)) + " bytes: " +
            Hex::fromBytes(Utils::cArrayToBytes(result.output_data, result.output_size)).get());
        }
        break;
      }
    }
  } catch (const std::exception &e) {
    std::string what = std::string("ContractHost execution failed: ") + e.what();
    what += " OTHER INFO: ";
    for (const auto& evmcError : this->evmcThrows_) {
      what += evmcError;
      what += " --- OTHER INFO: --- ";
    }
    throw DynamicException(what);
  }
  // We only set that we don't revert, if EVMC didn't throw a exception
  if (this->evmcThrow_) {
    std::string what = std::string("ContractHost execution failed: EVMC threw an exception: ");
    for (const auto& evmcError : this->evmcThrows_) {
      what += evmcError;
      what += " --- OTHER INFO: --- ";
    }
    throw DynamicException(what);
  }
  this->mustRevert_ = false;
}

Bytes ContractHost::ethCallView(const ethCallInfo& tx, const ContractType& type) {
  Bytes ret;
  const auto& [from, to, gasLimit, gasPrice, value, functor, data, fullData] = tx;
  try {
    switch (type) {
      case ContractType::CPP: {
        auto contractIt = this->contracts_.find(to);
        if (contractIt == this->contracts_.end()) {
          throw DynamicException("contract not found");
        }
        ret = contractIt->second->ethCallView(tx, this);
        break;
      }
      case ContractType::EVM: {
        // Execute a EVM contract.
        evmc_message msg;
        msg.kind = evmc_call_kind::EVMC_CALL;
        msg.flags = 0;
        msg.gas = static_cast<int64_t>(this->leftoverGas_);
        msg.recipient = to.toEvmcAddress();
        msg.sender = from.toEvmcAddress();
        msg.input_data = fullData.data();
        msg.input_size = fullData.size();
        msg.value = Utils::uint256ToEvmcUint256(value);
        msg.create2_salt = {};
        msg.depth = 1;
        msg.code_address = to.toEvmcAddress();
        /// TODO: We have a problem here
        /// as you might know, using unordered_map::operator[] and then insert a new element (e.g. the contract call
        /// creates another contract, effectively creating a new account on the accounts_ unordered_map)
        /// it may **invalidate** all the references to the elements of the unordered_map
        /// that **includes** this->accounts_[to].code.data() and this->accounts_[to].code.size()
        /// so we must find a way to fix this
        auto result = evmc::Result(evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
                   evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
                    this->accounts_[to].code.data(), this->accounts_[to].code.size()));

        if (result.status_code) {
          // Set the leftOverGas_ to the gas left after the execution
          this->leftoverGas_ = result.gas_left;
          throw DynamicException("Error when executing EVM contract, EVM status code: " +
            std::string(evmc_status_code_to_string(result.status_code)) + " bytes: " +
            Hex::fromBytes(Utils::cArrayToBytes(result.output_data, result.output_size)).get());
        }
        ret = Bytes(result.output_data, result.output_data + result.output_size);
        break;
      }
    }
  } catch (const std::exception &e) {
    std::string what = std::string("ContractHost execution failed: ") + e.what();
    what += " OTHER INFO: ";
    for (const auto& evmcError : this->evmcThrows_) {
      what += evmcError;
      what += " --- OTHER INFO: --- ";
    }
    throw DynamicException(what);
  }
  // We only set that we don't revert, if EVMC didn't throw a exception
  if (this->evmcThrow_) {
    std::string what = std::string("ContractHost execution failed: EVMC threw an exception: ");
    for (const auto& evmcError : this->evmcThrows_) {
      what += evmcError;
      what += " --- OTHER INFO: --- ";
    }
    throw DynamicException(what);
  }
  return ret;
}


void ContractHost::simulate(const ethCallInfo& tx, const ContractType& type) {
  this->execute(tx, type);
  // We should set the revert flag to true, as we are only simulating the execution
  this->mustRevert_ = true;
}

bool ContractHost::account_exists(const evmc::address& addr) const noexcept {
  if (accounts_.find(addr) != accounts_.end()) {
    return true;
  }
  return false;
}

evmc::bytes32 ContractHost::get_storage(const evmc::address& addr, const evmc::bytes32& key) const noexcept {
  StorageKey storageKey(addr, key);
  try {
    auto it = vmStorage_.find(storageKey);
    if (it != vmStorage_.end()) {
      return it->second.toEvmcBytes32();
    }
    return {};
  } catch (const std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return {};
  }
}

// on SET_STORAGE, we can return multiple types of evmc_storage_status
// But we simply say that the storage was modified ;)
// TODO: Make it EIP-2200 compliant
evmc_storage_status ContractHost::set_storage(const evmc::address& addr, const evmc::bytes32& key, const evmc::bytes32& value) noexcept {
  StorageKey storageKey(addr, key);
  try {
    Hash hashValue(value);
    this->stack_.registerStorageChange(storageKey, hashValue);
    vmStorage_[storageKey] = hashValue;
    return EVMC_STORAGE_MODIFIED;
  } catch (const std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return EVMC_STORAGE_MODIFIED;
  }
}

evmc::uint256be ContractHost::get_balance(const evmc::address& addr) const noexcept {
  try {
    auto it = accounts_.find(addr);
    if (it != accounts_.end()) {
      return Utils::uint256ToEvmcUint256(it->second.balance);
    }
    return {};
  } catch (const std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return {};
  }
}

size_t ContractHost::get_code_size(const evmc::address& addr) const noexcept {
  try {
    auto it = accounts_.find(addr);
    if (it != accounts_.end()) {
      return it->second.code.size();
    }
    return 0;
  } catch (const std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return 0;
  }
}

evmc::bytes32 ContractHost::get_code_hash(const evmc::address& addr) const noexcept {
  try {
    auto it = accounts_.find(addr);
    if (it != accounts_.end()) {
      return it->second.codeHash.toEvmcBytes32();
    }
    return {};
  } catch (const std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return {};
  }
}

size_t ContractHost::copy_code(const evmc::address& addr, size_t code_offset, uint8_t* buffer_data, size_t buffer_size) const noexcept {
  try {
    const auto it = this->accounts_.find(addr);
    if (it == this->accounts_.end())
      return 0;

    const auto& code = it->second.code;

    if (code_offset >= code.size())
      return 0;

    const auto n = std::min(buffer_size, code.size() - code_offset);

    if (n > 0)
      std::copy_n(&code[code_offset], n, buffer_data);

    return n;
  } catch (std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    std::cerr << e.what() << std::endl;
    this->evmcThrow_ = true;
    return 0;
  }
}

bool ContractHost::selfdestruct(const evmc::address& addr, const evmc::address& beneficiary) noexcept {
  this->evmcThrow_ = true; // SELFDESTRUCT is not allowed in the current implementation
  return false;
}

evmc::Result ContractHost::call(const evmc_message& msg) noexcept {
  // TODO: WE NEED TO COPY THE CODE INSTEAD OF TAKING FROM THE MAP!
  // the VM call might insert new items into the accounts_ map, invalidating the references
  evmc::Result result (evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
           evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
           accounts_[msg.recipient].code.data(), accounts_[msg.recipient].code.size()));
  return result;
}

evmc_tx_context ContractHost::get_tx_context() const noexcept {
  return this->currentTxContext_;
}

evmc::bytes32 ContractHost::get_block_hash(int64_t number) const noexcept {
  try {
    return Utils::uint256ToEvmcUint256(number);
  } catch (std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return {};
  }
}

void ContractHost::emit_log(const evmc::address& addr, const uint8_t* data, size_t data_size, const evmc::bytes32 topics[], size_t topics_count) noexcept {
  // TODO: Implement after integrating with state
  try {
    // We need the following arguments to build a event:
    // (std::string) name The event's name.
    // (uint64_t) logIndex The event's position on the block.
    // (Hash) txHash The hash of the transaction that emitted the event.
    // (uint64_t) txIndex The position of the transaction in the block.
    // (Hash) blockHash The hash of the block that emitted the event.
    // (uint64_t) blockIndex The height of the block.
    // (Address) address The address that emitted the event.
    // (Bytes) data The event's arguments.
    // (std::vector<Hash>) topics The event's indexed arguments.
    // (bool) anonymous Whether the event is anonymous or not.
    std::vector<Hash> topics_;
    for (uint64_t i = 0; i < topics_count; i++) {
      topics_.emplace_back(topics[i]);
    }
    Event event("", // EVM events do not have names
      this->eventIndex_,
      this->txHash_,
      this->txIndex_,
      this->blockHash_,
      this->currentTxContext_.block_number,
      addr,
      Bytes(data, data + data_size),
      topics_,
      (topics_count == 0)
    );
    ++this->eventIndex_;
    this->stack_.registerEvent(std::move(event));
  } catch (std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
  }
}

// We always return warm because we are warm (storage is on ram) ;)
evmc_access_status ContractHost::access_account(const evmc::address& addr) noexcept {
  return EVMC_ACCESS_WARM;
}

// Same as above
evmc_access_status ContractHost::access_storage(const evmc::address& addr, const evmc::bytes32& key) noexcept {
  return EVMC_ACCESS_WARM;
}

evmc::bytes32 ContractHost::get_transient_storage(const evmc::address &addr, const evmc::bytes32 &key) const noexcept {
  StorageKey storageKey(addr, key);
  try {
    auto it = transientStorage_.find(storageKey);
    if (it != transientStorage_.end()) {
      return it->second.toEvmcBytes32();
    }
    return {};
  } catch (const std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return {};
  }
}

void ContractHost::set_transient_storage(const evmc::address &addr, const evmc::bytes32 &key, const evmc::bytes32 &value) noexcept {
  StorageKey storageKey(addr, key);
  try {
    Hash hashValue(value);
    transientStorage_[storageKey] = hashValue;
  } catch (const std::exception& e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
  }
}

void ContractHost::emitContractEvent(Event&& event) {
  this->stack_.registerEvent(std::move(event));
}

uint256_t ContractHost::getBalanceFromAddress(const Address& address) const {
  auto it = this->accounts_.find(address);
  if (it != this->accounts_.end()) {
    return it->second.balance;
  }
  return 0;
}

void ContractHost::sendTokens(const BaseContract* from, const Address& to, const uint256_t& amount) {
  this->transfer(from->getContractAddress(), to, amount);
}

uint64_t ContractHost::getNonce(const Address& nonce) const {
  auto it = this->accounts_.find(nonce);
  if (it == this->accounts_.end()) {
    return 0;
  }
  return it->second.nonce;
}

void ContractHost::registerNewCPPContract(const Address& address) {
  Account contractAcc;
  contractAcc.contractType = ContractType::CPP;
  contractAcc.nonce = 1;
  auto emplace = this->accounts_.try_emplace(address, contractAcc);
  if (!emplace.second) {
    throw DynamicException("ContractHost registerNewCPPContract: account on address already exists");
  }
  this->stack_.registerContract(address);
}

void ContractHost::registerNewEVMContract(const Address& address, const uint8_t* code, size_t codeSize) {
  auto& contractAcc = this->accounts_[address];
  contractAcc.contractType = ContractType::EVM;
  contractAcc.nonce = 1;
  contractAcc.code = Bytes(code, code + codeSize);
  contractAcc.codeHash = Utils::sha3(contractAcc.code);
  this->stack_.registerContract(address);
}


void ContractHost::registerVariableUse(SafeBase& variable) {
  this->stack_.registerVariableUse(variable);
}