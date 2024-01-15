#ifndef EVENT_H
#define EVENT_H

#include <algorithm>
#include <shared_mutex>
#include <string>

#include "../libs/json.hpp"

#include "../utils/db.h"
#include "../utils/strings.h"
#include "../utils/utils.h"
#include "abi.h"
#include "contract.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace bmi = boost::multi_index;

using json = nlohmann::ordered_json;

/// Abstraction of a Solidity event.
class Event {
  friend struct event_indices;
  private:
    std::string name_;          ///< Event name.
    uint64_t logIndex_;         ///< Position of the event inside the block it was emitted from.
    Hash txHash_;               ///< Hash of the transaction that emitted the event.
    uint64_t txIndex_;          ///< Position of the transaction inside the block the event was emitted from.
    Hash blockHash_;            ///< Hash of the block that emitted the event.
    uint64_t blockIndex_;       ///< Height of the block that the event was emitted from.
    Address address_;           ///< Address that emitted the event.
    Bytes data_;                ///< Non-indexed arguments of the event.
    std::vector<Hash> topics_;  ///< Indexed arguments of the event, limited to a max of 3 (4 for anonymous events). Topics are Hashes since they are all 32 bytes.
    bool anonymous_;            ///< Whether the event is anonymous or not (its signature is indexed and searchable).

  public:
    /**
     * Constructor. Only sets data partially, setStateData() should be called
     * after creating a new Event so the rest of the data can be set.
     * @tparam Args The types of the event's arguments.
     * @param name The event's name.
     * @param address The address that emitted the event.
     * @param params The event's arguments. (a tuple of N std::pair<T, bool> where T is the type and bool is whether it's indexed or not)
     * @param anonymous Whether the event is anonymous or not. Defaults to false.
     */
    template <typename... Args, bool... Flags> Event(
      const std::string& name, Address address,
      const std::tuple<EventParam<Args, Flags>...>& params,
      bool anonymous = false
    ) : name_(name), address_(address), anonymous_(anonymous) {
      // Get the event's signature
      auto eventSignature = ABI::EventEncoder::encodeSignature<Args...>(name);
      std::vector<Hash> topics;

      // Process indexed parameters.
      // For each EventParam in the tuple where Flag is true, encode it with
      // ABI::EventEncoder::encodeTopicSignature<T>, where T is the type of the
      // parameter, and append it to the topics vector.
      // For each EventParam in the tuple where Flag is false, encode it with
      // ABI::Encoder::encodeData<T...>, where T... is all the types of the
      // params tuple that are false, which should result in a single Bytes
      // object that should be appended to the data vector.
      // We use std::apply for indexed parameters because we need to iterate over the tuple.
      Bytes encodedNonIndexed;
      std::apply([&](const auto&... param) {
        (..., (param.isIndexed ? topics.push_back(ABI::EventEncoder::encodeTopicSignature(param.value)) : void()));
      }, params);

      // For non-indexed parameters, we need to encode them all together.
      // encodeEventData differs from ABI::Encoder::EncoderData where it skips indexed parameters.
      this->data_ = ABI::EventEncoder::encodeEventData(params);
      if (!anonymous) this->topics_.push_back(eventSignature);
      for (const auto& topic : topics) {
        if (this->topics_.size() >= 4) {
          Logger::logToDebug(LogType::WARNING, Log::event, __func__,
            "Attention! Event " + name + " has more than 3 indexed parameters. Only the first 3 will be indexed."
          );
          break;
        }
        this->topics_.push_back(topic);
      }
    }

    /**
     * Constructor from deserialization.
     * @param jsonstr The JSON string to deserialize.
     */
    Event(const std::string& jsonstr);

    /**
     * Set data from the block and transaction that is supposed to emit the event.
     * @param logIndex The event's position on the block.
     * @param txHash The hash of the transaction that emitted the event.
     * @param txIndex The position of the transaction in the block.
     * @param blockHash The hash of the block that emitted the event.
     * @param blockIndex The height of the block.
     */
    void setStateData(
      uint64_t logIndex, Hash txHash, uint64_t txIndex,
      Hash blockHash, uint64_t blockIndex
    ) {
      this->logIndex_ = logIndex;
      this->txHash_ = txHash;
      this->txIndex_ = txIndex;
      this->blockHash_ = blockHash;
      this->blockIndex_ = blockIndex;
    }

    /// Getter for `name_`.
    const std::string& getName() const { return this->name_; }

    /// Getter for `logIndex_`.
    const uint64_t& getLogIndex() const { return this->logIndex_; }

    /// Getter for `txHash_`.
    const Hash& getTxHash() const { return this->txHash_; }

    /// Getter for `txIndex_`.
    const uint64_t& getTxIndex() const { return this->txIndex_; }

    /// Getter for `blockHash_`.
    const Hash& getBlockHash() const { return this->blockHash_; }

    /// Getter for `blockIndex_`.
    const uint64_t& getBlockIndex() const { return this->blockIndex_; }

    /// Getter for `address_`.
    const Address& getAddress() const { return this->address_; }

    /// Getter for `data_`.
    const Bytes& getData() const { return this->data_; }

    /// Getter for `topics_`.
    const std::vector<Hash>& getTopics() const { return this->topics_; }

    /// Getter for `anonymous_`.
    bool isAnonymous() const { return this->anonymous_; }

    /**
     * Get the event's selector (keccak hash of its signature).
     * @return topics[0] when non-anonymous, empty bytes when anonymous.
     */
    Hash getSelector() const {
      if (!this->anonymous_) return this->topics_[0]; else return Hash();
    }

