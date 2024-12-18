/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SDKTESTSUITE_H
#define SDKTESTSUITE_H

#include "../src/core/blockchain.h" // net/http/httpserver.h, consensus.h -> state.h -> rdpos.h -> net/p2p/managernormal.h, (utils/tx.h -> ecdsa.h -> utils.h -> libs/json.hpp -> tuple), (dump.h -> utils/db.h, storage.h -> options.h)

#include "../src/utils/evmcconv.h"
#include "../src/utils/uintconv.h"

/// Wrapper struct for accounts used within the SDKTestSuite.
struct TestAccount {
  const PrivKey privKey;    ///< Private key of the account.
  const Address address;    ///< Address of the account.
  TestAccount() = default;  ///< Empty Account constructor.

  /**
   * Account constructor.
   * @param privKey_ Private key of the account.
   */
  TestAccount(const PrivKey& privKey_) : privKey(privKey_), address(Secp256k1::toAddress(Secp256k1::toPub(privKey))) {}

  /// Create a new random account.
  inline static TestAccount newRandomAccount() { return TestAccount(PrivKey(Utils::randBytes(32))); }

  /// Operator bool to check if the account is not default, use PrivKey::operator bool.
  explicit operator bool() const { return bool(this->privKey); }
};

/**
 * Helper class for seamlessly managing blockchain components during testing
 * (performing txs, creating and calling contracts).
 */
class SDKTestSuite : public Blockchain {
  private:

    // Test listen P2P port number generator needs to be in SDKTestSuite due to createNewEnvironment(),
    //   which selects the port for the caller.
    // This should be used by all tests that open a node listen port, not only SDKTestSuite tests.
    static int p2pListenPortMin_;
    static int p2pListenPortMax_;
    static int p2pListenPortGen_;

  public:

    /// Get next P2P listen port to use in unit tests.
    static int getTestPort() {
      int tries = 1000;
      boost::asio::io_context io_context;
      while (true) {
        if (p2pListenPortGen_ > p2pListenPortMax_) {
          p2pListenPortGen_ = p2pListenPortMin_;
        } else {
          ++p2pListenPortGen_;
        }
        boost::asio::ip::tcp::acceptor acceptor(io_context);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), p2pListenPortGen_);
        boost::system::error_code ec, ec2;
        acceptor.open(endpoint.protocol(), ec);
        if (!ec) {
          acceptor.bind(endpoint, ec);
          acceptor.close(ec2);
          if (!ec) {
            return p2pListenPortGen_;
          }
        }
        if (--tries <= 0) {
          SLOGFATAL_THROW("Exhausted tries while searching for a free port number");
        }
      }
    }

    /// Construct a test Blockchain.
    explicit SDKTestSuite(const Options& options, const std::string instanceId = "")
      : Blockchain(options, options.getRootPath(), instanceId)
    {
    }

    /// Destructor. Ensure the Blockchain is stopped before it is destroyed.
    ~SDKTestSuite() {
      stop();
    }

    /**
     * Initialize all components of a full blockchain node.
     * @param sdkPath Path to the SDK folder.
     * @param accounts (optional) List of accounts to initialize the blockchain with. Defaults to none (empty vector).
     * @param options (optional) Options to initialize the blockchain with. Defaults to none (nullptr).
     * @param instanceId (optional) String instance ID for logging.
     */
    static SDKTestSuite createNewEnvironment(
      const std::string& sdkPath,
      const std::vector<TestAccount>& accounts = {},
      const Options* const options = nullptr,
      const std::string& instanceId = ""
    );

    /**
     * Create a transaction to deploy a new contract and advance the chain with it.
     * Always use the chain owner account to deploy contracts.
     * Specialization with no args.
     * @tparam TContract Contract type to deploy.
     * @tparam Args... Arguments to pass to the contract constructor.
     * @return Address of the deployed contract.
     */
    template <typename TContract> const Address deployContract() {

      /**
       * TODO:
       *
       * The assumption behind deployContract(), and probably other bits of unit testing functionality,
       * no longer holds, as cometbft does not really give us an "advanceChain()" RPC call -- there's no
       * trivial implementation of "advanceChain()" to be made.
       *
       * Instead, deployContract() will have to contend with the possibility of block advancement involving
       * multiple blocks. That is, it cannot assume that one block is produced. It will have to use the
       * Comet interface to send a deploy-contract transaction, then synchronously wait for it to be
       * accepted, and only then return from here with the address of the successful deployment.
       *
       * TODO: reintroduce createNewTx()
       */
/*
      TContract::registerContract();
      auto prevContractList = this->state_.getCppContracts();
      using ContractArgumentTypes = decltype(Utils::removeQualifiers<typename TContract::ConstructorArguments>());

      // Encode the functor
      std::string createSignature = "createNew" + Utils::getRealTypeName<TContract>() + "Contract("
        + ContractReflectionInterface::getConstructorArgumentTypesString<TContract>() + ")";
      Bytes data;
      Utils::appendBytes(data, Utils::sha3(Utils::create_view_span(createSignature)));
      data.resize(4); // We only need the first 4 bytes for the function signature

      // Create the transaction, advance the chain with it, and get the new contract address.
      TxBlock createContractTx = createNewTx(this->chainOwnerAccount(), ProtocolContractAddresses.at("ContractManager"), 0, data);
      this->advanceChain(0, {createContractTx});
      auto newContractList = this->state_.getCppContracts();

      // Filter new contract list to find the new contract.
      // TODO: We are assuming that only one contract of the same type is deployed at a time.
      // We need to somehow link a transaction to a newly created contract.
      // This can also be used on eth_getTransactionReceipt contractAddress field.
      Address newContractAddress;
      for (const auto& contract : newContractList) {
        if (std::find(prevContractList.begin(), prevContractList.end(), contract) == prevContractList.end()) {
          newContractAddress = contract.second; break;
        }
      }
      return newContractAddress;
*/
      return {};
    }

};

#endif // SDKTESTSUITE_H
