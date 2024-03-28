/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DUMP_H
#define DUMP_H

#include <vector>
#include <functional>
#include <shared_mutex>

#include "storage.h"
#include "../utils/db.h"

/**
 * Abstraction of the dumpable object.
 */
class Dumpable {
public:
  /**
   * Pure virtual function to be implemented.
   * The function should dump implemented by the methods that are dumpable.
   */
  virtual DBBatch dump() const = 0;
};

/**
 * Dumpable management.
 * Used to store dumpable objects in memory.
 */
class DumpManager {
private:
  /// Reference to the database that contains the state cache
  DB& dbState_;
  /// Mutex for managing read/write access to the state object
  std::shared_mutex& stateMutex_;
  /// Dumpable objects.
  std::vector<std::reference_wrapper<Dumpable>> dumpable_;
public:
  /**
   * Constructor.
   * @param db Pointer to state database.
   */
  DumpManager(DB& dbState, std::shared_mutex& stateMutex);

  /**
   * Function that will register dumpable objects.
   * @param dumpable Pointer to be registered.
   */
  void pushBack(std::reference_wrapper<Dumpable> dumpable) {
    dumpable_.push_back(dumpable);
  }

  /**
   * Call dump functions contained in
   * the dumpable_ vector.
   */
  void dumpAll();
};

class DumpWorker {
private:
  /// Reference to the DumpManager object
  DumpManager& dumpManager_;
  /// Reference to the storage object
  const Storage& storage_;
  /// Flag for stopping the worker thread.
  std::atomic<bool> stopWorker_ = false;
  /// Future object for the worker thread, used to wait the thread to finish
  std::future<bool> workerFuture_;
  /// Flag for knowing if the worker is ready to dump
  std::atomic<bool> canDump_ = false;

  /**
   * Entry function for the worker thread (runs the workerLoop() function).
   * @return `true` when done running.
   */
  bool workerLoop();
public:
  /**
   * Constructor.
   * Automatically starts the worker thread.
   */
  DumpWorker(const Storage& storage, DumpManager& dumpManager);

  /**
   * Destructor.
   * Automatically stops the worker thread if it's still running.
   */
  ~DumpWorker();

  ///< Start `workerFuture_` and `workerLoop()`.
  void start();

  ///< Stop `workerFuture_` and `workerLoop()`.
  void stop();
};

#endif // DUMP
