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

/// Forward Declaration
class rdPoS;
class State;

/**
 * Addresses for the contracts that are deployed at protocol level
 * That means, these contracts are deployed at the beginning of the chain.
 * They cannot be dynamically deployed like other contracts, and they cannot be
 * destroyed. Instead, they are deployed in the constructor of State. Such
 * example of these contracts is rdPoS. contract name -> contract address
 */

const std::unordered_map<std::string, Address> ProtocolContractAddresses = {
    {"rdPoS",
     Address(Hex::toBytes("0xb23aa52dbeda59277ab8a962c69f5971f22904cf"),
             true)}, // Sha3("randomDeterministicProofOfStake").substr(0,20)
    {"ContractManager",
     Address(Hex::toBytes("0x0001cb47ea6d8b55fe44fdd6b1bdb579efb43e61"),
             true)} // Sha3("ContractManager").substr(0,20).
};

/**
 * Class that holds all current contract instances in the blockchain state.
 * Responsible for being the factory and deploying contracts in the chain.
 * Also acts as an access point for contracts to access each other.
 */

class ContractManager : BaseContract {
private:
  /// Pointer back to the State
  /// Used if the Contract is a payable function.
  /// Can be a nullptr due to tests not requiring state (contract balance).
  State *state;

  /// Temporary map of balances within the chain.
  /// Used during callContract with a payable function.
  /// Cleared after the callContract function
  /// Commited to the state if getCommit() == true and the ethCall was
  /// successful.
  std::unordered_map<Address, uint256_t, SafeHash> balances;

  /// Address of the contract that is currently being called.
  Address currentActiveContract = Address();

  /// List of currently deployed contracts.
  std::unordered_map<Address, std::unique_ptr<DynamicContract>, SafeHash>
      contracts;

  /// Contracts mutex
  mutable std::shared_mutex contractsMutex;

  /// Reference back to the rdPoS contract.
  const std::unique_ptr<rdPoS> &rdpos;

  /// Reference back to options
  const std::unique_ptr<Options> &options;
  
  /// Derive a new contract address based on transaction sender + nonce.
  Address deriveContractAddress(const ethCallInfo &callInfo) const;

  /// Create a new ERC20 contract.
  /// function createNewERC20Contract(string memory name, string memory symbol,
  /// uint8 decimals, uint256 supply) public {}
  void createNewERC20Contract(const ethCallInfo &callInfo);

  /// Check if transaction can actually create a new ERC20 contract.
  /// function createNewERC20Contract(string memory name, string memory symbol,
  /// uint8 decimals, uint256 supply) public {}
  void validateCreateNewERC20Contract(const ethCallInfo &callInfo) const;

  /// Create a new ERC20Wrapper Contract.
  /// function createNewERC20WrapperContract() public {}
  void createNewERC20WrapperContract(const ethCallInfo &callInfo);

  /// Check if transaction can actually create a new ERC20 contract.
  void validateCreateNewERC20WrapperContract(const ethCallInfo &callInfo) const;

  /// Create a new ERC20 Native Wrapper Contract.
  /// function createNewERC20NativeWrapperContract(string memory name, string
  /// memory symbol, uint8 decimals) public {}
  void createNewERC20NativeWrapperContract(const ethCallInfo &callInfo);

  /// Check if transaction can actually create a new ERC20 Native Wrapper
  /// contract.
  void validateCreateNewERC20NativeWrapperContract(
      const ethCallInfo &callInfo) const;

  /// Serialization function for
  /// function getDeployedContracts() public view returns (string[] memory,
  /// address[] memory) {}
  std::string getDeployedContracts() const;

public:
  /// Interface class for DynamicContract to access ContractManager and interact
  /// with other dynamic contracts.
  class ContractManagerInterface {
  private:
    /// Reference back to the contract manager
    ContractManager &contractManager;

  public:
    /// Constructor
    explicit ContractManagerInterface(ContractManager &contractManager)
        : contractManager(contractManager) {}

    /// Populate a given address with the balance of the State
    void populateBalance(const Address &address) const;

    /// Call a contract function.
    /// Used by DynamicContract to call other contracts
    /// @param callInfo The call info
    /// A given DynamicContract will only call another contract if it was
    /// firstly triggered by a transaction. That means, we can use
    /// contractManager::commit to know if the call should commit or not. This
    /// function will only be called if a ContractManager::callContract or
    /// ContractManager::validateCallContractWithTx was called before.
    void callContract(const ethCallInfo &callInfo);