    /// Serialize event data from the object to a JSON string.
    std::string serialize() const;

    /**
     * Serialize event data to a JSON string, formatted to RPC response standards:
     * https://medium.com/alchemy-api/deep-dive-into-eth-getlogs-5faf6a66fd81
     */
    std::string serializeForRPC();
};

/// Multi-index container used for storing events in memory.
struct event_indices : bmi::indexed_by<
  // Ordered index by blockIndex for range queries
  bmi::ordered_non_unique<bmi::member<Event, uint64_t, &Event::blockIndex_>>,
  // Ordered index by address (optional)
  bmi::ordered_non_unique<bmi::member<Event, Address, &Event::address_>>,
  // Ordered index by txHash for direct access
  bmi::ordered_non_unique<bmi::member<Event, Hash, &Event::txHash_>>
> {};

/// Typedef for the event multi-index container.
typedef bmi::multi_index_container<Event, event_indices> EventContainer;

/**
 * Class that holds all events emitted by contracts in the blockchain.
 * Responsible for registering, managing and saving/loading events to/from the database.
 */
class EventManager {
  private:
    // TODO: keep up to 1000 (maybe 10000? 100000? 1M seems too much) events in memory, dump older ones to DB (this includes checking save/load - maybe this should be a deque?)
    EventContainer events_;                 ///< List of all emitted events in memory. Older ones FIRST, newer ones LAST.
    EventContainer tempEvents_;             ///< List of temporary events waiting to be commited or reverted.
    const std::unique_ptr<DB>& db_;         ///< Reference pointer to the database.
    mutable std::shared_mutex lock_;        ///< Mutex for managing read/write access to the permanent events vector.
    const unsigned short blockCap_ = 2000;  ///< Maximum block range allowed for querying events (safety net).
    const unsigned short logCap_ = 10000;   ///< Maximum number of consecutive matches allowed for querying events (safety net).

  public:
    /**
     * Constructor; Automatically loads events from the database.
     * @param db The database to use.
     */
    EventManager(const std::unique_ptr<DB>& db);

    /// Destructor. Automatically saves events to the database.
    ~EventManager();

    // TODO: maybe a periodicSaveToDB() just like on Storage?

    /**
     * Get all the events emitted under the given inputs.
     * Used by "eth_getLogs", from where parameters are defined on an HTTP request
     * (directly from the http/jsonrpc submodules, through handle_request() on httpparser).
     * @param fromBlock The initial block height to look for.
     * @param toBlock The final block height to look for.
     * @param address The address to look for. If empty, will look for all available addresses.
     * @param topics The topics to filter by. If empty, will look for all available topics.
     * @return A list of matching events, limited by the block and/or log caps set above.
     */
    std::vector<Event> getEvents(
      const uint64_t& fromBlock, const uint64_t& toBlock,
      const std::optional<Address>& address, const std::optional<std::vector<Hash>>& topics
    );

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
    );

    /**
     * Filter events in memory. Used by getEvents().
     * @param fromBlock The starting block range to query.
     * @param toBlock Tne ending block range to query.
     * @param address The address to look for. If empty, will look for all available addresses.
     * @return A list of found events.
     */
    std::vector<Event> filterFromMemory(
      const uint64_t& fromBlock, const uint64_t& toBlock, const std::optional<Address>& address
    );

    /**
     * Query and filter events from the database. Used by getEvents().
     * @param fromBlock The starting block range to query.
     * @param toBlock Tne ending block range to query.
     * @param address The address to look for. If empty, will look for all available addresses.
     * @param topics The topics to filter by. If empty, will look for all available topics.
     * @param ret The list to use as return.
     */
    void fetchAndFilterFromDB(
      const uint64_t& fromBlock, const uint64_t& toBlock,
      const std::optional<Address>& address, const std::optional<std::vector<Hash>>& topics,
      std::vector<Event>& ret
    );

    /**
     * Check if all topics of an event match the desired topics from a query.
     * @param event The event to match.
     * @param topics The queried topics to match for.
     * @return `true` if all topics match, `false` otherwise.
     */
    bool matchTopics(const Event& event, const std::optional<std::vector<Hash>>& topics);

    /**
     * Register the event in the temporary list.
     * Keep in mind the original Event object is MOVED to the list.
     * @param event The event to register.
     */
    void registerEvent(Event&& event) {this->tempEvents_.insert(std::move(event));}

    /**
     * Actually register events in the permanent list.
     * @param txHash The hash of the transaction that emitted the events.
     * @param txIndex The index of the transaction inside the block that emitted the events.
     */
    void commitEvents(const Hash txHash, const uint64_t txIndex) {
      uint64_t logIndex = 0;
      auto it = tempEvents_.begin(); // Use iterators to loop through the MultiIndex container
      while (it != tempEvents_.end()) {
        // Since we can't modify the element directly because iterators are const...
        Event e = *it;                // ...we make a copy...
        e.setStateData(logIndex, txHash, txIndex,
          ContractGlobals::getBlockHash(), ContractGlobals::getBlockHeight()
        );                            // ...modify it...
        events_.insert(std::move(e)); // ...move it to the permanent container...
        it = tempEvents_.erase(it);   // ...erase the original from the temp...
        logIndex++;                   // ...and move on to the next
      }
    }

    /// Discard events in the temporary list.
    void revertEvents() { this->tempEvents_.clear(); }
};

#endif  // EVENT_H
