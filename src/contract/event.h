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

using json = nlohmann::ordered_json;

// TODO: generate the ABI for events
// TODO: add tests when done
// TODO: update docs when done

/// Abstraction of a Solidity event.
class Event {
  private:
    std::string name_;            ///< Event name.
    uint64_t logIndex_;           ///< Position of the event inside the block it was emitted from.
    Hash txHash_;                 ///< Hash of the transaction that emitted the event.
    uint64_t txIndex_;            ///< Position of the transaction inside the block the event was emitted from.
    Hash blockHash_;              ///< Hash of the block that emitted the event.
    uint64_t blockIndex_;         ///< Height of the block that the event was emitted from.
    Address address_;             ///< Address that emitted the event.
    Bytes data_;                  ///< Non-indexed arguments of the event.
    std::vector<Bytes> topics_;   ///< Indexed arguments of the event, limited to a max of 3 (4 for anonymous events).
    bool anonymous_;              ///< Whether the event is anonymous or not (its signature is indexed and searchable).

    /**
     * Encode an indexed parameter for topic storage, as specified here:
     * https://docs.soliditylang.org/en/develop/abi-spec.html#events
     * https://docs.soliditylang.org/en/develop/abi-spec.html#indexed-event-encoding
     * @param param The parameter to encode.
     * @param type The type of the parameter.
     * @return The topic-encoded parameter.
     */
    Bytes encodeTopicParam(BaseTypes& param, ABI::Types type);

  public:
    /**
     * Constructor. Only sets data partially, setStateData() should be called
     * after creating a new Event so the rest of the data can be set.
     * @param name The event's name.
     * @param address The address that emitted the event.
     * @param params The event's arguments. Defaults to none (empty list).
     * @param anonymous Whether the event is anonymous or not. Defaults to false.
     */
    Event(
      const std::string& name, Address address,
      std::vector<std::pair<BaseTypes, bool>> params = {}, bool anonymous = false
    );

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
    std::string getName() const { return this->name_; }

    /// Getter for `logIndex_`.
    uint64_t getLogIndex() const { return this->logIndex_; }

    /// Getter for `txHash_`.
    Hash getTxHash() const { return this->txHash_; }

    /// Getter for `txIndex_`.
    uint64_t getTxIndex() const { return this->txIndex_; }

    /// Getter for `blockHash_`.
    Hash getBlockHash() const { return this->blockHash_; }

    /// Getter for `blockIndex_`.
    uint64_t getBlockIndex() const { return this->blockIndex_; }

    /// Getter for `address_`.
    Address getAddress() const { return this->address_; }

    /// Getter for `data_`.
    Bytes getData() const { return this->data_; }

    /// Getter for `topics_`.
    std::vector<Bytes> getTopics() const { return this->topics_; }

    /// Getter for `anonymous_`.
    bool isAnonymous() const { return this->anonymous_; }

    /**
     * Get the event's selector (keccak hash of its signature).
     * @return topics[0] when non-anonymous, empty bytes when anonymous.
     */
    Bytes getSelector() const {
      if (!this->anonymous_) return this->topics_[0]; else return Bytes();
    }

    /// Serialize event data to a JSON string.
    std::string serialize();
};

/**
 * Class that holds all events emitted by contracts in the blockchain.
 * Responsible for registering, managing and saving/loading events to/from the database.
 */
class EventManager {
  private:
    // TODO: keep up to 1000 events in memory, dump older ones to DB (maybe this should be a deque?)
    std::vector<Event> events_;             ///< List of all emitted events in memory.
    std::vector<Event> tempEvents_;         ///< List of temporary events waiting to be commited or reverted.
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
     * Parameters are defined when calling "eth_getLogs" on an HTTP request
     * (directly from the http/jsonrpc submodules, through handle_request() on httpparser).
     * They're supposed to be all "optional" at that point, but here they're
     * all required, even if all of them turn out to be empty.
     * @param fromBlock The initial block height to look for.
     * @param toBlock The final block height to look for.
     * @param address The address to look for. If empty, will look for all available addresses.
     * @param topics The topics to filter by. If empty, will look for all available topics.
     * @return A list of matching events, limited by the block and/or log caps set above.
     */
    std::vector<Event> getEvents(
      uint64_t fromBlock, uint64_t toBlock, Address address, std::vector<Bytes> topics
    );

    /**
     * Register the event in the temporary list.
     * Keep in mind the original Event object is MOVED to the list.
     * @param event The event to register.
     */
    void registerEvent(Event& event) { this->tempEvents_.push_back(std::move(event)); }

    /**
     * Actually register events in the permanent list.
     * @param tx The transaction that emitted the events.
     */
    void commitEvents(const Hash txHash, const uint64_t txIndex) {
      uint64_t logIndex = 0;
      for (Event& e : this->tempEvents_) {
        e.setStateData(logIndex, txHash, txIndex,
          ContractGlobals::getBlockHash(), ContractGlobals::getBlockHeight()
        );
        this->events_.push_back(std::move(e));
        logIndex++;
      }
      this->tempEvents_.clear();
    }

    /// Discard events in the temporary list.
    void revertEvents() { this->tempEvents_.clear(); }
};

#endif  // EVENT_H
