/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dump.h"
#include "../contract/event.h"

DumpManager::DumpManager(
  const Storage& storage, const Options& options, EventManager& eventManager, std::shared_mutex& stateMutex
) : options_(options), storage_(storage), stateMutex_(stateMutex), eventManager_(eventManager) {}

void DumpManager::pushBack(Dumpable* dumpable) {
  // Check if latest Dumpable* is the same as the one we trying to append
  if (!this->dumpables_.empty() && this->dumpables_.back() == dumpable) return;
  dumpables_.push_back(dumpable);
}

std::vector<DBBatch> DumpManager::dumpToBatch(unsigned int threadOffset, unsigned int threadItems) const {
  std::vector<DBBatch> ret;
  for (auto i = threadOffset; i < (threadOffset + threadItems); i++) {
    ret.emplace_back(this->dumpables_[i]->dump());
  }
  return ret;
}

std::pair<std::vector<DBBatch>, uint64_t> DumpManager::dumpState() const {
  std::pair<std::vector<DBBatch>, uint64_t> ret;
  auto& [batches, blockHeight] = ret;
  {
    // state mutex lock
    std::unique_lock lock(stateMutex_);
    // We can only safely get the nHeight that we are dumping after **uniquely locking** the state (making sure that no new blocks
    // or state changes are happening)
    blockHeight = storage_.latest()->getNHeight();
    // Emplace DBBatch operations
    LOGDEBUG("Emplace DBBatch operations");

    const auto nThreads = std::thread::hardware_concurrency();
    auto requiredOffset = this->dumpables_.size() / nThreads;
    auto remaining = (this->dumpables_.size() - (requiredOffset * nThreads));
    auto currentOffset = 0;
    std::vector<std::future<std::vector<DBBatch>>> futures(nThreads);
    std::vector<std::vector<DBBatch>> outputs(nThreads);

    for (decltype(futures)::size_type i = 0; i < nThreads; ++i) {
      auto nItems = requiredOffset;
      if (remaining != 0) {
        /// Add a extra job if the division was not perfect and we have remainings
        ++nItems;
        --remaining;
      }
      futures[i] = std::async(&DumpManager::dumpToBatch, this, currentOffset, nItems);
      currentOffset += nItems;
    }
    // get futures output (wait thread, implicit)
    for (auto i = 0; i < nThreads; ++i)
      outputs[i] = futures[i].get();

    // emplace futures return into batches
    for (auto i = 0; i < nThreads; ++i)
      for (auto j = 0; j < outputs[i].size(); ++j)
        batches.emplace_back(outputs[i][j]);

    // Also dump the events
    // EventManager has its own database.
    // We just need to make sure that its data is dumped
    // At the same block height as the state data
    this->eventManager_.dump();
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

DumpWorker::DumpWorker(const Options& options,
                       const Storage& storage,
                       DumpManager& dumpManager)
  : options_(options),
    storage_(storage),
    dumpManager_(dumpManager)
{
  LOGXTRACE("DumpWorker Started.");
}

DumpWorker::~DumpWorker()
{
  stopWorker();
  LOGXTRACE("DumpWorker Stopped.");
}

bool DumpWorker::workerLoop()
{
  uint64_t latestBlock = this->storage_.currentChainSize();
  while (!this->stopWorker_) {
    if (latestBlock + this->options_.getStateDumpTrigger() < this->storage_.currentChainSize()) {
      LOGDEBUG("Current size >= " + std::to_string(this->options_.getStateDumpTrigger()));
      dumpManager_.dumpToDB();
      latestBlock = this->storage_.currentChainSize();
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
