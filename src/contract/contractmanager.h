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

// Forward declarations.
class rdPoS;
class State;
class ContractManagerInterface;

/**
 * Addresses for the contracts that are deployed at protocol level (contract name -> contract address).
 * That means these contracts are deployed at the beginning of the chain.
 * They cannot be destroyed nor dynamically deployed like other contracts.
 * Instead, they are deployed in the constructor of State.
 */
const std::unordered_map<std::string, Address> ProtocolContractAddresses = {
  {"rdPoS", Address(Hex::toBytes("0xb23aa52dbeda59277ab8a962c69f5971f22904cf"))}, // Sha3("randomDeterministicProofOfStake").substr(0,20)
  {"ContractManager", Address(Hex::toBytes("0x0001cb47ea6d8b55fe44fdd6b1bdb579efb43e61"))} // Sha3("ContractManager").substr(0,20)
};

/**
 * Class that holds all current contract instances in the blockchain state.
 * Responsible for being the factory and deploying contracts in the chain.
 * Also acts as an access point for contracts to access each other.
 */
class ContractManager : BaseContract {
  private:
    /**
     * Raw pointer to the blockchain state.
     * Used if the contract is a payable function.
     * Can be `nullptr` due to tests not requiring state (contract balance).
     */
    State* state;

    /**
     * Temporary map of balances within the chain.
     * Used during callContract with a payable function.
     * Cleared after the callContract function commited to the state if
     * `getCommit() == true` and the ethCall was successful.
     */
    std::unordered_map<Address, uint256_t, SafeHash> balances;

    /// Address of the contract that is currently being called.
    Address currentActiveContract = Address();

    /// List of currently deployed contracts.
    std::unordered_map<Address, std::unique_ptr<DynamicContract>, SafeHash> contracts;

    /// Map of contract functors and create functions, used to create contracts.
    std::unordered_map<Bytes, std::function<void(const ethCallInfo &)>, SafeHash> createContractFuncs;

    /// Map of contract functors and validate functions, used to validate contracts.
    std::unordered_map<Bytes, std::function<void(const ethCallInfo &)>, SafeHash> validateContractFuncs;

    /// Mutex that manages read/write access to the contracts.
    mutable std::shared_mutex contractsMutex;

    /// Reference pointer to the rdPoS contract.
    const std::unique_ptr<rdPoS>& rdpos;

    /// Reference pointer to the options singleton.
    const std::unique_ptr<Options>& options;

    /// Derive a new contract address based on transaction sender and nonce.
    Address deriveContractAddress(const ethCallInfo &callInfo) const;

    /**
     * Setup data for a new contract before creating/validating it.
     * @param callInfo The call info to process.
     * @return A pair containing the contract address and the ABI decoder.
     * @throw runtime_error if non contract creator tries to create a contract.
     * @throw runtime_error if contract already exists.
     */
    template <typename TContract>
    std::pair<Address, ABI::Decoder> setupNewContract(const ethCallInfo &callInfo) {
      // Check if caller is creator
      if (this->caller != this->getContractCreator()) {
        throw std::runtime_error("Only contract creator can create new contracts");
      }

      // Check if contract address already exists
      const Address derivedContractAddress = this->deriveContractAddress(callInfo);
      if (this->contracts.contains(derivedContractAddress)) {
        throw std::runtime_error("Contract already exists");
      }

      std::unique_lock lock(this->contractsMutex);
      for (const auto &[protocolContractName, protocolContractAddress] : ProtocolContractAddresses) {
        if (protocolContractAddress == derivedContractAddress) {
          throw std::runtime_error("Contract already exists");
        }
      }
      std::vector<ABI::Types> types = ContractReflectionInterface::getConstructorArgumentTypes<TContract>();
      ABI::Decoder decoder(types, std::get<6>(callInfo));
      return std::make_pair(derivedContractAddress, decoder);
    }

    /**
     * Helper function to create a new contract from a given call info.
     * @tparam TContract The contract type to create.
     * @tparam TTuple The tuple type of the contract constructor arguments.
     * @tparam Is The indices of the tuple.
     * @param derivedContractAddress The address of the contract to create.
     * @param dataVec The vector of arguments to pass to the contract constructor.
     * @return A unique pointer to the newly created contract.
     * @throw runtime_error if any argument type mismatches.
     */
    template <typename TContract, typename TTuple, std::size_t... Is>
    std::unique_ptr<TContract> createContractWithTuple(const Address& derivedContractAddress,
        const std::vector<std::any>& dataVec,
        std::index_sequence<Is...>) {
      try {
        return std::make_unique<TContract>(std::any_cast<typename std::tuple_element<Is, TTuple>::type>(dataVec[Is])...,
          *this->interface, derivedContractAddress, this->getCaller(), this->options->getChainID(), this->db
        );
      } catch (const std::bad_any_cast& ex) {
        throw std::runtime_error("Mismatched argument types for contract constructor. Expected: " +
          Utils::getRealTypeName<TTuple>()
        );
      }
    }

