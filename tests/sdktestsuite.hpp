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
#include "../src/utils/contractreflectioninterface.h"

#include "../src/contract/event.h"

/// Value that has been used by SDKTestSuite tests for the uint64_t chainId field in Options
#define DEFAULT_UINT64_TEST_CHAIN_ID 8080

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

    /// Owner of the chain (0x00dead00...).
    static TestAccount chainOwnerAccount() {
      return TestAccount(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")));
    };

    // Test listen P2P port number generator needs to be in SDKTestSuite due to createNewEnvironment(),
    //   which selects the port for the caller.
    // This should be used by all tests that open a node listen port, not only SDKTestSuite tests.
    static int p2pListenPortMin_;
    static int p2pListenPortMax_;
    static int p2pListenPortGen_;

    // Members used by advanceChain(), updated by incomingBlock().
    // You can only call advanceChain() once at a time and it is a fully synchronous call
    std::mutex advanceChainMutex_;
    uint64_t advanceChainHeight_ = 0;
    std::set<Hash> advanceChainPendingTxs_;

  public:

    // long-name getters (expected by existing test code)
    Options& getOptions() { return this->options_; }
    Comet& getComet() { return this->comet_; }
    State& getState() { return this->state_; }
    Storage& getStorage() { return this->storage_; }
    HTTPServer& getHttp() { return this->http_; }

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
    explicit SDKTestSuite(
      const Options& options,
      const std::string instanceId = "",
      const std::vector<TestAccount>& accounts = {}
    )
      : Blockchain(options, options.getRootPath(), instanceId)
    {
      // We need to give some tokens to the chainOwner and to all the
      //  `accounts` that were passed in so they can pay for test contract
      //  deployment, etc.
      state_.addBalance(options_.getChainOwner());
      for (const TestAccount& account : accounts) {
        state_.addBalance(account.address);
      }
      // SimpleContract doesn't call start(), so we must start.
      start();
    }

    /// Destructor. Ensure the Blockchain is stopped before it is destroyed.
    ~SDKTestSuite() {
      stop();
    }

    /// Getter for `chainOwnerAccount_`.
    TestAccount getChainOwnerAccount() const;

    /// Get the native balance of a given address.
    const uint256_t getNativeBalance(const Address& address) const;

    /// Get the nonce of a given address.
    const uint64_t getNativeNonce(const Address& address) const;

    /// Estimate gas.
    int64_t estimateGas(const evmc_message& callInfo);

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
     * Compatibility method for tests that want to advance a cometbft blockchain.
     * Timestamp parameter is removed as you can't control the block timestamp.
     * If given no transactions will fetch the current block and wait for another block to be the head block.
     * If given transactions, will send them and wait for all of them to be included as per the current head block.
     * Does not return a block since we can't know how many blocks will be generated and that's not how you're
     * supposed to test chain or machine state advancement with cometbft.
     * @param txs (optional) List of transactions to include in the block. Defaults to none (empty vector).
     */
    void advanceChain(std::vector<TxBlock>&& txs = {});

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
    );

    /**
     * Create a transaction to deploy a given EVM bytecode and advance the chain with it.
     * Always use the chain owner account to deploy contracts.
     * @param bytecode EVM bytecode to deploy.
     * @return Address of the deployed contract.
     */
    Address deployBytecode(const Bytes& bytecode);

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
      this->advanceChain({createContractTx});
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
      auto prevContractList = this->state_.getCppContracts();
      using ContractArgumentTypes = decltype(Utils::removeQualifiers<typename TContract::ConstructorArguments>());
      static_assert(std::is_same_v<ContractArgumentTypes, std::tuple<std::decay_t<Args>...>>, "Invalid contract constructor arguments");

      // Encode the functor
      std::string createSignature = "createNew" + Utils::getRealTypeName<TContract>() + "Contract("
        + ContractReflectionInterface::getConstructorArgumentTypesString<TContract>() + ")";
      Bytes data;
      Utils::appendBytes(data, Utils::sha3(Utils::create_view_span(createSignature)));
      data.resize(4); // We only need the first 4 bytes for the function signature

      // Encode the arguments
      if (sizeof...(args) > 0) Utils::appendBytes(
        data, ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...)
      );

      // Create the transaction, advance the chain with it, and get the new contract address.
      TxBlock createContractTx = createNewTx(this->chainOwnerAccount(), ProtocolContractAddresses.at("ContractManager"), 0, data);
      this->advanceChain({createContractTx});
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
      TContract::registerContract();
      Hash ret;
      Functor txFunctor = ABI::FunctorEncoder::encode<>(
        ContractReflectionInterface::getFunctionName(func)
      );
      Bytes txData;
      Utils::appendBytes(txData, UintConv::uint32ToBytes(txFunctor.value));
      // Use the chain owner account if no account is provided
      TxBlock tx = this->createNewTx(
        ((!testAccount) ? this->getChainOwnerAccount() : testAccount),
        contractAddress, value, txData
      );
      ret = tx.hash();
      // Check if the execution is not going to be reverted/throw
      this->advanceChain({tx}); // timestamp,
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
      TContract::registerContract();
      Hash ret;
      Functor txFunctor = ABI::FunctorEncoder::encode<Args...>(
        ContractReflectionInterface::getFunctionName(func)
      );
      Bytes txData;
      Utils::appendBytes(txData, UintConv::uint32ToBytes(txFunctor.value));
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
      this->advanceChain({tx}); // timestamp,
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
     * Specialization for function with args with testAccount and timestamp parameters.
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
      TContract::registerContract();
      evmc_message callData;
      auto& [callKind,
        callFlags,
        callDepth,
        callGas,
        callRecipient,
        callSender,
        callInputData,
        callInputSize,
        callValue,
        callCreate2Salt,
        callCodeAddress] = callData;

      auto functor = ABI::FunctorEncoder::encode<>(ContractReflectionInterface::getFunctionName(func));
      Bytes fullData;
      Utils::appendBytes(fullData, UintConv::uint32ToBytes(functor.value));

      callKind = EVMC_CALL;
      callFlags = 0;
      callDepth = 1;
      callGas = 10000000;
      callRecipient = contractAddress.toEvmcAddress();
      callSender = this->getChainOwnerAccount().address.toEvmcAddress();
      callInputData = fullData.data();
      callInputSize = fullData.size();
      callValue = EVMCConv::uint256ToEvmcUint256(0);
      callCreate2Salt = {};
      callCodeAddress = contractAddress.toEvmcAddress();
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
      evmc_message callData;
      auto& [callKind,
        callFlags,
        callDepth,
        callGas,
        callRecipient,
        callSender,
        callInputData,
        callInputSize,
        callValue,
        callCreate2Salt,
        callCodeAddress] = callData;
      auto functor = ABI::FunctorEncoder::encode<Args...>(ContractReflectionInterface::getFunctionName(func));
      Bytes fullData;
      Utils::appendBytes(fullData, UintConv::uint32ToBytes(functor.value));
      Utils::appendBytes(fullData, ABI::Encoder::encodeData<Args...>(std::forward<decltype(args)>(args)...));

      callKind = EVMC_CALL;
      callFlags = 0;
      callDepth = 1;
      callGas = 10000000;
      callRecipient = contractAddress.toEvmcAddress();
      callSender = this->getChainOwnerAccount().address.toEvmcAddress();
      callInputData = fullData.data();
      callInputSize = fullData.size();
      callValue = EVMCConv::uint256ToEvmcUint256(0);
      callCreate2Salt = {};
      callCodeAddress = {};

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
    ) {
      return this->storage_.getEvents(fromBlock, toBlock, address, topics);
    }

    /**
     * Overload of getEvents() used by "eth_getTransactionReceipts", where
     * parameters are filtered differently (by exact tx, not a range).
     * @param txHash The hash of the transaction to look for events.
     * @param blockIndex The height of the block to look for events.
     * @param txIndex The index of the transaction to look for events.
     * @return A list of matching events, limited by the block and/or log caps set above.
     */
    std::vector<Event> getEvents(const uint64_t& blockIndex, const uint64_t& txIndex) {
      return this->storage_.getEvents(blockIndex, txIndex);
    }

    /**
     * Get all events emitted by a given confirmed transaction.
     * @param txHash The hash of the transaction to look for events.
     */
    std::vector<Event> getEvents(const Hash& txHash) {
      GetTxResultType tx = this->getTx(txHash);
      return this->storage_.getEvents(tx.blockHeight, tx.blockIndex);
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
    )  {
      // Get all the events emitted by the transaction.
      auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(
        ContractReflectionInterface::getFunctionName(func)
      );
      std::vector<Hash> topicsToFilter;
      if (!anonymous) topicsToFilter.push_back(eventSignature);
      std::vector<Event> filteredEvents;
      // Specifically filter events from the most recent 2000 blocks
      uint64_t lastBlock = this->state_.getHeight();
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
      uint64_t lastBlock = this->state_.getHeight();
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
    template <typename TFunc> struct FunctionTraits;

    // Specialization for member function pointers
    template <typename TContract, typename... Args, bool... Flags>
    struct FunctionTraits<void(TContract::*)(const EventParam<Args, Flags>&...)> {
      using TupleType = typename Utils::makeTupleType<EventParam<Args, Flags>...>::type;
    };

    /**
     * FIXME: Documentation
     */
    template <typename TContract, typename... Args, bool... Flags>
    auto getEventsEmittedByTxTup(
      const Hash& txHash,
      void(TContract::*func)(const EventParam<Args, Flags>&...),
      bool anonymous = false
    ) {
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

    // ------------------------------------------------------------------
    // CometListener
    // ------------------------------------------------------------------

    virtual void initChain(
      const uint64_t genesisTime, const std::string& chainId, const Bytes& initialAppState, const uint64_t initialHeight,
      const std::vector<CometValidatorUpdate>& initialValidators, Bytes& appHash
    ) override;
    virtual void checkTx(const Bytes& tx, int64_t& gasWanted, bool& accept) override;
    virtual void incomingBlock(
      const uint64_t syncingToHeight, std::unique_ptr<CometBlock> block, Bytes& appHash,
      std::vector<CometExecTxResult>& txResults, std::vector<CometValidatorUpdate>& validatorUpdates
    ) override;
    virtual void buildBlockProposal(
      const uint64_t maxTxBytes, const CometBlock& block, bool& noChange, std::vector<size_t>& txIds
    ) override;
    virtual void validateBlockProposal(const CometBlock& block, bool& accept) override;
    virtual void getCurrentState(uint64_t& height, Bytes& appHash, std::string& appSemVer, uint64_t& appVersion) override;
    virtual void getBlockRetainHeight(uint64_t& height) override;
    virtual void currentCometBFTHeight(const uint64_t height) override;
    virtual void sendTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash, const Bytes& tx) override;
    virtual void checkTransactionResult(const uint64_t tId, const bool success, const json& response, const std::string& txHash) override;
    virtual void rpcAsyncCallResult(const uint64_t tId, const bool success, const json& response, const std::string& method, const json& params) override;
    virtual void cometStateTransition(const CometState newState, const CometState oldState) override;
};

#endif // SDKTESTSUITE_H
