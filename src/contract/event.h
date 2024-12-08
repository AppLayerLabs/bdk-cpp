/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef EVENT_H
#define EVENT_H

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>

#include "abi.h" // utils.h -> (strings.h -> libs/zpp_bits.h), (libs/json.hpp -> algorithm, string)

namespace bmi = boost::multi_index;

using json = nlohmann::ordered_json;

/// Abstraction of a Solidity event.
class Event {
  friend struct event_indices;
  friend zpp::bits::access;
  using serialize = zpp::bits::members<10>; ///< Typedef for the serialization struct.

  private:
    std::string name_;          ///< Event name.
    uint64_t logIndex_;         ///< Position of the event inside the block it was emitted from.
    Hash txHash_;               ///< Hash of the transaction that emitted the event.
    uint64_t txIndex_;          ///< Position of the transaction inside the block the event was emitted from.
    Hash blockHash_;            ///< Hash of the block that emitted the event.
    uint64_t blockIndex_;       ///< Height of the block that the event was emitted from.
    Address address_;           ///< Address that emitted the event.
    Bytes data_;                ///< Non-indexed arguments of the event.
    std::vector<Hash> topics_;  ///< Indexed arguments of the event, limited to a max of 3 (4 for anonymous events). Topics are Hashes since they are always 32 bytes.
    bool anonymous_;            ///< Whether the event is anonymous or not (its signature is indexed and searchable).
    friend struct event_indices;

  public:
    Event() = default;

    /**
     * Constructor for EVM events.
     * @param name The event's name.
     * @param logIndex The event's position on the block.
     * @param txHash The hash of the transaction that emitted the event.
     * @param txIndex The position of the transaction in the block.
     * @param blockHash The hash of the block that emitted the event.
     * @param blockIndex The height of the block.
     * @param address The address that emitted the event.
     * @param data The event's arguments.
     * @param topics The event's indexed arguments.
     * @param anonymous Whether the event is anonymous or not.
     */
    Event(const std::string& name, uint64_t logIndex, const Hash& txHash, uint64_t txIndex,
          const Hash& blockHash, uint64_t blockIndex, Address address, const Bytes& data,
          const std::vector<Hash>& topics, bool anonymous) :
          name_(name), logIndex_(logIndex), txHash_(txHash), txIndex_(txIndex),
          blockHash_(blockHash), blockIndex_(blockIndex), address_(address),
          data_(data), topics_(topics), anonymous_(anonymous) {}

    /**
     * Constructor for C++ events.
     * After creating a new Event so the rest of the data can be set.
     * @tparam Args The types of the event's arguments.
     * @param name The event's name.
     * @param logIndex The log index of the event.
     * @param txHash The hash of the transaction that emitted the event.
     * @param txIndex The index of the transaction that emitted the event.
     * @param blockHash The hash of the block that emitted the event.
     * @param blockIndex The index of the block that emitted the event.
     * @param address The address that emitted the event.
     * @param params The event's arguments. (a tuple of N std::pair<T, bool> where T is the type and bool is whether it's indexed or not)
     * @param anonymous Whether the event is anonymous or not. Defaults to false.
     */
    template <typename... Args, bool... Flags> Event(
      const std::string& name, uint64_t logIndex, const Hash& txHash, uint64_t txIndex,
      const Hash& blockHash, uint64_t blockIndex, Address address,
      const std::tuple<EventParam<Args, Flags>...>& params,
      bool anonymous = false
    ) : name_(name), logIndex_(logIndex), txHash_(txHash), txIndex_(txIndex),
        blockHash_(blockHash), blockIndex_(blockIndex), address_(address), anonymous_(anonymous) {
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
      std::apply([&topics](const auto&... param) {
        (..., (param.isIndexed ? topics.push_back(ABI::EventEncoder::encodeTopicSignature(param.value)) : void()));
      }, params);

      // For non-indexed parameters, we need to encode them all together.
      // encodeEventData differs from ABI::Encoder::EncoderData where it skips indexed parameters.
      this->data_ = ABI::EventEncoder::encodeEventData(params);
      if (!anonymous) this->topics_.push_back(eventSignature);
      for (const auto& topic : topics) {
        if (this->topics_.size() >= 4) {
          LOGWARNING("Attention! Event " + name + " has more than 3 indexed parameters. Only the first 3 will be indexed.");
          break;
        }
        this->topics_.push_back(topic);
      }
    }

    Event(const Event&) = default; ///< Copy constructor.
    Event(Event&&) = default; ///< Move constructor.

    /**
     * Constructor from deserialization.
     * @param jsonstr The JSON string to deserialize.
     */
    explicit Event(const std::string& jsonstr);

    ///@{
    /** Getter. */
    const std::string& getName() const { return this->name_; }
    const uint64_t& getLogIndex() const { return this->logIndex_; }
    const Hash& getTxHash() const { return this->txHash_; }
    const uint64_t& getTxIndex() const { return this->txIndex_; }
    const Hash& getBlockHash() const { return this->blockHash_; }
    const uint64_t& getBlockIndex() const { return this->blockIndex_; }
    const Address& getAddress() const { return this->address_; }
    const Bytes& getData() const { return this->data_; }
    const std::vector<Hash>& getTopics() const { return this->topics_; }
    bool isAnonymous() const { return this->anonymous_; }
    ///@}

    /**
     * Get the event's selector (keccak hash of its signature).
     * @return topics[0] when non-anonymous, empty bytes when anonymous.
     */
    Hash getSelector() const {
      if (!this->anonymous_) return this->topics_[0]; else return Hash();
    }

    std::string serializeToJson() const;  ///< Serialize event data from the object to a JSON string.

    /**
     * Serialize event data to a JSON string, formatted to RPC response standards
     * @see https://medium.com/alchemy-api/deep-dive-into-eth-getlogs-5faf6a66fd81
     */
    json serializeForRPC() const;
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

#endif  // EVENT_H