    /**
     * @brief Get a contract by address
     * Used by DynamicContract to access view/const functions of other contracts
     * @tparam T The contract type
     * @param address The address of the contract
     * @return A pointer to the contract
     * @throw runtime_error if contract not found or if contract is not of the
     * requested type
     */
    template <typename T> const T *getContract(const Address &address) const {
      std::shared_lock<std::shared_mutex> lock(
          this->contractManager.contractsMutex);
      auto it = this->contractManager.contracts.find(address);
      if (it == this->contractManager.contracts.end()) {
        throw std::runtime_error(
            "ContractManager::getContract: contract at address " +
            address.hex().get() + " not found.");
      }
      T *ptr = dynamic_cast<T *>(it->second.get());
      if (ptr == nullptr) {
        throw std::runtime_error(
            "ContractManager::getContract: Contract at address " +
            address.hex().get() +
            " is not of the requested type: " + typeid(T).name());
      }
      return ptr;
    }

    /**
     * @brief Returns the balance from the State
     * Doesn't take account of current transaction being processed.
     * @param address The address to get the balance from.
     * @return The balance of the address.
     */
    uint256_t getBalanceFromAddress(const Address &address) const;

    /// Send tokens to a given address
    /// Used by DynamicContract to send Tokens to other Contracts
    /// @param address The address to send the tokens to.
    /// @param amount The amount of tokens to send.

    /**
     * @brief Uses currentActiveContract to know which contract is sending the
     * tokens.
     * @param address The address to send the tokens to.
     * @param amount The amount of tokens to send.
     * @throw runtime_error if there is not enough balance to send the tokens.
     */
    void sendTokens(const Address &from, const Address &to,
                    const uint256_t &amount);
  };

private:
  ContractManagerInterface interface; ///< Interface to be passed to
                                      ///< DynamicContract

public:
  // TODO: constructor and destructor are not implemented because we don't know
  // how contract loading/saving will work yet

  /**
   * Constructor. Automatically loads contracts from the database and deploys
   * them.
   * @param db Pointer to the database.    Contract(const std::string&
   * contractName, const Address& address, const Address& creator, const
   * uint64_t& chainId, const std::unique_ptr<DB> &db
   */
  ContractManager(State *state, const std::unique_ptr<DB> &db,
                  const std::unique_ptr<rdPoS> &rdpos,
                  const std::unique_ptr<Options> &options);

  /// Destructor. Automatically saves contracts to the database before wiping
  /// them.
  ~ContractManager() override;

  /**
   * @brief Override the default contract function call.
   * ContractManager process things in a non-standard (cannot use SafeVariables
   * as contract creation actively writes to DB)
   * @param callInfo The call info to process.
   * @throws runtime_error if the call is not valid.
   */
  void ethCall(const ethCallInfo &callInfo) override;

  /**
   * @brief Override the default contract function call.
   * ContractManager process things in a non-standard (cannot use SafeVariables
   * as contract creation actively writes to DB)
   * @param callInfo The call info to process.
   * @throws runtime_error if the call is not valid.
   */
  const std::string ethCallView(const ethCallInfo &data) const override;

  /**
   * Process a transaction that calls a function from a given contract.
   * @param tx The transaction to process.
   * @throws runtime_error if the call to the ethCall function fails.
   */
  void callContract(const TxBlock &callInfo);

  /**
   * Check if a EthCallInfo is trying to access a payable function
   * @param callInfo The call info to check.
    * @return True if the function is payable, false otherwise.

   */
  bool isPayable(const ethCallInfo &callInfo) const;

  /**
   * Validate a transaction that calls a function from a given contract.
   * @param callInfo The call info to validate.
   * @return True if the transaction is valid, false otherwise.
   * @throws runtime_error if the validation fails.
   */
  bool validateCallContractWithTx(const ethCallInfo &callInfo);

  /**
   * Make a eth_call to a view function from the contract.
   * Used by RPC
   * @param callInfo The call info to process.
   * @return Just a empty return.
   * @throws runtime_error if the call to the ethCall function fails or if the
   * contract is does not exist.
   */
  const std::string callContract(const ethCallInfo &callInfo) const;

  /**
   * Check if a transaction calls a contract
   * @param tx The transaction to check.
    * @return True if the transaction calls a contract, false otherwise.

   */
  bool isContractCall(const TxBlock &tx) const;

  /**
   * Check if an address is a contract address.
   * @param address the address to check.
   * @return True if the address is a contract address, false otherwise.
   */
  bool isContractAddress(const Address &address) const;

  /**
   * Get list of contracts addresses and names.
   * @return A vector of pairs of contract names and addresses.
   */
  std::vector<std::pair<std::string, Address>> getContracts() const;

  friend class ContractManagerInterface; ///< ContractManagerInterface is a
                                         ///< friend class of ContractManager
                                         ///< so it can access private members
                                         ///< of ContractManager
};

#endif // CONTRACTMANAGER_H
