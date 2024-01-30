/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CONTRACTMANAGER_H
#define CONTRACTMANAGER_H

#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "abi.h"
#include "contract.h"
#include "contractcalllogger.h"
#include "event.h"
#include "variables/safeunorderedmap.h"

#include "../utils/db.h"
#include "../utils/options.h"
#include "../utils/safehash.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"
#include "../utils/contractreflectioninterface.h"

// Forward declarations.
class rdPoS;
class State;
class ContractFactory;
class ContractManagerInterface;

/**
 * Addresses for the contracts that are deployed at protocol level (contract name -> contract address).
 * That means these contracts are deployed at the beginning of the chain.
 * They cannot be destroyed nor dynamically deployed like other contracts.
 * Instead, they are deployed in the constructor of State.
 */
const std::unordered_map<std::string, Address> ProtocolContractAddresses = {
  {"rdPoS", Address(Hex::toBytes("0xb23aa52dbeda59277ab8a962c69f5971f22904cf"))},           // Sha3("randomDeterministicProofOfStake").substr(0,20)
  {"ContractManager", Address(Hex::toBytes("0x0001cb47ea6d8b55fe44fdd6b1bdb579efb43e61"))}  // Sha3("ContractManager").substr(0,20)
};

/**
 * Class that holds all current contract instances in the blockchain state.
 * Responsible for creating and deploying contracts in the chain.
 * Also acts as an access point for contracts to access each other.
 */
class ContractManager : BaseContract {
  private:
    /**
     * Raw pointer to the blockchain state object.
     * Used if the contract is a payable function.
     * Can be `nullptr` due to tests not requiring state (contract balance).
     */
    State* state_;

    /// List of currently deployed contracts.
    std::unordered_map<Address, std::unique_ptr<DynamicContract>, SafeHash> contracts_;

    /**
     * Pointer to the contract factory object.
     * Responsible for actually creating the contracts and
     * deploying them in the contract manager.
     */
    std::unique_ptr<ContractFactory> factory_;

    /// Pointer to the contract manager's interface to be passed to DynamicContract.
    std::unique_ptr<ContractManagerInterface> interface_;

    /**
     * Pointer to the call state object.
     * Responsible for maintaining temporary data used in contract call chains.
     */
    std::unique_ptr<ContractCallLogger> callLogger_;

    /**
     * Pointer to the event manager object.
     * Responsible for maintaining events emitted in contract calls.
     */
    const std::unique_ptr<EventManager> eventManager_;

    /// Reference pointer to the rdPoS contract.
    const std::unique_ptr<rdPoS>& rdpos_;

    /// Reference pointer to the options singleton.
    const std::unique_ptr<Options>& options_;

    /// Mutex that manages read/write access to the contracts.
    mutable std::shared_mutex contractsMutex_;

    /// Derive a new contract address based on transaction sender and nonce.
    Address deriveContractAddress() const;

    /**
     * Get a serialized string with the deployed contracts. Solidity counterpart:
     * function getDeployedContracts() public view returns (string[] memory, address[] memory) {}
     */
    Bytes getDeployedContracts() const;

    /**
     * Helper function to load all contracts from the database.
     * @tparam Tuple The tuple of contracts to load.
     * @tparam Is The indices of the tuple.
     * @param contract The contract to load.
     * @param contractAddress The address of the contract.
     * @return True if the contract exists in the database, false otherwise.
     */
    template <typename Tuple, std::size_t... Is>
    bool loadFromDBHelper(const auto& contract, const Address& contractAddress, std::index_sequence<Is...>) {
      return (loadFromDBT<std::tuple_element_t<Is, Tuple>>(contract, contractAddress) || ...);
    }

    /**
     * Load all contracts from the database.
     * @tparam Tuple The tuple of contracts to load.
     * @param contract The contract to load.
     * @param contractAddress The address of the contract.
     * @return True if the contract exists in the database, false otherwise.
     */
    template <typename T>
    bool loadFromDBT(const auto& contract, const Address& contractAddress) {
      // Here we disable this template when T is a tuple
      static_assert(!Utils::is_tuple<T>::value, "Must not be a tuple");
      if (Utils::bytesToString(contract.value) == Utils::getRealTypeName<T>()) {
        this->contracts_.insert(std::make_pair(
          contractAddress, std::make_unique<T>(*this->interface_, contractAddress, this->db_)
        ));
        return true;
      }
      return false;
    }