    /**
     * Helper function to create a new contract from a given call info.
     * @param derivedContractAddress The address of the contract to create.
     * @param dataVec The vector of arguments to pass to the contract constructor.
     * @throw runtime_error if the size of the vector does not match the number of
     * arguments of the contract constructor.
     */
    template <typename TContract, typename TTuple>
    std::unique_ptr<TContract> createContractWithTuple(const Address& derivedContractAddress, const std::vector<std::any>& dataVec) {
      constexpr std::size_t TupleSize = std::tuple_size<TTuple>::value;
      if (TupleSize != dataVec.size()) {
        throw std::runtime_error("Not enough arguments provided for contract constructor. Expected: " +
          std::to_string(TupleSize) + ", got: " + std::to_string(dataVec.size())
        );
      }
      return createContractWithTuple<TContract, TTuple>(
        derivedContractAddress, dataVec, std::make_index_sequence<TupleSize>{}
      );
    }

    /**
     * Create a new contract from a given call info.
     * @param callInfo The call info to process.
     * @throw runtime_error if the call to the ethCall function fails or if the
     * contract is does not exist.
     */
    template <typename TContract>
    void createNewContract(const ethCallInfo& callInfo) {
      using ConstructorArguments = typename TContract::ConstructorArguments;
      auto setupResult = this->setupNewContract<TContract>(callInfo);
      if (!ContractReflectionInterface::isContractRegistered<TContract>()) {
        throw std::runtime_error("Contract " + Utils::getRealTypeName<TContract>() + " is not registered");
      }

      Address derivedContractAddress = setupResult.first;
      ABI::Decoder decoder = setupResult.second;
      std::vector<ABI::Types> types = ContractReflectionInterface::getConstructorArgumentTypes<TContract>();
      std::vector<std::any> dataVector;

      std::unordered_map<ABI::Types, std::function<std::any(uint256_t)>> castFunctions = {
        {ABI::Types::uint8, [](uint256_t value) { return std::any(static_cast<uint8_t>(value)); }},
        {ABI::Types::uint16, [](uint256_t value) { return std::any(static_cast<uint16_t>(value)); }},
        {ABI::Types::uint32, [](uint256_t value) { return std::any(static_cast<uint32_t>(value)); }},
        {ABI::Types::uint64, [](uint256_t value) { return std::any(static_cast<uint64_t>(value)); }}
      };

      for (size_t i = 0; i < types.size(); i++) {
        if (castFunctions.count(types[i]) > 0) {
          uint256_t value = std::any_cast<uint256_t>(decoder.getDataDispatch(i, types[i]));
          dataVector.push_back(castFunctions[types[i]](value));
        } else {
          dataVector.push_back(decoder.getDataDispatch(i, types[i]));
        }
      }

      auto contract = createContractWithTuple<TContract, ConstructorArguments>(derivedContractAddress, dataVector);
      this->contracts.insert(std::make_pair(derivedContractAddress, std::move(contract)));
    }

    /**
     * Validate a new contract from a given call info.
     * @param callInfo The call info to process.
     * @throw runtime_error if the call to the ethCall function fails or if the
     * contract is does not exist.
     */
    template <typename TContract>
    void validateNewContract(const ethCallInfo &callInfo) {
      this->setupNewContract<TContract>(callInfo);
    }

    /**
     * Adds contract create and validate functions to the respective maps
     * @tparam Contract Contract type
     * @param createFunc Function to create a new contract
     * @param validateFunc Function to validate a new contract
     */
    template <typename Contract>
    void addContractFuncs(std::function<void(const ethCallInfo &)> createFunc, std::function<void(const ethCallInfo &)> validateFunc) {
      std::string createSignature = "createNew" + Utils::getRealTypeName<Contract>() + "Contract";
      std::vector<std::string> args = ContractReflectionInterface::getConstructorArgumentTypesString<Contract>();
      std::ostringstream createFullSignatureStream;
      createFullSignatureStream << createSignature << "(";
      if (!args.empty()) {
        std::copy(args.begin(), args.end() - 1, std::ostream_iterator<std::string>(createFullSignatureStream, ","));
        createFullSignatureStream << args.back();
      }
      createFullSignatureStream << ")";
      Functor functor = Utils::sha3(Utils::create_view_span(createFullSignatureStream.str())).view_const(0, 4);
      createContractFuncs[functor.asBytes()] = createFunc;
      validateContractFuncs[functor.asBytes()] = validateFunc;
    }

    /**
     * Struct for calling the registerContract function of a contract.
     * @tparam TContract The contract to register.
     */
    template <class T> struct RegisterContract {
      RegisterContract() { T::registerContract(); }
    };

    /**
     * Helper function to register all contracts.
     * @tparam Tuple The tuple of contracts to register.
     * @tparam Is The indices of the tuple.
     */
    template <typename Tuple, std::size_t... Is>
    void registerContractsHelper(std::index_sequence<Is...>) {
      (RegisterContract<std::tuple_element_t<Is, Tuple>>(), ...);
    }

