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
#include "../utils/eventsdb.h"

#include "../contract/calltracer.h"
#include "../contract/event.h"

class Blockchain;

/**
 * Interface to the DB instances of a BDK node.
 */
class Storage : public Log::LogicalLocationProvider {

  /*
  * NOTE: Each Blockchain instance can have one or more file-backed DBs, which
  * will all be managed by storage_ (Storage class) to implement internal
  * features as needed. However, these should not store:
  * (i) any data that the consensus engine (cometbft) already stores and
  * that we have no reason to duplicate, such as blocks (OK to cache in RAM),
  * (ii) State (consistent contract checkpoints/snapshots at some execution
  * height) as those should be serialized/deserialized from/to their own
  * file-backed data structures (we are using the dump-to-fresh-speedb system)
  * (iii) the list of contract types/templates that exist, since that pertains
  * to the binary itself, and should be built statically in RAM on startup (const)
  * (it is OK-ish to store in Storage the range of block heights for which a
  * template is or isn't available, while keeping in mind that these are really
  * consensus parameters, that is, changing these means the protocol is forked).
  * What we are going to store in the node db:
  * - Activation block height for contract templates (if absent, the contract
  * template is not yet enabled) -- this should be directly modifiable by node
  * operators when enabling new contracts (these are soft forks).
  * - Events (we will keep these in the BDK to retain control and speed it up)
  * - Any State metadata we may need (e.g. name of latest state snapshot file
  * that was saved, and the last executed, locally seen, final block height).
  */

  private:
    Blockchain& blockchain_; ///< Our parent object through which we reach the other components
    DB blocksDb_; ///< DB for general-purpose usage (doesn't actually hold blocks)
    EventsDB eventsDb_; ///< DB exclusive to events

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
     * Get the indexing mode of the storage.
     * @returns The indexing mode of the storage.
     */
    IndexingMode getIndexingMode() const;

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
     * Helper function for checking if an event has certain topics.
     * @param event The event to check.
     * @param topics A list of topics to check for.
     * @return `true` if all topics match (or if no topics were provided), `false` otherwise.
     */
    static bool topicsMatch(const Event& event, const std::vector<Hash>& topics);

    /**
     * Store validator updates.
     * By saving all validator updates, we can reconstruct the full history of validator sets if required.
     * This full validator update history is not saved in snapshots.
     * Validator updates are sequences of 41 bytes, where the first 33 bytes are the
     * validator's PubKey, and the following 8 bytes are the int64_t vote count.
     * @param height Block height at which the validator updates have been requested.
     * @param validatorUpdates Pre-encoded validator updates to store.
     */
    void putValidatorUpdates(uint64_t height, Bytes validatorUpdates);


    EventsDB& events() { return eventsDb_; }

    const EventsDB& events() const { return eventsDb_; }
};

#endif  // STORAGE_H