    /**
     * Load all contracts from the database using the helper function.
     * @tparam Tuple The tuple of contracts to load.
     * @param contract The contract to load.
     * @param contractAddress The address of the contract.
     * @return True if the contract exists in the database, false otherwise.
     */
    template <typename Tuple>
    std::enable_if_t<Utils::is_tuple<Tuple>::value, bool> loadFromDB(
      const auto& contract, const Address& contractAddress
    ) {
      return loadFromDBHelper<Tuple>(
        contract, contractAddress, std::make_index_sequence<std::tuple_size<Tuple>::value>{}
      );
    }

  public:
    /**
     * Constructor. Automatically loads contracts from the database and deploys them.
     * @param state Raw pointer to the state.
     * @param db Pointer to the database.
     * @param rdpos Pointer to the rdPoS contract.
     * @param options Pointer to the options singleton.
     */
    ContractManager(
      State* state, const std::unique_ptr<DB>& db,
      const std::unique_ptr<rdPoS>& rdpos, const std::unique_ptr<Options>& options
    );

    /// Destructor. Automatically saves contracts to the database before wiping them.
    ~ContractManager() override;

    /**
     * Override the default contract function call.
     * ContractManager processes things in a non-standard way (you cannot use
     * SafeVariables as contract creation actively writes to DB).
     * @param callInfo The call info to process.
     * @throw runtime_error if the call is not valid.
     */
    void ethCall(const ethCallInfo& callInfo) override;

    /**
     * Override the default contract view function call.
     * ContractManager process things in a non-standard way (you cannot use
     * SafeVariables as contract creation actively writes to DB).
     * @param data The call info to process.
     * @return A string with the requested info.
     * @throw std::runtime_error if the call is not valid.
     */
    const Bytes ethCallView(const ethCallInfo& data) const override;

    /**
     * Process a transaction that calls a function from a given contract.
     * @param tx The transaction to process.
     * @param blockHash The hash of the block that called the contract. Defaults to an empty hash.
     * @param txIndex The index of the transaction inside the block that called the contract. Defaults to the first position.
     * @throw std::runtime_error if the call to the ethCall function fails.
     * TODO: it would be a good idea to revise tests that call this function, default values here only exist as a placeholder
     */
    void callContract(const TxBlock& tx, const Hash& blockHash = Hash(), const uint64_t& txIndex = 0);

    /**
     * Make an eth_call to a view function from the contract. Used by RPC.
     * @param callInfo The call info to process.
     * @return A string with the requested info.
     * @throw std::runtime_error if the call to the ethCall function fails
     * or if the contract does not exist.
     */
    const Bytes callContract(const ethCallInfo& callInfo) const;

    /**
     * Check if an ethCallInfo is trying to access a payable function.
     * @param callInfo The call info to check.
     * @return `true` if the function is payable, `false` otherwise.
     */
    bool isPayable(const ethCallInfo& callInfo) const;

    /**
     * Validate a transaction that calls a function from a given contract.
     * @param callInfo The call info to validate.
     * @return `true` if the transaction is valid, `false` otherwise.
     * @throw std::runtime_error if the validation fails.
     */
    bool validateCallContractWithTx(const ethCallInfo& callInfo);

    /**
     * Check if a transaction calls a contract
     * @param tx The transaction to check.
     * @return `true` if the transaction calls a contract, `false` otherwise.
     */
    bool isContractCall(const TxBlock& tx) const;

    /**
     * Check if an address is a contract.
     * @param address The address to check.
     * @return `true` if the address is a contract, `false` otherwise.
     */
    bool isContractAddress(const Address& address) const;

    /// Get a list of contract names and addresses.
    std::vector<std::pair<std::string, Address>> getContracts() const;

    /**
     * Get all the events emitted under the given inputs.
     * Parameters are defined when calling "eth_getLogs" on an HTTP request
     * (directly from the http/jsonrpc submodules, through handle_request() on httpparser).
     * They're supposed to be all "optional" at that point, but here they're
     * all required, even if all of them turn out to be empty.
     * @param fromBlock The initial block height to look for.
     * @param toBlock The final block height to look for.
     * @param address The address to look for. Defaults to empty (look for all available addresses).
     * @param topics The topics to filter by. Defaults to empty (look for all available topics).
     * @return A list of matching events.
     */
    const std::vector<Event> getEvents(
      const uint64_t& fromBlock, const uint64_t& toBlock,
      const Address& address = Address(), const std::vector<Hash>& topics = {}
    ) const;

    /**
     * Overload of getEvents() for transaction receipts.
     * @param txHash The hash of the transaction to look for events.
     * @param blockIndex The height of the block to look for events.
     * @param txIndex The index of the transaction to look for events.
     * @return A list of matching events.
     */
    const std::vector<Event> getEvents(
      const Hash& txHash, const uint64_t& blockIndex, const uint64_t& txIndex
    ) const;

