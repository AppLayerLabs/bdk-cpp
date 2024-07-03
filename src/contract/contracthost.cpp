#include "contracthost.h"
#include "dynamiccontract.h"

void ContractHost::transfer(const Address& from, const Address& to, const uint256_t& value) {
  // the from account **Must exist** on the unordered_map.
  // unordered_map references to values are valid **until** you insert a new element
  // So we can safely take a reference from it and create a reference from the to account.
  auto& fromAccount = accounts_[from];
  auto& toAccount = accounts_[to];
  auto& fromBalance = fromAccount->balance;
  auto& toBalance = toAccount->balance;
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

    for (const auto& contractPair : this->stack_.getContracts()) {
      const auto& [address, contract] = contractPair;
      if (contract != nullptr) {
        this->manager_.pushBack(dynamic_cast<Dumpable*>(contract));
      } else {
        // If the contract is nullptr, it means that it was a EVM contract, we need to link txHash and txIndex
        this->txToAddr_[txHash_] = address;
      }
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
    for (const auto& contractPair : this->stack_.getContracts()) {
      const auto& address = std::get<0>(contractPair);
      this->accounts_.erase(address);
      this->contracts_.erase(address);
    }
    // Thirdly, revert all storage changes, erasing them if the key was (0x00)
    for (const auto& [key, value] : this->stack_.getStorage()) {
      if (value == Hash()) {
        // If the storage key was empty, we must erase it.
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
        accountIt->second->balance = balance;
      }
    }
    // Finally, revert nonce changes
    for (const auto& [address, nonce] : this->stack_.getNonce()) {
      // Make sure we don't create a new account if it doesn't exist (deleted as it was a newly created contract)
      auto accountIt = this->accounts_.find(address);
      if (accountIt != this->accounts_.end()) {
        accountIt->second->nonce = nonce;
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

evmc::Result ContractHost::createEVMContract(const evmc_message& msg, const Address& contractAddr, const evmc_call_kind& kind) {
  assert (kind == evmc_call_kind::EVMC_CREATE || kind == evmc_call_kind::EVMC_CREATE2);
  // Create a new contract
  auto createMsg = msg;

  createMsg.recipient = contractAddr.toEvmcAddress();
  createMsg.kind = kind;
  createMsg.input_data = nullptr;
  createMsg.input_size = 0;
  auto result = evmc::Result(evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
                 evmc_revision::EVMC_LATEST_STABLE_REVISION, &createMsg,
                 msg.input_data, msg.input_size));

  this->leftoverGas_ = result.gas_left; // gas_left is not linked with leftoverGas_, we need to link it.
  this->deduceGas(100000);
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
  return evmc::Result{result.status_code, this->leftoverGas_, 0, msg.recipient};
}

evmc::Result ContractHost::processBDKPrecompile(const evmc_message& msg) const {
  /**
  *  interface BDKPrecompile {
  *    function getRandom() external view returns (uint256);
  *  }
  */
  try {
    if (msg.input_size < 4) {
      throw DynamicException("ContractHost processBDKPrecompile: invalid input size");
    }
    Functor f = Utils::getFunctor(msg);
    // We only have one function on the BDKD precompile
    // getRandom() == 0xaacc5a17 == 2865519127
    if (f.value == 2865519127) {
      auto random = this->randomGen_.operator()();
      auto ret = Utils::uint256ToBytes(random);
      return evmc::Result(EVMC_SUCCESS, this->leftoverGas_, 0, ret.data(), ret.size());
    }
    throw DynamicException("ContractHost processBDKPrecompile: invalid function selector");
  } catch (std::exception &e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return evmc::Result(EVMC_PRECOMPILE_FAILURE, this->leftoverGas_, 0, nullptr, 0);
  }
}

void ContractHost::execute(const evmc_message& msg, const ContractType& type) {
  const Address from(msg.sender);
  const Address to(msg.recipient);
  const uint256_t value(Utils::evmcUint256ToUint256(msg.value));
  if (value) {
    this->transfer(from, to, value);
  }
  try {
    if (to == Address()) {
      // If the destination address of the transaction is 0x00, it means that we are creating a new contract
      auto contractAddress = this->deriveContractAddress(this->getNonce(from), from);
      if (this->accounts_.contains(contractAddress)) {
        throw DynamicException("ContractHost create/execute: contract already exists");
      }
      this->createEVMContract(msg, contractAddress, EVMC_CREATE);
    } else {
      switch (type) {
        case ContractType::CPP: {
          auto contractIt = this->contracts_.find(to);
          if (contractIt == this->contracts_.end()) {
            throw DynamicException("contract not found");
          }
          this->setContractVars(contractIt->second.get(), from, value);
          contractIt->second->ethCall(msg, this);
          break;
        }
        case ContractType::EVM: {
          // Execute a EVM contract.
          auto result = evmc::Result(evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
                     evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
                      this->accounts_[to]->code.data(), this->accounts_[to]->code.size()));
          this->leftoverGas_ = result.gas_left; // gas_left is not linked with leftoverGas_, we need to link it.
          if (result.status_code) {
            // Set the leftOverGas_ to the gas left after the execution
            throw DynamicException("Error when executing EVM contract, EVM status code: " +
              std::string(evmc_status_code_to_string(result.status_code)) + " bytes: " +
              Hex::fromBytes(Utils::cArrayToBytes(result.output_data, result.output_size)).get());
          }
          break;
        }
      }
    }
    // Take out 21000 gas Limit from the tx
    this->deduceGas(21000);
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

Bytes ContractHost::ethCallView(const evmc_message& msg, const ContractType& type) {
  Bytes ret;
  //const Address from(tx.sender);
  const Address to(msg.recipient);
  //const uint256_t value(Utils::evmcUint256ToUint256(tx.value));
  try {
    switch (type) {
      case ContractType::CPP: {
        auto contractIt = this->contracts_.find(to);
        if (contractIt == this->contracts_.end()) {
          throw DynamicException("contract not found");
        }
        ret = contractIt->second->ethCallView(msg, this);
        break;
      }
      case ContractType::EVM: {
        // Execute a EVM contract.
        auto result = evmc::Result(evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
                   evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
                    this->accounts_[to]->code.data(), this->accounts_[to]->code.size()));
        this->leftoverGas_ = result.gas_left;
        if (result.status_code) {
          // Set the leftOverGas_ to the gas left after the execution
          throw DynamicException("Error when executing (view) EVM contract, EVM status code: " +
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


void ContractHost::simulate(const evmc_message& msg, const ContractType& type) {
  this->execute(msg, type);
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
    auto& storageValue = vmStorage_[storageKey];
    this->stack_.registerStorageChange(storageKey, storageValue);
    storageValue = hashValue;
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
      return Utils::uint256ToEvmcUint256(it->second->balance);
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
      return it->second->code.size();
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
      return it->second->codeHash.toEvmcBytes32();
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

    const auto& code = it->second->code;

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

// EVM -> EVM calls don't need to use this->leftOverGas_ as the final
// evmc::Result will have the gas left after the execution
evmc::Result ContractHost::call(const evmc_message& msg) noexcept {
  // Check against bdk static precompiles
  std::cout << "Calling ContractHost::call" << std::endl;
  std::cout << Address(msg.recipient).hex() << std::endl;
  std::cout << msg.kind << std::endl;

  try {
    if (msg.kind == EVMC_CREATE) {
      auto fromAddress = Address(msg.sender);
      auto& fromNonce = this->getNonce(fromAddress);
      this->stack_.registerNonce(fromAddress, fromNonce);
      auto derivedContractAddress = this->deriveContractAddress(fromNonce, fromAddress);
      ++fromNonce;
      uint256_t value = Utils::evmcUint256ToUint256(msg.value);
      if (value) {
        this->transfer(fromAddress, derivedContractAddress, value);
      }
      std::cout << "Calling createEVMContract to: " << derivedContractAddress.hex() << std::endl;
      return this->createEVMContract(msg, derivedContractAddress, EVMC_CREATE);
    }
    if (msg.kind == EVMC_CREATE2) {

    }
  } catch (const std::exception &e) {
    this->evmcThrows_.emplace_back(e.what());
    this->evmcThrow_ = true;
    return evmc::Result(EVMC_REVERT, this->leftoverGas_, 0, nullptr, 0);
  }

  this->leftoverGas_ = msg.gas;
  if (msg.recipient == BDK_PRECOMPILE) {
    this->deduceGas(1000); // CPP contract call is 1000 gas
    return this->processBDKPrecompile(msg);
  }

  Address recipient(msg.recipient);
  auto &recipientAccount = *accounts_[recipient]; // We need to take a reference to the account, not a reference to the pointer.
  /// evmc::Result constructor is: _status_code + _gas_left + _output_data + _output_size
  if (recipientAccount.contractType == CPP) {
    // Uh we are an CPP contract, we need to call the contract evmEthCall function and put the result into a evmc::Result
    try {
      this->deduceGas(1000); // CPP contract call is 1000 gas
      auto& contract = contracts_[recipient];
      if (contract == nullptr) {
        throw DynamicException("ContractHost call: contract not found");
      }
      this->setContractVars(contract.get(), Address(msg.sender), Utils::evmcUint256ToUint256(msg.value));
      Bytes ret = contract->evmEthCall(msg, this);
      return evmc::Result(EVMC_SUCCESS, this->leftoverGas_, 0, ret.data(), ret.size());
    } catch (std::exception& e) {
      this->evmcThrows_.emplace_back(e.what());
      this->evmcThrow_ = true;
      return evmc::Result(EVMC_PRECOMPILE_FAILURE, this->leftoverGas_, 0, nullptr, 0);
    }
  }
  evmc::Result result (evmc_execute(this->vm_, &this->get_interface(), this->to_context(),
           evmc_revision::EVMC_LATEST_STABLE_REVISION, &msg,
           recipientAccount.code.data(), recipientAccount.code.size()));
  this->leftoverGas_ = result.gas_left; // gas_left is not linked with leftoverGas_, we need to link it.
  this->deduceGas(5000); // EVM contract call is 5000 gas
  result.gas_left = this->leftoverGas_; // We need to set the gas left to the leftoverGas_
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
    return it->second->balance;
  }
  return 0;
}

void ContractHost::sendTokens(const BaseContract* from, const Address& to, const uint256_t& amount) {
  this->transfer(from->getContractAddress(), to, amount);
}

uint64_t& ContractHost::getNonce(const Address& nonce) {
  return this->accounts_[nonce]->nonce;
}

void ContractHost::registerNewCPPContract(const Address& address, BaseContract* contract) {
  Account contractAcc;
  contractAcc.contractType = ContractType::CPP;
  contractAcc.nonce = 1;
  auto emplace = this->accounts_.try_emplace(address, contractAcc);
  if (!emplace.second) {
    throw DynamicException("ContractHost registerNewCPPContract: account on address already exists");
  }
  this->stack_.registerContract(address, contract);
}

void ContractHost::registerNewEVMContract(const Address& address, const uint8_t* code, size_t codeSize) {
  Account contractAcc;
  contractAcc.contractType = ContractType::EVM;
  contractAcc.nonce = 1;
  contractAcc.code = Bytes(code, code + codeSize);
  contractAcc.codeHash = Utils::sha3(contractAcc.code);
  auto emplace = this->accounts_.try_emplace(address, contractAcc);
  if (!emplace.second) {
    throw DynamicException("ContractHost registerNewCPPContract: account on address already exists");
  }
  this->stack_.registerContract(address, nullptr);
}


void ContractHost::registerVariableUse(SafeBase& variable) {
  this->stack_.registerVariableUse(variable);
}