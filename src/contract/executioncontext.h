#ifndef BDK_EXECUTIONCONTEXT_H
#define BDK_EXECUTIONCONTEXT_H

#include <stack>
#include "utils/hash.h"
#include "utils/address.h"
#include "utils/utils.h"
#include "utils/transactional.h"
#include "utils/safehash.h"
#include "contract/contract.h"
#include "contract/event.h"

class ExecutionContext {
public:
  using Storage = boost::unordered_flat_map<StorageKey, Hash, SafeHash, SafeCompare>;
  using Accounts = boost::unordered_flat_map<Address, NonNullUniquePtr<Account>, SafeHash, SafeCompare>;
  using Contracts = boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash, SafeCompare>;

  class Checkpoint;

  class Builder;

  class AccountPointer;

  ExecutionContext(
    Accounts& accounts, Storage& storage, Contracts& contracts,
    int64_t blockGasLimit,  int64_t blockNumber, int64_t blockTimestamp, int64_t txIndex,
    View<Address> blockCoinbase, View<Address> txOrigin, View<Hash> blockHash, View<Hash> txHash,
    const uint256_t& chainId, const uint256_t& txGasPrice) :
    accounts_(accounts), storage_(storage), contracts_(contracts), newContracts_(),
    blockGasLimit_(blockGasLimit), blockNumber_(blockNumber), blockTimestamp_(blockTimestamp), txIndex_(txIndex),
    blockCoinbase_(blockCoinbase), txOrigin_(txOrigin), blockHash_(blockHash), txHash_(txHash),
    chainId_(chainId), txGasPrice_(txGasPrice) {}

  ~ExecutionContext() { revert(); }

  ExecutionContext(const ExecutionContext&) = delete;

  ExecutionContext(ExecutionContext&&) = delete;

  ExecutionContext& operator=(const ExecutionContext&) = delete;

  ExecutionContext& operator=(ExecutionContext&&) = delete;

  View<Hash> getBlockHash() const { return blockHash_; }

  View<Hash> getTxHash() const { return txHash_; }

  View<Address> getTxOrigin() const { return txOrigin_; }

  View<Address> getBlockCoinbase() const { return blockCoinbase_; }

  int64_t getBlockGasLimit() const { return blockGasLimit_; }

  int64_t getBlockNumber() const { return blockNumber_; }

  int64_t getBlockTimestamp() const { return blockTimestamp_; }

  int64_t getTxIndex() const { return txIndex_; }

  const uint256_t& getTxGasPrice() const { return txGasPrice_; }

  const uint256_t& getChainId() const { return chainId_; }

  void addEvent(Event event);

  void addEvent(std::string_view name, View<Address> address, const auto& args, bool anonymous) {
    Event event(std::string(name), events_.size(), Hash(this->getTxHash()), this->getTxIndex(), Hash(this->getBlockHash()),
      this->getBlockNumber(), Address(address), args, anonymous);

    this->addEvent(std::move(event));
  }

  void addEvent(View<Address> address, View<Bytes> data, std::vector<Hash> topics);

  bool accountExists(View<Address> accountAddress) const;

  AccountPointer getAccount(View<Address> accountAddress);

  BaseContract& getContract(View<Address> contractAddress);

  const BaseContract& getContract(View<Address> contractAddress) const;

  const auto& getEvents() const { return events_; }

  const auto& getNewContracts() const { return newContracts_; }

  void addContract(View<Address> address, std::unique_ptr<BaseContract> contract);

  void notifyNewContract(View<Address> address, BaseContract* contract);

  void transferBalance(View<Address> fromAddress, View<Address> toAddress, const uint256_t& amount);

  void store(View<Address> addr, View<Hash> slot, View<Hash> data);

  Hash retrieve(View<Address> addr, View<Hash> slot) const;

  void commit();

  void revert();

  Checkpoint checkpoint();

private:
  Account& getMutableAccount(View<Address> accountAddress);