    /**
     * Update the ContractGlobals variables
     * Used by the State (when processing a block) to update the variables.
     * Also used by the tests.
     * CM does NOT update the variables by itself.
     * @param coinbase The coinbase address.
     * @param blockHash The hash of the block.
     * @param blockHeight The height of the block.
     * @param blockTimestamp The timestamp of the block.
     */
    void updateContractGlobals(const Address& coinbase, const Hash& blockHash, const uint64_t& blockHeight, const uint64_t& blockTimestamp);

    /// ContractManagerInterface is a friend so it can access private members.
    friend class ContractManagerInterface;

    /// ContractFactory is a friend so it can access private members.
    friend class ContractFactory;

    /// ContractCallLogger is a friend so it can access private members.
    friend class ContractCallLogger;
};

/// Interface class for DynamicContract to access ContractManager and interact with other dynamic contracts.
class ContractManagerInterface {
  private:
    ContractManager& manager_; ///< Reference to the contract manager.

  public:
    /**
     * Constructor.
     * @param manager Reference to the contract manager.
     */
    explicit ContractManagerInterface(ContractManager& manager): manager_(manager) {}

    /**
     * Register a variable that was used a given contract.
     * @param variable Reference to the variable.
     */
    void registerVariableUse(SafeBase& variable);

    /// Populate a given address with its balance from the State.
    void populateBalance(const Address& address) const;

    /**
     * Call a contract function. Used by DynamicContract to call other contracts.
     * A given DynamicContract will only call another contract if triggered by a transaction.
     * This will only be called if callContract() or validateCallContractWithTx() was called before.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @tparam Args The arguments types.
     * @param txOrigin The address of the originator of the transaction.
     * @param fromAddr The address of the caller.
     * @param targetAddr The address of the contract to call.
     * @param value Flag to indicate if the function is payable.,
     * @param func The function to call.
     * @param args The arguments to pass to the function.
     * @return The return value of the function.
     */
    template <typename R, typename C, typename... Args> R callContractFunction(
      const Address& txOrigin, const Address& fromAddr, const Address& targetAddr,
      const uint256_t& value,
      R(C::*func)(const Args&...), const Args&... args
    ) {
      if (!this->manager_.callLogger_) throw std::runtime_error(
        "Contracts going haywire! Trying to call ContractState without an active callContract"
      );
      if (value) {
        this->sendTokens(fromAddr, targetAddr, value);
      }
      if (!this->manager_.contracts_.contains(targetAddr)) {
        throw std::runtime_error(std::string(__func__) + ": Contract does not exist - Type: "
          + Utils::getRealTypeName<C>() + " at address: " + targetAddr.hex().get()
        );
      }
      C* contract = this->getContract<C>(targetAddr);
      this->manager_.callLogger_->setContractVars(contract, txOrigin, fromAddr, value);
      try {
        return contract->callContractFunction(func, std::forward<const Args&>(args)...);
      } catch (const std::exception& e) {
        throw std::runtime_error(e.what() + std::string(" - Type: ")
          + Utils::getRealTypeName<C>() + " at address: " + targetAddr.hex().get()
        );
      }
    }

    /**
     * Call a contract function with no arguments. Used by DynamicContract to call other contracts.
     * A given DynamicContract will only call another contract if triggered by a transaction.
     * This will only be called if callContract() or validateCallContractWithTx() was called before.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @param txOrigin The address of the originator of the transaction.
     * @param fromAddr The address of the caller.
     * @param targetAddr The address of the contract to call.
     * @param value Flag to indicate if the function is payable.
     * @param func The function to call.
     * @return The return value of the function.
     */
    template <typename R, typename C> R callContractFunction(
      const Address& txOrigin, const Address& fromAddr, const Address& targetAddr,
      const uint256_t& value, R(C::*func)()
    ) {
      if (!this->manager_.callLogger_) throw std::runtime_error(
        "Contracts going haywire! Trying to call ContractState without an active callContract"
      );
      if (value) this->sendTokens(fromAddr, targetAddr, value);
      if (!this->manager_.contracts_.contains(targetAddr)) {
        throw std::runtime_error(std::string(__func__) + ": Contract does not exist");
      }
      C* contract = this->getContract<C>(targetAddr);
      this->manager_.callLogger_->setContractVars(contract, txOrigin, fromAddr, value);
      try {
        return contract->callContractFunction(func);
      } catch (const std::exception& e) {
        throw std::runtime_error(e.what());
      }
    }

