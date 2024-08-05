/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DUMP_H
#define DUMP_H

#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <functional>
#include <shared_mutex>

#include "storage.h"
#include "../utils/db.h"

// Forward declaration
class EventManager;

/// Abstraction of a dumpable object (an object that can be dumped to the database).
class Dumpable {
  public:
    /**
     * Pure virtual function to be implemented.
     * The function should dump implemented by the methods that are dumpable.
     */
    virtual DBBatch dump() const = 0;
};

/// Class that manages dumping to the database. Used to store dumpable objects in memory.
class DumpManager : public Log::LogicalLocationProvider {
  private:
    const Options& options_; ///< Reference to the options object.
    const Storage& storage_; ///< Reference to the storage object
    std::shared_mutex& stateMutex_; ///< Mutex for managing read/write access to the state object.
    std::vector<Dumpable*> dumpables_; /// List of Dumpable objects.
    EventManager& eventManager_; /// Reference to the EventManager object.

    /**
     * Auxiliary function used by async calls that processes a little slice of dumps in a separate thread.
     * @param threadOffset Offset for the dumpables list.
     * @param threadItems How many items to dump from the dumpables list.
     * @return A list of DBBatch dump operations.
     */
    std::vector<DBBatch> dumpToBatch(unsigned int threadOffset, unsigned int threadItems) const;

  public:
    /**
     * Constructor.
     * @param storage Reference to the Storage object.
     * @param options Reference to the Options singleton.
     * @param eventManager Reference to the EventManager object.
     * @param stateMutex Reference to the state mutex.
     */
    DumpManager(const Storage& storage, const Options& options, EventManager& eventManager, std::shared_mutex& stateMutex);

    /// Log instance from Storage.
    std::string getLogicalLocation() const override { return storage_.getLogicalLocation(); }

    /**
     * Register a Dumpable object into the list.
     * @param dumpable Pointer to the Dumpable object to be registered.
     */
    void pushBack(Dumpable* dumpable);

    /**
     * Call dump functions contained in the dumpable list.
     * @returns A vector of DBBatch objects and the nHeight of the last block.
     */
    std::pair<std::vector<DBBatch>, uint64_t> dumpState() const;

    /// Dump the state to DB.
    void dumpToDB() const;

    /// Get the size of the dupables list.
    size_t size() const { return this->dumpables_.size(); }

    /**
     * Get the best state DB patch.
     * @param options the options object
     * @return a pair of the best state DB patch and the nHeight of the last block.
     */
    static std::pair<std::string, uint64_t> getBestStateDBPath(const Options& options) {
      std::filesystem::path stateDbRootFolder = options.getRootPath() + "/stateDb/";
      // Each state DB patch is named with the block height.
      // Therefore, we need to list all the directories in the stateDbRootFolder and return 
      // the one with the highest block height using std::filesystem::directory_iterator.
      uint64_t bestHeight = 0;
      std::string bestPath;
      if (!std::filesystem::exists(stateDbRootFolder)) {
        // If the state DB folder does not exist, return stateDbRootFolder + "0"
        return std::make_pair(stateDbRootFolder.string() + "0", 0);
      }

      for (const auto& entry : std::filesystem::directory_iterator(stateDbRootFolder)) {
        std::string path = entry.path().string();
        // Get the block height from the path
        uint64_t height = std::stoull(path.substr(path.find_last_of('/') + 1));
        if (height > bestHeight) {
          bestHeight = height;
          bestPath = path;
        }
      }
      if (bestPath.empty()) {
        // If there are no state DB patches, return stateDbRootFolder + "0"
        return std::make_pair(stateDbRootFolder.string() + "0", 0);
      }
      return std::make_pair(bestPath, bestHeight);
    }
};

class DumpWorker : public Log::LogicalLocationProvider {
  private:
    const Options& options_; ///< Reference to the Options singleton.
    const Storage& storage_; ///< Reference to the Storage object.
    DumpManager& dumpManager_; /// Reference to the DumpManager object.
    std::atomic<bool> stopWorker_ = false; ///< Flag for stopping the worker thread.
    std::future<bool> workerFuture_; ///< Future object for the worker thread, used to wait for the thread to finish.
    std::atomic<bool> canDump_ = false; ///< Flag for knowing if the worker is ready to dump.

    /**
     * Entry function for the worker thread (runs the workerLoop() function).
     * @return `true` when done running.
     */
    bool workerLoop();

  public:
    /**
     * Constructor. Automatically starts the worker thread.
     * @param options Reference to the Options singleton.
     * @param storage Reference to the Storage object.
     * @param dumpManager Reference to the DumpManager object.
     */
    DumpWorker(const Options& options, const Storage& storage, DumpManager& dumpManager);

    /// Destructor. Automatically stops the worker thread if it's still running.
    ~DumpWorker();

    /// Log instance from Storage.
    std::string getLogicalLocation() const override { return storage_.getLogicalLocation(); }

    void startWorker(); ///< Start `workerFuture_` and `workerLoop()`.
    void stopWorker(); ///< Stop `workerFuture_` and `workerLoop()`.
};

#endif // DUMP_H
