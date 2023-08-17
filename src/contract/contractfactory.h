#ifndef CONTRACTFACTORY_H
#define CONTRACTFACTORY_H

#include "../utils/safehash.h"
#include "../utils/strings.h"
#include "../utils/utils.h"
#include "../utils/contractreflectioninterface.h"

#include "abi.h"
#include "contract.h"
#include "contractmanager.h"

/// Factory class that does the setup, creation and registration of contracts to the blockchain.
class ContractFactory {
  private:
    ContractManager& manager; ///< Reference to the contract manager.

    /// Map of contract functors and create functions, used to create contracts.
    std::unordered_map<Bytes, std::function<void(const ethCallInfo&)>, SafeHash> createContractFuncs;

    /// Set of recently created contracts.
    std::unordered_set<Address, SafeHash> recentContracts;

  public:
    /**
     * Constructor.
     * @param manager Reference to the contract manager.
     */
    ContractFactory(ContractManager& manager) : manager(manager) {}

    /// Getter for `recentContracts`.
    std::unordered_set<Address, SafeHash> getRecentContracts() const;

    /// Clearer for `recentContracts`.
    void clearRecentContracts();

    /**
     * Get the createNewContract function of a given contract.
     * @param func The functor to search for.
     * @return The respective functor's creation function, or an empty function if not found.
     */
    std::function<void(const ethCallInfo&)> getCreateContractFunc(Functor func) const;

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
      if (this->manager.getOrigin() != this->manager.getContractCreator()) {
        throw std::runtime_error("Only contract creator can create new contracts");
      }

      // Check if contract address already exists on the Dynamic Contract list
      const Address derivedAddress = this->manager.deriveContractAddress();
      if (this->manager.contracts.contains(derivedAddress)) {
        throw std::runtime_error("Contract already exists");
      }

      // Check if contract address already exists on the Protocol Contract list
      for (const auto &[name, address] : ProtocolContractAddresses) {
        if (address == derivedAddress) {
          throw std::runtime_error("Contract already exists");
        }
      }

      // Setup the contract
      std::vector<ABI::Types> types = ContractReflectionInterface::getConstructorArgumentTypes<TContract>();
      ABI::Decoder decoder(types, std::get<6>(callInfo));
      return std::make_pair(derivedAddress, decoder);
    }

    /**
     * Create a new contract from a given call info.
     * @param callInfo The call info to process.
     * @throw runtime_error if the call to the ethCall function fails,
     *                      or if the contract does not exist.
     */
    template <typename TContract> void createNewContract(const ethCallInfo& callInfo) {
      using ConstructorArguments = typename TContract::ConstructorArguments;
      auto setupResult = this->setupNewContract<TContract>(callInfo);
      if (!ContractReflectionInterface::isContractRegistered<TContract>()) {
        throw std::runtime_error("Contract " + Utils::getRealTypeName<TContract>() + " is not registered");
      }

      Address derivedAddress = setupResult.first;
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

      // Update the inner variables of the contract.
      // The constructor can set SafeVariable values from the constructor.
      // We need to take account of that and set the variables accordingly.
      auto contract = createContractWithTuple<TContract, ConstructorArguments>(
        std::get<0>(callInfo), derivedAddress, dataVector
      );
      this->recentContracts.insert(derivedAddress);
      this->manager.contracts.insert(std::make_pair(derivedAddress, std::move(contract)));
    }

    /**
     * Helper function to create a new contract from a given call info.
     * @tparam TContract The contract type to create.
     * @tparam TTuple The tuple type of the contract constructor arguments.
     * @tparam Is The indices of the tuple.
     * @param creator The address of the contract creator.
     * @param derivedContractAddress The address of the contract to create.
     * @param dataVec The vector of arguments to pass to the contract constructor.
     * @return A unique pointer to the newly created contract.
     * @throw runtime_error if any argument type mismatches.
     */
    template <typename TContract, typename TTuple, std::size_t... Is>
    std::unique_ptr<TContract> createContractWithTuple(
      const Address& creator,
      const Address& derivedContractAddress,
      const std::vector<std::any>& dataVec,
      std::index_sequence<Is...>
    ) {
      try {
        return std::make_unique<TContract>(
          std::any_cast<typename std::tuple_element<Is, TTuple>::type>(dataVec[Is])...,
          *this->manager.interface, derivedContractAddress, creator,
          this->manager.options->getChainID(), this->manager.db
        );
      } catch (const std::bad_any_cast& ex) {
        throw std::runtime_error(
          "Mismatched argument types for contract constructor. Expected: " +
          Utils::getRealTypeName<TTuple>()
        );
      }
    }

    /**
     * Helper function to create a new contract from a given call info.
     * @param creator The address of the contract creator.
     * @param derivedContractAddress The address of the contract to create.
     * @param dataVec The vector of arguments to pass to the contract constructor.
     * @throw runtime_error if the size of the vector does not match the number of
     * arguments of the contract constructor.
     */
    template <typename TContract, typename TTuple>
    std::unique_ptr<TContract> createContractWithTuple(
      const Address& creator, const Address& derivedContractAddress,
      const std::vector<std::any>& dataVec
    ) {
      constexpr std::size_t TupleSize = std::tuple_size<TTuple>::value;
      if (TupleSize != dataVec.size()) throw std::runtime_error(
        "Not enough arguments provided for contract constructor. Expected: " +
        std::to_string(TupleSize) + ", got: " + std::to_string(dataVec.size())
      );
      return this->createContractWithTuple<TContract, TTuple>(
        creator, derivedContractAddress, dataVec, std::make_index_sequence<TupleSize>{}
      );
    }

    /**
     * Adds contract create and validate functions to the respective maps
     * @tparam Contract Contract type
     * @param createFunc Function to create a new contract
     */
    template <typename Contract>
    void addContractFuncs(std::function<void(const ethCallInfo &)> createFunc) {
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
      this->createContractFuncs[functor.asBytes()] = createFunc;
    }

    /**
     * Register all contracts in the variadic template.
     * @tparam Contracts The contracts to register.
     */
    template <typename Tuple, std::size_t... Is>
    void addAllContractFuncsHelper(std::index_sequence<Is...>) {
      ((this->addContractFuncs<std::tuple_element_t<Is, Tuple>>( [&](const ethCallInfo &callInfo) {
        this->createNewContract<std::tuple_element_t<Is, Tuple>>(callInfo);
      })), ...);
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
     * Add all contract functions to the respective maps.
     * @tparam Contracts The contracts to add.
     */
    template <typename... Contracts>
    std::enable_if_t<!Utils::is_tuple<std::tuple<Contracts...>>::value, void> addAllContractFuncs() {
      (void)std::initializer_list<int>{((void)addAllContractFuncs<Contracts>(), 0)...};
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
};

#endif  // CONTRACTFACTORY_H
