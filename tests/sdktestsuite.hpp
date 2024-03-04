/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef SDKTESTSUITE_H
#define SDKTESTSUITE_H

#include "../src/core/storage.h"
#include "../src/core/rdpos.h"
#include "../src/core/state.h"
#include "../src/net/p2p/managernormal.h"
#include "../src/net/http/httpserver.h"
#include "../src/utils/options.h"
#include "../src/utils/db.h"
#include "../src/core/blockchain.h"
#include "../src/utils/utils.h"

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
  inline static TestAccount newRandomAccount() { return TestAccount(Utils::randBytes(32)); }

  /// Operator bool to check if the account is not default, use PrivKey::operator bool.
  explicit operator bool() const { return bool(this->privKey); }
};

/**
 * Helper class for seamlessly managing blockchain components during testing
 * (performing txs, creating and calling contracts).
 */
class SDKTestSuite {
  private:
    const Options options_;      ///< Options singleton.
    DB db_;                      ///< Database.
    Storage storage_;            ///< Blockchain storage.
    State state_;                ///< Blockchain state.
    P2P::ManagerNormal p2p_;     ///< P2P connection manager.
    HTTPServer http_;            ///< HTTP server.

    /// Owner of the chain (0x00dead00...).
    static TestAccount chainOwnerAccount() {
      return {Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")};
    };

    /// PrivateKeys of the validators for the rdPoS within SDKTestSuite.
    static std::vector<PrivKey> validatorPrivKeys() {
      return {
        PrivKey(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
        PrivKey(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
        PrivKey(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
        PrivKey(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
        PrivKey(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
        PrivKey(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
        PrivKey(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
        PrivKey(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
      };
    };

  public:
    /**
     * Constructor for SDKTestSuite based on a given Options.
     */
    explicit SDKTestSuite(const Options& options) :
      options_(options),
      db_(options_.getRootPath() + "/db"),
      storage_(db_, options_),
      state_(db_, storage_, p2p_, options_),
      p2p_(boost::asio::ip::address::from_string("127.0.0.1"), options_, storage_, state_),
      http_(state_, storage_, p2p_, options_)
    {}

    ~SDKTestSuite() {
      state_.rdposStopWorker();
      p2p_.stopDiscovery();
      p2p_.stop();
      http_.stop();
    }

    /**
     * Initialize all components of a full blockchain node.
     * @param sdkPath Path to the SDK folder.
     * @param accounts (optional) List of accounts to initialize the blockchain with. Defaults to none (empty vector).
     * @param options (optional) Options to initialize the blockchain with. Defaults to none (nullptr).
     */
    static SDKTestSuite createNewEnvironment(
      const std::string& sdkPath,
      const std::vector<TestAccount>& accounts = {},
      const Options* const options = nullptr
    ) {
      // Initialize the DB
      std::string dbPath = sdkPath + "/db";
      if (std::filesystem::exists(dbPath)) std::filesystem::remove_all(dbPath);

      // Create a default options if none is provided.
      std::unique_ptr<Options> options_;
      if (options == nullptr) {
        // Create a genesis block with a timestamp of 1678887538000000 (2023-02-12 00:45:38 UTC)
        uint64_t genesisTimestamp = 1678887538000000;
        PrivKey genesisSigner(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8"));
        Block genesis(Hash(), 0, 0);
        genesis.finalize(genesisSigner, genesisTimestamp);
        std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
        std::vector<std::pair<Address,uint256_t>> genesisBalances;
        // Add the chain owner account to the genesis balances.
        const uint256_t desiredBalance("1000000000000000000000");
        genesisBalances.emplace_back(chainOwnerAccount().address, desiredBalance);
        // Add the remaining accounts to the genesis balances.
        for (const TestAccount& account : accounts) {
          genesisBalances.emplace_back(account.address, desiredBalance);
        }
        std::vector<Address> genesisValidators;
        for (const auto& privKey : validatorPrivKeys()) {
          genesisValidators.push_back(Secp256k1::toAddress(Secp256k1::toUPub(privKey)));
        }
        options_ = std::make_unique<Options>(
          sdkPath,
          "OrbiterSDK/cpp/linux_x86-64/0.2.0",
          1,
          8080,
          Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
          8080,
          9999,
          11,
          11,
          200,
          50,
          2000,
          10000,
          4,
          false,
          discoveryNodes,
          Address(),
          genesis,
          genesisTimestamp,
          genesisSigner,
          genesisBalances,
          genesisValidators,
          nullptr
        );
      } else {
        options_ = std::make_unique<Options>(*options);
      }
      return SDKTestSuite(*options_);
    }

    /**
     * Get a block by its hash.
     * @param hash Hash of the block to get.
     * @return A pointer to the block, or nullptr if not found.
     */
    const std::shared_ptr<const Block> getBlock(const Hash& hash) const {
      return this->storage_.getBlock(hash);
    }

    /**
     * Get a block by its height.
     * @param height Height of the block to get.
     * @return A pointer to the block, or nullptr if not found.
     */
    const std::shared_ptr<const Block> getBlock(const uint64_t height) const {
      return this->storage_.getBlock(height);
    }

    /**
     * Create a new valid block, finalize it, and add it to the chain.
     * @param timestamp (optional) Timestamp to use for the block in microseconds. Defaults to 0.
     * @param txs (optional) List of transactions to include in the block. Defaults to none (empty vector).
     * @return A pointer to the new block.
     */
    const std::shared_ptr<const Block> advanceChain(const uint64_t& timestamp = 0, const std::vector<TxBlock>& txs = {}) {
      auto validators = state_.rdposGetValidators();
      auto randomList = state_.rdposGetRandomList();
      Hash blockSignerPrivKey;           // Private key for the block signer.
      std::vector<Hash> orderedPrivKeys; // Private keys for the rdPoS in the order of the random list, limited to rdPoS' minValidators.
      orderedPrivKeys.reserve(4);
      for (const auto& privKey : this->validatorPrivKeys()) {
        if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[0]) {
          blockSignerPrivKey = privKey; break;
        }
      }
      for (uint64_t i = 1; i < state_.rdposGetMinValidators() + 1; i++) {
        for (const auto& privKey : this->validatorPrivKeys()) {
          if (Secp256k1::toAddress(Secp256k1::toUPub(privKey)) == randomList[i]) {
            orderedPrivKeys.push_back(privKey); break;
          }
        }
      }

      // By now we should have randomList[0] privKey in blockSignerPrivKey and
      // the rest in orderedPrivKeys, ordered by the random list.
      // We can proceed with creating the block, transactions have to be
      // **ordered** by the random list.

      // Create a block with 8 TxValidator transactions, 2 for each validator, in order (randomHash and random)
      uint64_t newBlocknHeight = this->storage_.latest()->getNHeight() + 1;
      uint64_t newBlockTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
      ).count();
      Hash newBlockPrevHash = this->storage_.latest()->hash();
      Block newBlock(newBlockPrevHash, newBlockTimestamp, newBlocknHeight);
      std::vector<TxValidator> randomHashTxs;
      std::vector<TxValidator> randomTxs;

      std::vector<Hash> randomSeeds(orderedPrivKeys.size(), Hash::random());
      for (uint64_t i = 0; i < orderedPrivKeys.size(); ++i) {
        Address validatorAddress = Secp256k1::toAddress(Secp256k1::toUPub(orderedPrivKeys[i]));
        Bytes hashTxData = Hex::toBytes("0xcfffe746");
        Utils::appendBytes(hashTxData, Utils::sha3(randomSeeds[i].get()));
        Bytes randomTxData = Hex::toBytes("0x6fc5a2d6");
        Utils::appendBytes(randomTxData, randomSeeds[i].get());
        randomHashTxs.emplace_back(
          validatorAddress, hashTxData, 8080, newBlocknHeight, orderedPrivKeys[i]
        );
        randomTxs.emplace_back(
          validatorAddress, randomTxData, 8080, newBlocknHeight, orderedPrivKeys[i]
        );
      }

      // Append the transactions to the block.
      for (const auto& tx : randomHashTxs) {
        this->state_.rdposAddValidatorTx(tx); newBlock.appendTxValidator(tx);
      }
      for (const auto& tx : randomTxs) {
        this->state_.rdposAddValidatorTx(tx); newBlock.appendTxValidator(tx);
      }
      for (const auto& tx : txs) newBlock.appendTx(tx);

      // Finalize the block.
      if (timestamp == 0) {
        newBlock.finalize(blockSignerPrivKey, std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch()
        ).count());
      } else {
        newBlock.finalize(blockSignerPrivKey, timestamp);
      }

      // After finalization, the block should be valid. If it is, process the next one.
      if (!this->state_.validateNextBlock(newBlock)) throw DynamicException(
        "SDKTestSuite::advanceBlock: Block is not valid"
      );
      state_.processNextBlock(std::move(newBlock));
      return this->storage_.latest();
    }

    /**
     * Create a new TxBlock object based on the provided account and given the current state (for nonce).
     * @param TestAccount from Account to send from. (the private key to sign the transaction will be taken from here)
     * @param Address to Address to send to.
     * @param uint256_t value Amount to send.
     * @param Bytes (optional) data Data to send. Defaults to nothing (empty bytes).
     * @return The newly created transaction.
     */
    TxBlock createNewTx(
      const TestAccount& from, const Address& to, const uint256_t& value, Bytes data = Bytes()
    ) {
      // 1000000000 = 1 GWEI, 21000 = 21000 WEI
      return TxBlock(to, from.address, data, this->options_.getChainID(),
        this->state_.getNativeNonce(from.address), value, 1000000000, 1000000000, 21000, from.privKey
      );
    }

    /**
     * Make a simple transfer transaction. Automatically advances the chain.
     * @param from Account to send from. (the private key to sign the transaction will be taken from here)
     * @param to Address to send to.
     * @param value Amount to send.
     * @return The hash of the transaction.
     */
    const Hash transfer(const TestAccount& from, const Address& to, const uint256_t& value) {
      TxBlock tx = createNewTx(from, to, value);
      this->advanceChain(0, {tx});
      return tx.hash();
    }

    /**
     * Get all events emitted under a given transaction.
     * @param txHash The hash of the transaction to look for events.
     * @return a vector of events emitted by the transaction.
     */
    const std::vector<Event> getEvents(const Hash& tx) const {
      auto txBlock = this->storage_.getTx(tx);
      return this->state_.getEvents(
        std::get<0>(txBlock)->hash(), std::get<3>(txBlock), std::get<2>(txBlock)
      );
    }

    /**
     * Get the latest accepted block.
     * @return A pointer to the latest accepted block.
     */
    inline const std::shared_ptr<const Block> getLatestBlock() const { return this->storage_.latest(); }

    /**
     * Get a transaction from the chain using a given hash.
     * @param tx The transaction hash to get.
     * @return A tuple with the found transaction, block hash, index and height, nullptr if the tx was not found
     * @throw DynamicException on hash mismatch (should never happen).
     */
    const std::tuple<
      const std::shared_ptr<const TxBlock>, const Hash, const uint64_t, const uint64_t
    > getTx(const Hash& tx) { return this->storage_.getTx(tx); }

    /**
     * Create a transaction to deploy a new contract and advance the chain with it.
     * Always use the chain owner account to deploy contracts.
     * Specialization with no args.
     * @tparam TContract Contract type to deploy.
     * @tparam Args... Arguments to pass to the contract constructor.
     * @return Address of the deployed contract.
     */
    template <typename TContract> const Address deployContract() {
      TContract::registerContract();
      auto prevContractList = this->state_.getContracts();
      using ContractArgumentTypes = decltype(Utils::removeQualifiers<typename TContract::ConstructorArguments>());

      // Encode the functor
      std::string createSignature = "createNew" + Utils::getRealTypeName<TContract>() + "Contract("
        + ContractReflectionInterface::getConstructorArgumentTypesString<TContract>() + ")";
      Functor functor = Utils::sha3(Utils::create_view_span(createSignature)).view(0, 4);
      Bytes data(functor.cbegin(), functor.cend());

      // Create the transaction, advance the chain with it, and get the new contract address.
      TxBlock createContractTx = createNewTx(this->chainOwnerAccount(), ProtocolContractAddresses.at("ContractManager"), 0, data);
      this->advanceChain(0, {createContractTx});
      auto newContractList = this->state_.getContracts();

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
    }

    /**
     * Create a transaction to deploy a new contract and advance the chain with it.
     * Always use the chain owner account to deploy contracts.
     * Specialization with args.
     * @tparam TContract Contract type to deploy.
     * @tparam Args... Arguments to pass to the contract constructor.
     * @return Address of the deployed contract.
     */
    template <typename TContract, typename ...Args> const Address deployContract(Args&&... args) {
      TContract::registerContract();
      auto prevContractList = this->state_.getContracts();
      using ContractArgumentTypes = decltype(Utils::removeQualifiers<typename TContract::ConstructorArguments>());
      static_assert(std::is_same_v<ContractArgumentTypes, std::tuple<std::decay_t<Args>...>>, "Invalid contract constructor arguments");

      // Encode the functor
      std::string createSignature = "createNew" + Utils::getRealTypeName<TContract>() + "Contract("
        + ContractReflectionInterface::getConstructorArgumentTypesString<TContract>() + ")";
      Functor functor = Utils::sha3(Utils::create_view_span(createSignature)).view(0, 4);
      Bytes data(functor.cbegin(), functor.cend());

      // Encode the arguments
      if (sizeof...(args) > 0) Utils::appendBytes(
        data, ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...)
      );

      // Create the transaction, advance the chain with it, and get the new contract address.
      TxBlock createContractTx = createNewTx(this->chainOwnerAccount(), ProtocolContractAddresses.at("ContractManager"), 0, data);
      this->advanceChain(0, {createContractTx});
      auto newContractList = this->state_.getContracts();

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
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function without args.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @param contractAddress Address of the contract to call.
     * @param func Function to call.
     * @param value (optional) Value to send with the transaction. Defaults to 0.
     * @param testAccount (optional) Account to send the transaction from. Defaults to none (empty account).
     * @param timestamp (optional) Timestamp to use for the transaction in microseconds. Defaults to 0.
     * @return The hash of the transaction.
     */
    template <typename ReturnType, typename TContract> const Hash callFunction(
      const Address& contractAddress,
      ReturnType(TContract::*func)(),
      const uint256_t& value = 0,
      const TestAccount& testAccount = TestAccount(),
      const uint64_t& timestamp = 0
    ) {
      // Create the transaction data
      Hash ret;
      Functor txFunctor = ABI::FunctorEncoder::encode<>(
        ContractReflectionInterface::getFunctionName(func)
      );
      Bytes txData(txFunctor.cbegin(), txFunctor.cend());
      // Use the chain owner account if no account is provided
      TxBlock tx = this->createNewTx(
        ((!testAccount) ? this->getChainOwnerAccount() : testAccount),
        contractAddress, value, txData
      );
      ret = tx.hash();
      // Check if the execution is not going to be reverted/throw
      this->state_.estimateGas(tx.txToCallInfo());
      this->advanceChain(timestamp, {tx});
      return ret;
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args.
     * We are not able to set default values like in the other specialization because of the variadic template.
     * Therefore we need functions with no value/testAccount/timestamp parameters.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param value Value to send with the transaction.
     * @param testAccount Account to send the transaction from.
     * @param timestamp Timestamp to use for the transaction in microseconds.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const Hash callFunction(
      const Address& contractAddress,
      const uint256_t& value,
      const TestAccount& testAccount,
      const uint64_t& timestamp,
      ReturnType(TContract::*func)(const Args&...),
      const Args&... args
    ) {
      // Create the transaction data
      Hash ret;
      Functor txFunctor = ABI::FunctorEncoder::encode<Args...>(
        ContractReflectionInterface::getFunctionName(func)
      );
      Bytes txData(txFunctor.cbegin(), txFunctor.cend());
      Utils::appendBytes(
        txData, ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...)
      );
      // Use the chain owner account if no account is provided
      TxBlock tx = this->createNewTx(
        ((!testAccount) ? this->getChainOwnerAccount() : testAccount),
        contractAddress, value, txData
      );
      ret = tx.hash();
      // Check if the execution is not going to be reverted/throw
      this->state_.estimateGas(tx.txToCallInfo());
      this->advanceChain(timestamp, {tx});
      return ret;
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with no value/testAccount/timestamp parameters.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const Hash callFunction(
      const Address& contractAddress,
      ReturnType(TContract::*func)(const Args&...),
      const Args&... args
    ) {
      return this->callFunction(
        contractAddress, 0, this->getChainOwnerAccount(),
        0, func, std::forward<decltype(args)>(args)...
      );
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with only a value parameter.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param value Value to send with the transaction.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const Hash callFunction(
      const Address& contractAddress,
      uint256_t value,
      ReturnType(TContract::*func)(const Args&...),
      const Args&... args
    ) {
      return this->callFunction(
        contractAddress, value, this->getChainOwnerAccount(),
        0, func, std::forward<decltype(args)>(args)...
      );
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with only testAccount
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param testAccount Value to send with the transaction.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const Hash callFunction(
      const Address& contractAddress,
      const TestAccount& testAccount,
      ReturnType(TContract::*func)(const Args&...),
      const Args&... args
    ) {
      return this->callFunction(
        contractAddress, 0, testAccount, 0,
        func, std::forward<decltype(args)>(args)...
      );
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with only timestamp parameter.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param timestamp Timestamp to use for the block in microseconds.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const Hash callFunction(
      const Address& contractAddress,
      uint64_t timestamp,
      ReturnType(TContract::*func)(const Args&...),
      const Args&... args
    ) {
      return this->callFunction(
        contractAddress, 0, this->getChainOwnerAccount(),
        timestamp, func, std::forward<decltype(args)>(args)...
      );
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with value and testAccount parameters
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param value Value to send with the transaction.
     * @param testAccount Account to send the transaction from.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const Hash callFunction(
      const Address& contractAddress,
      uint256_t value,
      const TestAccount& testAccount,
      ReturnType(TContract::*func)(const Args&...),
      const Args&... args
    ) {
      return this->callFunction(
        contractAddress, value, testAccount,
        0, func, std::forward<decltype(args)>(args)...
      );
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with value and timestamp parameters.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param value Value to send with the transaction.
     * @param timestamp Timestamp to use for the block in microseconds.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const Hash callFunction(
      const Address& contractAddress,
      uint256_t value,
      uint64_t timestamp,
      ReturnType(TContract::*func)(const Args&...),
      const Args&... args
    ) {
      return this->callFunction(
        contractAddress, value, this->getChainOwnerAccount(),
        timestamp, func, std::forward<decltype(args)>(args)...
      );
    }

    /**
     * Create a transaction to call a contract function and advance the chain with it.
     * Specialization for function with args with value and timestamp parameters.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param testAccount Account to send the transaction from.
     * @param timestamp Timestamp to use for the block in microseconds.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const Hash callFunction(
      const Address& contractAddress,
      const TestAccount& testAccount,
      uint64_t timestamp,
      ReturnType(TContract::*func)(const Args&...),
      const Args&... args
    ) {
      return this->callFunction(
        contractAddress, 0, testAccount, timestamp,
        func, std::forward<decltype(args)>(args)...
      );
    }

    /**
     * Call a contract view function with no args and return the result.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @param contractAddress Address of the contract to call.
     * @param func Function to call.
     * @return The result of the function call. (ReturnType)
     */
    template <typename ReturnType, typename TContract>
    const ReturnType callViewFunction(
      const Address& contractAddress, ReturnType(TContract::*func)() const
    ) {
      ethCallInfoAllocated callData;
      auto& [fromInfo, toInfo, gasInfo, gasPriceInfo, valueInfo, functorInfo, dataInfo] = callData;
      toInfo = contractAddress;
      functorInfo = ABI::FunctorEncoder::encode<>(ContractReflectionInterface::getFunctionName(func));
      dataInfo = Bytes();
      return std::get<0>(ABI::Decoder::decodeData<ReturnType>(this->state_.ethCall(callData)));
    }

    /**
     * Call a contract view function with args and return the result.
     * @tparam ReturnType Return type of the function.
     * @tparam TContract Contract type to call.
     * @tparam Args... Arguments to pass to the function.
     * @param contractAddress Address of the contract to call.
     * @param func Function to call.
     * @param args Arguments to pass to the function.
     * @return The result of the function call. (ReturnType)
     */
    template <typename ReturnType, typename TContract, typename ...Args>
    const ReturnType callViewFunction(
      const Address& contractAddress,
      ReturnType(TContract::*func)(const Args&...) const,
      const Args&... args
    ) {
      TContract::registerContract();
      ethCallInfoAllocated callData;
      auto& [fromInfo, toInfo, gasInfo, gasPriceInfo, valueInfo, functorInfo, dataInfo] = callData;
      toInfo = contractAddress;
      functorInfo = ABI::FunctorEncoder::encode<Args...>(ContractReflectionInterface::getFunctionName(func));
      dataInfo = ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...);
      return std::get<0>(ABI::Decoder::decodeData<ReturnType>(this->state_.ethCall(callData)));
    }

    /**
     * Get all the events emitted under the given inputs.
     * @param fromBlock The initial block height to look for.
     * @param toBlock The final block height to look for.
     * @param address The address to look for. If empty, will look for all available addresses.
     * @param topics The topics to filter by. If empty, will look for all available topics.
     * @return A list of matching events, limited by the block and/or log caps set above.
     */
    std::vector<Event> getEvents(
      const uint64_t& fromBlock, const uint64_t& toBlock,
      const Address& address, const std::vector<Hash>& topics
    ) { return this->state_.getEvents(fromBlock, toBlock, address, topics); }

    /**
     * Overload of getEvents() used by "eth_getTransactionReceipts", where
     * parameters are filtered differently (by exact tx, not a range).
     * @param txHash The hash of the transaction to look for events.
     * @param blockIndex The height of the block to look for events.
     * @param txIndex The index of the transaction to look for events.
     * @return A list of matching events, limited by the block and/or log caps set above.
     */
    std::vector<Event> getEvents(
      const Hash& txHash, const uint64_t& blockIndex, const uint64_t& txIndex
    ) { return this->state_.getEvents(txHash, blockIndex, txIndex); }

    /**
     * Get all events emitted by a given confirmed transaction.
     * @param txHash The hash of the transaction to look for events.
     */
    std::vector<Event> getEvents(const Hash& txHash) {
      auto tx = this->storage_.getTx(txHash);
      return this->state_.getEvents(std::get<0>(tx)->hash(), std::get<3>(tx), std::get<2>(tx));
    }

    /**
     * Get events emitted by a given address.
     * Specialization without args (will not filter indexed args).
     * @tparam TContract Contract type to look for.
     * @tparam Args... Arguments to pass to the EventParam.
     * @tparam Flags... Flags to pass to the EventParam.
     * @param address The address to look for events.
     * @param func The function to look for.
     * @param anonymous (optional) Whether the event is anonymous or not. Defaults to false.
     * @return A list of emitted events from the address.
     */
    template <typename TContract, typename... Args, bool... Flags>
    std::vector<Event> getEventsEmittedByAddress(
      const Address& address,
      void(TContract::*func)(const EventParam<Args, Flags>&...),
      bool anonymous = false
    ) {
      // Get all the events emitted by the transaction.
      auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(
        ContractReflectionInterface::getFunctionName(func)
      );
      std::vector<Hash> topicsToFilter;
      if (!anonymous) topicsToFilter.push_back(eventSignature);
      std::vector<Event> filteredEvents;
      // Specifically filter events from the most recent 2000 blocks
      uint64_t lastBlock = this->storage_.latest()->getNHeight();
      uint64_t firstBlock = (lastBlock - 2000 >= 0) ? lastBlock - 2000 : 0;
      auto allEvents = this->getEvents(firstBlock, lastBlock, address, {});

      // Filter the events by the topics
      for (const auto& event : allEvents) {
        if (topicsToFilter.size() == 0) {
          filteredEvents.push_back(event);
        } else {
          if (event.getTopics().size() < topicsToFilter.size()) continue;
          bool match = true;
          for (uint64_t i = 0; i < topicsToFilter.size(); i++) {
            if (topicsToFilter[i] != event.getTopics()[i]) { match = false; break; }
          }
          if (match) filteredEvents.push_back(event);
        }
      }
      return filteredEvents;
    }

    /**
     * Get events emitted by a given address.
     * Specialization with args (topics).
     * @tparam TContract Contract type to look for.
     * @tparam Args... Arguments to pass to the EventParam.
     * @tparam Flags... Flags to pass to the EventParam.
     * @param address The address to look for events.
     * @param func The function to look for.
     * @param args The topics to search for.
     * @param anonymous (optional) Whether the event is anonymous or not. Defaults to false.
     * @return A list of emitted events from the address.
     */
    template <typename TContract, typename... Args, bool... Flags>
    std::vector<Event> getEventsEmittedByAddress(
      const Address& address,
      void(TContract::*func)(const EventParam<Args, Flags>&...),
      const std::tuple<EventParam<Args, Flags>...>& args,
      bool anonymous = false
    ) {
      // Get all the events emitted by the transaction.
      auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(
        ContractReflectionInterface::getFunctionName(func)
      );
      std::vector<Hash> topicsToFilter;
      if (!anonymous) topicsToFilter.push_back(eventSignature);
      std::apply([&](const auto&... param) {
        (..., (param.isIndexed ? topicsToFilter.push_back(ABI::EventEncoder::encodeTopicSignature(param.value)) : void()));
      }, args);

      if (topicsToFilter.size() > 4) topicsToFilter.resize(4); // Force max topic size to 4

      // Filter the events by the topics, from the most recent 2000 blocks
      std::vector<Event> filteredEvents;
      uint64_t lastBlock = this->storage_.latest()->getNHeight();
      uint64_t firstBlock = (lastBlock > 2000) ? lastBlock - 2000 : 0;
      auto allEvents = this->getEvents(firstBlock, lastBlock, address, {});
      for (const auto& event : allEvents) {
        if (topicsToFilter.size() == 0) {
          filteredEvents.push_back(event);
        } else {
          if (event.getTopics().size() < topicsToFilter.size()) continue;
          bool match = true;
          for (uint64_t i = 0; i < topicsToFilter.size(); i++) {
            if (topicsToFilter[i] != event.getTopics()[i]) { match = false; break; }
          }
          if (match) filteredEvents.push_back(event);
        }
      }
      return filteredEvents;
    }

    // Forward declaration for the extractor
      template <typename TFunc>
      struct FunctionTraits;  // Forward declaration

      // Specialization for member function pointers
      template <typename TContract, typename... Args, bool... Flags>
      struct FunctionTraits<void(TContract::*)(const EventParam<Args, Flags>&...)> {
          using TupleType = typename Utils::makeTupleType<EventParam<Args, Flags>...>::type;
      };
    template <typename TContract, typename... Args, bool... Flags>
    auto getEventsEmittedByTxTup(const Hash& txHash,
                                void(TContract::*func)(const EventParam<Args, Flags>&...),
                                bool anonymous = false) {
        // Get all the events emitted by the transaction.
        using TupleType = typename FunctionTraits<decltype(func)>::TupleType;

        //if TupleType is a empty tuple, then we throw an error
        auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(
            ContractReflectionInterface::getFunctionName(func)
        );
        std::vector<Hash> topicsToFilter;
        static_assert(ABI::always_false<TupleType>, "");
        if (!anonymous) topicsToFilter.push_back(eventSignature);
        std::vector<Event> filteredEvents;
        auto allEvents = this->getEvents(txHash);

        // Filter the events by the topics
        for (const auto& event : allEvents) {
            if (topicsToFilter.size() == 0) {
                filteredEvents.push_back(event);
            } else {
                if (event.getTopics().size() < topicsToFilter.size()) continue;
                bool match = true;
                for (uint64_t i = 0; i < topicsToFilter.size(); i++) {
                    if (topicsToFilter[i] != event.getTopics()[i]) { match = false; break; }
                }
                if (match) filteredEvents.push_back(event);
            }
        }

        // Process each filtered event to get the tuple of non-indexed arguments
        std::vector<TupleType> tuples;
        if constexpr (!std::is_same_v<TupleType, std::tuple<>>) {
            for (const auto& event : filteredEvents) {
                auto tuple = ABI::Decoder::decodeDataAsTuple<TupleType>::decode(event.getData());
                tuples.push_back(tuple);
            }
        } else {
            throw DynamicException("Attempted to decode an event with only indexed parameters (empty tuple).");
        }

        return tuples;
    }

    /**
     * Get events emitted by a given confirmed transaction.
     * Specialization without args (will not filter indexed args).
     * @tparam TContract Contract type to look for.
     * @tparam Args... Arguments to pass to the EventParam.
     * @tparam Flags... Flags to pass to the EventParam.
     * @param txHash The hash of the transaction to look for events.
     * @param func The function to look for.
     * @param anonymous (optional) Whether the event is anonymous or not. Defaults to false.
     * @return A list of emitted events from the tx.
     */
    template <typename TContract, typename... Args, bool... Flags>
    std::vector<Event> getEventsEmittedByTx(
      const Hash& txHash,
      void(TContract::*func)(const EventParam<Args, Flags>&...),
      bool anonymous = false
    ) {
      // Get all the events emitted by the transaction.
      auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(
        ContractReflectionInterface::getFunctionName(func)
      );
      std::vector<Hash> topicsToFilter;
      if (!anonymous) topicsToFilter.push_back(eventSignature);
      std::vector<Event> filteredEvents;
      auto allEvents = this->getEvents(txHash);

      // Filter the events by the topics
      for (const auto& event : allEvents) {
        if (topicsToFilter.size() == 0) {
          filteredEvents.push_back(event);
        } else {
          if (event.getTopics().size() < topicsToFilter.size()) continue;
          bool match = true;
          for (uint64_t i = 0; i < topicsToFilter.size(); i++) {
            if (topicsToFilter[i] != event.getTopics()[i]) { match = false; break; }
          }
          if (match) filteredEvents.push_back(event);
        }
      }
      return filteredEvents;
    }

    /**
     * Get events emitted by a given confirmed transaction.
     * Specialization with args (topics).
     * @tparam TContract Contract type to look for.
     * @tparam Args... Arguments to pass to the EventParam.
     * @tparam Flags... Flags to pass to the EventParam.
     * @param txHash The hash of the transaction to look for events.
     * @param func The function to look for.
     * @param args The topics to search for.
     * @param anonymous (optional) Whether the event is anonymous or not. Defaults to false.
     * @return A list of emitted events from the tx.
     */
    template <typename TContract, typename... Args, bool... Flags>
    std::vector<Event> getEventsEmittedByTx(
      const Hash& txHash,
      void(TContract::*func)(const EventParam<Args, Flags>&...),
      const std::tuple<EventParam<Args, Flags>...>& args,
      bool anonymous = false
    ) {
      // Get all the events emitted by the transaction.
      auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(
        ContractReflectionInterface::getFunctionName(func)
      );
      std::vector<Hash> topicsToFilter;
      if (!anonymous) topicsToFilter.push_back(eventSignature);
      std::apply([&](const auto&... param) {
        (..., (param.isIndexed ? topicsToFilter.push_back(ABI::EventEncoder::encodeTopicSignature(param.value)) : void()));
      }, args);

      if (topicsToFilter.size() > 4) topicsToFilter.resize(4); // Force max topic size to 4

      // Filter the events by the topics
      std::vector<Event> filteredEvents;
      auto allEvents = this->getEvents(txHash);
      for (const auto& event : allEvents) {
        if (topicsToFilter.size() == 0) {
          filteredEvents.push_back(event);
        } else {
          if (event.getTopics().size() < topicsToFilter.size()) continue;
          bool match = true;
          for (uint64_t i = 0; i < topicsToFilter.size(); i++) {
            if (topicsToFilter[i] != event.getTopics()[i]) { match = false; break; }
          }
          if (match) filteredEvents.push_back(event);
        }
      }
      return filteredEvents;
    }

    /// Getter for `chainOwnerAccount_`.
    TestAccount getChainOwnerAccount() const { return this->chainOwnerAccount(); };

    /// Getter for `options_`.
    const Options& getOptions() const { return this->options_; };

    /// Getter for `db_`.
    const DB& getDB() { return this->db_; };

    /// Getter for `storage_`.
    Storage& getStorage() { return this->storage_; };

    /// Getter for `state_`.
    State& getState() { return this->state_; };

    /// Getter for `p2p_`.
    P2P::ManagerNormal& getP2P() { return this->p2p_; };

    /// Getter for `http_`.
    HTTPServer& getHTTP() { return this->http_; };

    /// Get the native balance of a given address.
    const uint256_t getNativeBalance(const Address& address) const {
      return this->state_.getNativeBalance(address);
    }

    /// Get the nonce of a given address.
    const uint64_t getNativeNonce(const Address& address) const {
      return this->state_.getNativeNonce(address);
    }

    /// Initialize the P2P and HTTP servers.
    void initializeServices() { this->p2p_.start(); this->http_.start(); }

    /// Stop the P2P and HTTP servers.
    void stopServices() { this->http_.stop(); this->p2p_.stop(); }
};

#endif // SDKTESTSUITE_H