    /**
     * Register all contracts in the tuple.
     * @tparam Tuple The tuple of contracts to register.
     */
    template <typename Tuple>
    std::enable_if_t<Utils::is_tuple<Tuple>::value, void> registerContracts() {
      registerContractsHelper<Tuple>(std::make_index_sequence<std::tuple_size<Tuple>::value>{});
    }

    /**
     * Register all contracts in the variadic template.
     * @tparam Contracts The contracts to register.
     */
    template <typename Tuple, std::size_t... Is>
    void addAllContractFuncsHelper(std::index_sequence<Is...>) {
      ((this->addContractFuncs<std::tuple_element_t<Is, Tuple>>(
        [&](const ethCallInfo &callInfo) { this->createNewContract<std::tuple_element_t<Is, Tuple>>(callInfo); },
        [&](const ethCallInfo &callInfo) { this->validateNewContract<std::tuple_element_t<Is, Tuple>>(callInfo); }
      )), ...);
    }

    /**
     * Add all contract functions to the respective maps.
     * @tparam Contracts The contracts to add.
     */
    template <typename... Contracts>
    std::enable_if_t<!Utils::is_tuple<std::tuple<Contracts...>>::value, void> addAllContractFuncs() {
      (void)std::initializer_list<int>{((void)addAllContractFuncs<Contracts>(), 0)...};
    }

    /**
     * Add all contract functions to the respective maps using the helper function.
     * @tparam Tuple The tuple of contracts to add.
     */
    template <typename Tuple>
    std::enable_if_t<Utils::is_tuple<Tuple>::value, void> addAllContractFuncs() {
      addAllContractFuncsHelper<Tuple>(std::make_index_sequence<std::tuple_size<Tuple>::value>{});
    }

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
    std::enable_if_t<Utils::is_tuple<Tuple>::value, bool> loadFromDB(const auto& contract, const Address& contractAddress) {
      return loadFromDBHelper<Tuple>(contract, contractAddress, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
    }

    /**
     * Get a serialized string with the deployed contracts. Solidity counterpart:
     * function getDeployedContracts() public view returns (string[] memory, address[] memory) {}
     */
    Bytes getDeployedContracts() const;

    std::unique_ptr<ContractManagerInterface> interface; ///< Interface to be passed to DynamicContract.

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
     * Make an eth_call to a view function from the contract. Used by RPC.
     * @param callInfo The call info to process.
     * @return A string with the requested info.
     * @throw std::runtime_error if the call to the ethCall function fails
     * or if the contract does not exist.
     */
    const Bytes callContract(const ethCallInfo& callInfo) const;

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

    ///< ContractManagerInterface is a friend so it can access private members.
    friend class ContractManagerInterface;
};

/// Interface class for DynamicContract to access ContractManager and interact with other dynamic contracts.
class ContractManagerInterface {
  private:
    /// Reference to the contract manager.
    ContractManager& contractManager;

  public:
    /**
     * Constructor.
     * @param contractManager Reference to the contract manager.
     */
    explicit ContractManagerInterface(ContractManager& contractManager)
      : contractManager(contractManager)
    {}

    /// Populate a given address with its balance from the State.
    void populateBalance(const Address& address) const;

    /**
     * Call a contract function. Used by DynamicContract to call other contracts.
     * A given DynamicContract will only call another contract if
     * it was first triggered by a transaction.
     * That means we can use contractManager::commit() to know if
     * the call should commit or not.
     * This function will only be called if ContractManager::callContract()
     * or ContractManager::validateCallContractWithTx() was called before.
     * @param callInfo The call info.
     */
    void callContract(const ethCallInfo& callInfo);

    template <typename R, typename C, typename... Args>
    R callContractFunction(const Address& fromAddr, 
                           const Address& targetAddr, 
                           const uint256_t& value, 
                           R(C::*func)(const Args&...), 
                           const Args&... args) {
        if (value) {
          this->sendTokens(fromAddr, targetAddr, value);
        }
        else {
          if (!this->contractManager.contracts.contains(targetAddr)) {
            throw std::runtime_error("Contract does not exist");
          }
        }
        C* contract = this->getContract<C>(targetAddr);
        contract->caller = fromAddr;
        contract->value = value;
        contract->commit = this->contractManager.getCommit();
        try {
          return (contract->*func)(std::forward<const Args&>(args)...);
        } catch (const std::exception& e) {
          contract->commit = false;
          throw std::runtime_error(e.what());
        }
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
      auto it = this->contractManager.contracts.find(address);
      if (it == this->contractManager.contracts.end()) throw std::runtime_error(
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

    template <typename T> T* getContract(const Address& address) {
      auto it = this->contractManager.contracts.find(address);
      if (it == this->contractManager.contracts.end()) throw std::runtime_error(
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
