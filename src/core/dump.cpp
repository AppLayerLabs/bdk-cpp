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
  if (this->dumpables_.size() > 0 &&
      this->dumpables_.back() == dumpable) {
    return;
  }
  dumpables_.push_back(dumpable);
}

std::pair<std::vector<DBBatch>, uint64_t> DumpManager::dumpState() const {
  std::pair<std::vector<DBBatch>,uint64_t> ret;
  auto& [batches, blockHeight] = ret;
  {
    // state mutex lock
    std::unique_lock lock(stateMutex_);
    // We can only safely get the nHeight that we are dumping after **uniquely locking** the state (making sure that no new blocks
    // or state changes are happening)
    blockHeight = storage_.latest()->getNHeight();
    // Emplace DBBatch operations
    Logger::logToDebug(LogType::INFO,
                       Log::dump,
                       __func__,
                       "Emplace DBBatch operations");
    for (const auto dumpable: dumpables_) {
      // call dump functions and put the operations ate the database
      batches.emplace_back(dumpable->dump());
    }
  }
  return ret;
}

void DumpManager::dumpToDB() const {
  auto toDump = this->dumpState();
  const auto& [batches, blockHeight] = toDump;
  std::string dbName = options_.getRootPath() + "/stateDb/" + std::to_string(blockHeight);
  DB stateDb(dbName);
  for (const auto& batch : batches) {
    stateDb.putBatch(batch);
  }
}

DumpWorker::DumpWorker(const Storage& storage,
                       DumpManager& dumpManager)
  : storage_(storage),
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
  uint64_t latestBlock = this->storage_.currentChainSize();
  while (!this->stopWorker_) {
    if (latestBlock + 100 < this->storage_.currentChainSize()) {
      Logger::logToDebug(LogType::INFO,
                         Log::dump,
                         __func__,
                         "Current size >= 100");
      dumpManager_.dumpToDB();
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
