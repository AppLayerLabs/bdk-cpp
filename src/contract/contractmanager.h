#ifndef CONTRACTMANAGER_H
#define CONTRACTMANAGER_H

#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "abi.h"
#include "contract.h"

#include "../utils/db.h"
#include "../utils/options.h"
#include "../utils/safehash.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"
#include "../utils/contractreflectioninterface.h"
#include "variables/safeunorderedmap.h"

// Forward declarations.
class rdPoS;
class State;
class ContractFactory;
class ContractCallState;
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
    State* state;

    /**
     * Pointer to the contract factory object.
     * Responsible for actually creating the contracts and
     * deploying them in the contract manager.
     */
    std::unique_ptr<ContractFactory> factory;

    /**
     * Pointer to the call state object.
     * Responsible for maintaining temporary data used in contract call chains.
     */
    std::unique_ptr<ContractCallState> callState;

    /// Pointer to the contract manager's interface to be passed to DynamicContract.
    std::unique_ptr<ContractManagerInterface> interface;

    /// Reference pointer to the rdPoS contract.
    const std::unique_ptr<rdPoS>& rdpos;

    /// Reference pointer to the options singleton.
    const std::unique_ptr<Options>& options;

    /// List of currently deployed contracts.
    std::unordered_map<Address, std::unique_ptr<DynamicContract>, SafeHash> contracts;

    /// Mutex that manages read/write access to the contracts.
    mutable std::shared_mutex contractsMutex;

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
        this->contracts.insert(std::make_pair(
          contractAddress, std::make_unique<T>(*this->interface, contractAddress, this->db)
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
     * @throw std::runtime_error if the call to the ethCall function fails.
     */
    void callContract(const TxBlock& tx);

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

    /// ContractManagerInterface is a friend so it can access private members.
    friend class ContractManagerInterface;

    /// ContractFactory is a friend so it can access private members.
    friend class ContractFactory;

    /// ContractCallState is a friend so it can access private members.
    friend class ContractCallState;
};

/// Interface class for DynamicContract to access ContractManager and interact with other dynamic contracts.
class ContractManagerInterface {
  private:
    ContractManager& manager; ///< Reference to the contract manager.

  public:
    /**
     * Constructor.
     * @param manager Reference to the contract manager.
     */
    explicit ContractManagerInterface(ContractManager& manager): manager(manager) {}

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
     * That means we can use commit() to know if the call should commit or not.
     * This will only be called if callContract() or validateCallContractWithTx() was called before.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @tparam Args The arguments types.
     * @param fromAddr The address of the caller.
     * @param targetAddr The address of the contract to call.
     * @param value Flag to indicate if the function is payable.,
     * @param commit Flag to set contract->commit same as caller.
     * @param func The function to call.
     * @param args The arguments to pass to the function.
     * @return The return value of the function.
     */
    template <typename R, typename C, typename... Args> R callContractFunction(
      const Address& fromAddr, const Address& targetAddr,
      const uint256_t& value, const bool& commit,
      R(C::*func)(const Args&...), const Args&... args
    ) {
      if (value) {
        this->sendTokens(fromAddr, targetAddr, value);
      } else {
        if (!this->manager.contracts.contains(targetAddr)) {
          throw std::runtime_error("Contract does not exist");
        }
      }
      C* contract = this->getContract<C>(targetAddr);
      contract->caller = fromAddr;
      contract->value = value;
      contract->commit = commit;
      try {
        return contract->callContractFunction(func, std::forward<const Args&>(args)...);
      } catch (const std::exception& e) {
        contract->commit = false;
        throw std::runtime_error(e.what());
      }
    }

    /**
     * Call a contract function with no arguments. Used by DynamicContract to call other contracts.
     * A given DynamicContract will only call another contract if triggered by a transaction.
     * That means we can use commit() to know if the call should commit or not.
     * This will only be called if callContract() or validateCallContractWithTx() was called before.
     * @tparam R The return type of the function.
     * @tparam C The contract type.
     * @param fromAddr The address of the caller.
     * @param targetAddr The address of the contract to call.
     * @param value Flag to indicate if the function is payable.
     * @param commit Flag to set contract->commit same as caller.
     * @param func The function to call.
     * @return The return value of the function.
     */
    template <typename R, typename C> R callContractFunction(
      const Address& fromAddr, const Address& targetAddr,
      const uint256_t& value, const bool& commit, R(C::*func)()
    ) {
      if (value) {
        this->sendTokens(fromAddr, targetAddr, value);
      } else {
        if (!this->manager.contracts.contains(targetAddr)) {
          throw std::runtime_error("Contract does not exist");
        }
      }
      C* contract = this->getContract<C>(targetAddr);
      contract->caller = fromAddr;
      contract->value = value;
      contract->commit = commit;
      try {
        return contract->callContractFunction(func);
      } catch (const std::exception& e) {
        contract->commit = false;
        throw std::runtime_error(e.what());
      }
    }

    /**
     * Call the createNewContract function of a contract.
     * Used by DynamicContract to create new contracts.
     * @tparam TContract The contract type.
     * @param fromAddr The address of the caller.
     * @param gasValue Caller gas limit.
     * @param gasPriceValue Caller gas price.
     * @param callValue The caller value.
     * @param encoder The ABI encoder.
     * @return The address of the new contract.
     */
    template <typename TContract> Address callCreateContract(
      const Address &fromAddr, const uint256_t &gasValue,
      const uint256_t &gasPriceValue, const uint256_t &callValue,
      const ABI::Encoder &encoder
    ) {
      ethCallInfo callInfo;
      std::string createSignature = "createNew" + Utils::getRealTypeName<TContract>() + "Contract";
      std::vector<std::string> args = ContractReflectionInterface::getConstructorArgumentTypesString<TContract>();
      std::ostringstream createFullSignatureStream;
      createFullSignatureStream << createSignature << "(";
      if (!args.empty()) {
        std::copy(args.begin(), args.end() - 1, std::ostream_iterator<std::string>(createFullSignatureStream, ","));
        createFullSignatureStream << args.back();
      }
      createFullSignatureStream << ")";
      auto& [from, to, gas, gasPrice, value, functor, data] = callInfo;
      from = fromAddr;
      to = this->manager.deriveContractAddress();
      gas = gasValue;
      gasPrice = gasPriceValue;
      value = callValue;
      functor = Utils::sha3(Utils::create_view_span(createFullSignatureStream.str())).view_const(0, 4);
      data = encoder.getData();
      this->manager.ethCall(callInfo);
      return to;
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
      auto it = this->manager.contracts.find(address);
      if (it == this->manager.contracts.end()) throw std::runtime_error(
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
      auto it = this->manager.contracts.find(address);
      if (it == this->manager.contracts.end()) throw std::runtime_error(
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