    /**
     * Call the createNewContract function of a contract.
     * Used by DynamicContract to create new contracts.
     * @tparam TContract The contract type.
     * @param txOrigin The address of the originator of the transaction.
     * @param fromAddr The address of the caller.
     * @param gasValue Caller gas limit.
     * @param gasPriceValue Caller gas price.
     * @param callValue The caller value.
     * @param encoder The ABI encoder.
     * @return The address of the new contract.
     */
    template <typename TContract> Address callCreateContract(
      const Address& txOrigin, const Address &fromAddr, const uint256_t &gasValue,
      const uint256_t &gasPriceValue, const uint256_t &callValue,
      const Bytes &encoder
    ) {
      if (!this->manager_.callLogger_) throw std::runtime_error(
        "Contracts going haywire! Trying to call ContractState without an active callContract"
      );
      ethCallInfo callInfo;
      std::string createSignature = "createNew" + Utils::getRealTypeName<TContract>() + "Contract(";
      // Append args
      createSignature += ContractReflectionInterface::getConstructorArgumentTypesString<TContract>();
      createSignature += ")";
      auto& [from, to, gas, gasPrice, value, functor, data] = callInfo;
      from = fromAddr;
      to = this->manager_.getContractAddress();
      gas = gasValue;
      gasPrice = gasPriceValue;
      value = callValue;
      functor = Utils::sha3(Utils::create_view_span(createSignature)).view_const(0, 4);
      data = encoder;
      this->manager_.callLogger_->setContractVars(&manager_, txOrigin, fromAddr, value);
      Address newContractAddress = this->manager_.deriveContractAddress();
      this->manager_.ethCall(callInfo);
      return newContractAddress;
    }

    /**
     * Get a contract by its address.
     * Used by DynamicContract to access view/const functions of other contracts.
     * @tparam T The contract type.
     * @param address The address of the contract.
     * @return A pointer to the contract.
     * @throw runtime_error if contract is not found or not of the requested type.
     */
    template <typename T> const T* getContract(const Address &address) const {
      auto it = this->manager_.contracts_.find(address);
      if (it == this->manager_.contracts_.end()) throw std::runtime_error(
        "ContractManager::getContract: contract at address " +
        address.hex().get() + " not found."
      );
      T* ptr = dynamic_cast<T*>(it->second.get());
      if (ptr == nullptr) throw std::runtime_error(
        "ContractManager::getContract: Contract at address " +
        address.hex().get() + " is not of the requested type: " + Utils::getRealTypeName<T>()
      );
      return ptr;
    }

    /**
     * Get a contract by its address (non-const).
     * Used by DynamicContract to access view/const functions of other contracts.
     * @tparam T The contract type.
     * @param address The address of the contract.
     * @return A pointer to the contract.
     * @throw runtime_error if contract is not found or not of the requested type.
     */
    template <typename T> T* getContract(const Address& address) {
      auto it = this->manager_.contracts_.find(address);
      if (it == this->manager_.contracts_.end()) throw std::runtime_error(
        "ContractManager::getContract: contract at address " +
        address.hex().get() + " not found."
      );
      T* ptr = dynamic_cast<T*>(it->second.get());
      if (ptr == nullptr) throw std::runtime_error(
        "ContractManager::getContract: Contract at address " +
        address.hex().get() + " is not of the requested type: " + Utils::getRealTypeName<T>()
      );
      return ptr;
    }

    /**
     * Emit an event from a contract. Called by DynamicContract's emitEvent().
     * @param event The event to emit.
     * @throw std::runtime_error if there's an attempt to emit the event outside a contract call.
     */
    void emitContractEvent(Event& event) {
      // Sanity check - events should only be emitted during successful contract
      // calls AND on non-pure/non-view functions. Since callLogger on view
      // function calls is set to nullptr, this ensures that events only happen
      // inside contracts and are not emitted if a transaction reverts.
      // C++ itself already takes care of events not being emitted on pure/view
      // functions due to its built-in const-correctness logic.
      // TODO: check later if events are really not emitted on transaction revert
      if (!this->manager_.callLogger_) throw std::runtime_error(
        "Contracts going haywire! Trying to emit an event without an active contract call"
      );
      this->manager_.eventManager_->registerEvent(std::move(event));
    }

    /**
     * Get the balance from a given address. Calls populateBalance(), so
     * it's technically the same as getting the balance directly from State.
     * Does NOT consider the current transaction being processed, if there is one.
     * @param address The address to get the balance from.
     * @return The balance of the address.
     */
    uint256_t getBalanceFromAddress(const Address& address) const;

    /**
     * Send tokens to a given address. Used by DynamicContract to send tokens to other contracts.
     * @param from The address from which the tokens will be sent from.
     * @param to The address to send the tokens to.
     * @param amount The amount of tokens to send.
     */
    void sendTokens(const Address& from, const Address& to, const uint256_t& amount);
};

#endif // CONTRACTMANAGER_H
