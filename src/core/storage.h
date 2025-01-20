/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef STORAGE_H
#define STORAGE_H

#include "../utils/db.h"
#include "../utils/randomgen.h" // utils.h
#include "../utils/safehash.h" // tx.h -> ecdsa.h -> utils.h -> bytes/join.h, strings.h -> libs/zpp_bits.h
#include "../utils/options.h"

#include "../contract/calltracer.h"
#include "../contract/event.h"

class Blockchain;

/**
 * Interface to the DB instances of a BDK node.
 */
class Storage : public Log::LogicalLocationProvider {
  private:
    Blockchain& blockchain_; ///< Our parent object through which we reach the other components
    DB blocksDb_;  ///< DB for general-purpose usage (doesn't actually hold blocks; TODO: rename)
    DB eventsDb_; ///< DB for events

  public:
    /**
     * Constructor. Automatically loads the chain from the database
     * and starts the periodic save thread.
     * @param instanceIdStr Instance ID string to use for logging.
     * @param options Reference to the options singleton.
     */
    Storage(Blockchain& blockchain); //, std::string instanceIdStr, const Options& options);

    ~Storage() = default; ///< Destructor.

    std::string getLogicalLocation() const override; ///< Log helper.

    /**
     * Stores additional transaction data
     * @param txData The additional transaction data
     */
    void putTxAdditionalData(const TxAdditionalData& txData);

    /**
     * Retrieve the stored additional transaction data.
     * @param txHash The target transaction hash.
     * @return The transaction data if existent, or an empty optional otherwise.
     */
    std::optional<TxAdditionalData> getTxAdditionalData(const Hash& txHash) const;

    /**
     * Store a transaction call trace.
     * @param txHash The transaction hash.
     * @param callTrace The call trace of the transaction.
     */
    void putCallTrace(const Hash& txHash, const trace::Call& callTrace);

    /**
     * Retrieve the stored call trace of the target transaction.
     * @param txHash The target transaction hash.
     * @return The transation call trace if existent, or an empty optional otherwise.
     */
    std::optional<trace::Call> getCallTrace(const Hash& txHash) const;

    /**
     * Store an event.
     * @param event The event to be stored.
     */
    void putEvent(const Event& event);

    /**
     * Retrieve all events from a given range of block numbers.
     * @param fromBlock The initial block number (included).
     * @param toBlock The last block number (included).
     * @param address The address that emitted the events.
     * @param topics The event's topics.
     * @return The requested events if existent, or an empty vector otherwise.
     */
    std::vector<Event> getEvents(uint64_t fromBlock, uint64_t toBlock, const Address& address, const std::vector<Hash>& topics) const;

    /**
     * Retrieve all events from given block index and transaction index.
     * @param blockIndex The number of the block that contains the transaction.
     * @param txIndex The transaction index within the block.
     * @return The requested events if existent, or an empty vector otherwise.
     */
    std::vector<Event> getEvents(uint64_t blockIndex, uint64_t txIndex) const;

    /**
     * Get the indexing mode of the storage.
     * @returns The indexing mode of the storage.
     */
    IndexingMode getIndexingMode() const;
};

#endif  // STORAGE_H