  Accounts& accounts_;
  Storage& storage_;
  Contracts& contracts_;
  int64_t blockGasLimit_;
  int64_t blockNumber_;
  int64_t blockTimestamp_;
  int64_t txIndex_;
  Address blockCoinbase_;
  Address txOrigin_;
  Hash blockHash_;
  Hash txHash_;
  uint256_t chainId_;
  uint256_t txGasPrice_;
  size_t eventIndex_ = 0;
  std::vector<Event> events_;
  std::vector<std::pair<Address, BaseContract*>> newContracts_;
  std::stack<transactional::AnyTransactional> transactions_;
};

class ExecutionContext::AccountPointer {
public:
  AccountPointer(Account& account, std::stack<transactional::AnyTransactional>& transactions);

  const uint256_t& getBalance() const;

  uint64_t getNonce() const;

  View<Hash> getCodeHash() const;

  View<Bytes> getCode() const;

  ContractType getContractType() const;

  void setBalance(const uint256_t& amount);

  void setNonce(uint64_t nonce);

  void setCode(Bytes code);

  void setContractType(ContractType type);

private:
  Account& account_;
  std::stack<transactional::AnyTransactional>& transactions_;
};

class ExecutionContext::Checkpoint {
public:
  explicit Checkpoint(std::stack<transactional::AnyTransactional>& transactions);

  Checkpoint(const Checkpoint&) = delete;
  Checkpoint(Checkpoint&&) noexcept = delete;
  Checkpoint& operator=(const Checkpoint&) = delete;
  Checkpoint& operator=(Checkpoint&&) noexcept = delete;

  ~Checkpoint() { revert(); }

  void commit();

  void revert();

private:
  std::stack<transactional::AnyTransactional>* transactions_;
  size_t checkpoint_;
};

class ExecutionContext::Builder {
public:

  Builder() {}

  Builder& storage(ExecutionContext::Storage& storage) { storage_ = &storage; return *this; }

  Builder& accounts(ExecutionContext::Accounts& accounts) { accounts_ = &accounts; return *this; }

  Builder& contracts(ExecutionContext::Contracts& contracts) { contracts_ = &contracts; return *this; }

  Builder& blockHash(View<Hash> blockHash) { blockHash_ = Hash(blockHash); return *this; }

  Builder& txHash(View<Hash> txHash) { txHash_ = Hash(txHash); return *this; }

  Builder& txOrigin(View<Address> txOrigin) { txOrigin_ = Address(txOrigin); return *this; }

  Builder& blockCoinbase(View<Address> blockCoinbase) { blockCoinbase_ = Address(blockCoinbase); return *this; }

  Builder& txIndex(int64_t txIndex) { txIndex_ = txIndex; return *this; }

  Builder& blockNumber(int64_t blockNumber) { blockNumber_ = blockNumber; return *this; }

  Builder& blockTimestamp(int64_t blockTimestamp) { blockTimestamp_ = blockTimestamp; return *this; }

  Builder& blockGasLimit(int64_t blockGasLimit) { blockGasLimit_ = blockGasLimit; return *this; }

  Builder& txGasPrice(const uint256_t& txGasPrice) { txGasPrice_ = txGasPrice; return *this; }

  Builder& chainId(const uint256_t& chainId) { chainId_ = chainId; return *this; }

  ExecutionContext build() {
    return ExecutionContext(
      *accounts_, *storage_, *contracts_, blockGasLimit_, blockNumber_, blockTimestamp_, txIndex_,
      blockCoinbase_, txOrigin_, blockHash_, txHash_, chainId_, txGasPrice_);
  }

  std::unique_ptr<ExecutionContext> buildPtr() {
    return std::make_unique<ExecutionContext>(
      *accounts_, *storage_, *contracts_, blockGasLimit_, blockNumber_, blockTimestamp_, txIndex_,
      blockCoinbase_, txOrigin_, blockHash_, txHash_, chainId_, txGasPrice_);
  }

private:
  ExecutionContext::Accounts* accounts_;
  ExecutionContext::Storage* storage_;
  ExecutionContext::Contracts* contracts_;
  int64_t blockGasLimit_;
  int64_t blockNumber_;
  int64_t blockTimestamp_;
  int64_t txIndex_;
  Address blockCoinbase_;
  Address txOrigin_;
  Hash blockHash_;
  Hash txHash_;
  uint256_t chainId_;
  uint256_t txGasPrice_;
};

#endif // BDK_EXECUTIONCONTEXT_H
