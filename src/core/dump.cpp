/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dump.h"

DumpManager::DumpManager(const Storage& storage, const Options& options, std::shared_mutex& stateMutex)
  : storage_(storage),
    options_(options),
    stateMutex_(stateMutex)
{
}

void DumpManager::pushBack(Dumpable* dumpable)
{
  // Check if latest Dumpable* is the same as the one we trying to append
  if (this->dumpables_.back() == dumpable) {
    return;
  }
  dumpables_.push_back(dumpable);
}

void DumpManager::dumpAll()
{
  std::vector<DBBatch> batches;
  {
    // state mutex lock
    std::unique_lock lock(stateMutex_);
    // Logs
    Logger::logToDebug(LogType::INFO, Log::dump, __func__, "DumpAll Called!");
    // call dump functions and put the operations ate the database
    for (const auto dumpable: dumpables_) {
      batches.emplace_back(dumpable->dump());
    }
  }
  // Write to the database
  std::string dbName = options_.getRootPath() + "/stateDb/" + std::to_string(this->storage_.latest()->getNHeight());
  DB stateDb(dbName);
  for (const auto& batch: batches) {
    stateDb.putBatch(batch);
  }
}

DumpWorker::DumpWorker(const Options& options, const Storage& storage,
                       DumpManager& dumpManager)
  : options_(options),
    storage_(storage),
    dumpManager_(dumpManager)
{
  Logger::logToDebug(LogType::INFO, Log::dump, __func__, "DumpWorker Started.");
}

DumpWorker::~DumpWorker()
{
  Logger::logToDebug(LogType::INFO, Log::dump, __func__, "DumpWorker Stopped.");
}

bool DumpWorker::workerLoop()
{
  uint64_t latestBlock = 0;
  while (!this->stopWorker_) {
    if (latestBlock + 100 < this->storage_.currentChainSize()) {
      Logger::logToDebug(LogType::INFO,
                         Log::dump,
                         __func__,
                         "Current size >= 100");
      latestBlock = this->storage_.currentChainSize();
      std::cout << "Dumping all" << std::endl;
      dumpManager_.dumpAll();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return true;
}

void DumpWorker::startWorker()
{
  if (!this->workerFuture_.valid()) {
    this->workerFuture_ = std::async(std::launch::async,
                                     &DumpWorker::workerLoop,
                                     this);
  }
}

void DumpWorker::stopWorker()
{
  if (this->workerFuture_.valid()) {
    this->stopWorker_ = true;
    this->workerFuture_.wait();
    this->workerFuture_.get();
  }
}
