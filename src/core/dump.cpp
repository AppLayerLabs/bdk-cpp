/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dump.h"

DumpManager::DumpManager(
  const Storage& storage, const Options& options, std::shared_mutex& stateMutex
) : options_(options), storage_(storage), stateMutex_(stateMutex) {}

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
    // We can only safely get the nHeight that we are dumping after **uniquely locking** the state (making ASBOLUTELY sure that no new blocks
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
    Utils::safePrint("this->dumpables_.size() = " + std::to_string(this->dumpables_.size()));
    Utils::safePrint("nThreads = " + std::to_string(nThreads));
    Utils::safePrint("requiredOffset = " + std::to_string(requiredOffset));
    Utils::safePrint("remaining = " + std::to_string(remaining));

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
  }
  return ret;
}

std::tuple<uint64_t, uint64_t, uint64_t> DumpManager::dumpToDB() const {
  std::tuple<uint64_t, uint64_t, uint64_t> ret;
  auto& [dumpedBlockHeight, serializeTime, dumpTime] = ret;
  auto now = std::chrono::system_clock::now();
  Utils::safePrint("Dumping state to DB...");
  auto toDump = this->dumpState();
  serializeTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - now).count();
  Utils::safePrint("Dumping state at height " + std::to_string(toDump.second) + " took " + std::to_string(serializeTime) + "ms");
  const auto& [batches, blockHeight] = toDump;
  std::string dbName = options_.getRootPath() + "/stateDb/" + std::to_string(blockHeight);
  dumpedBlockHeight = blockHeight;
  Utils::safePrint("Dumping the new state at height " + std::to_string(blockHeight) + " to " + dbName);
  now = std::chrono::system_clock::now();
  DB stateDb(dbName, true); // Compressed
  Utils::safePrint("Total Batches to process: " + std::to_string(batches.size()));
  for (uint64_t i = 0; i < batches.size(); i++) {
    Utils::safePrint("Processing batch: " + std::to_string(i) + " of " + std::to_string(batches.size()));
    stateDb.putBatch(batches[i]);
  }
  dumpTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - now).count();
  if (stateDb.close())
    Utils::safePrint("State dumped at height " + std::to_string(blockHeight) + " took " + std::to_string(dumpTime) + "ms");

  return ret;
}

DumpWorker::DumpWorker(const Options& options, const Storage& storage, DumpManager& dumpManager)
  : options_(options), storage_(storage), dumpManager_(dumpManager)
{
  LOGXTRACE("DumpWorker Started.");
}

DumpWorker::~DumpWorker() {
  stopWorker();
  LOGXTRACE("DumpWorker Stopped.");
}

bool DumpWorker::workerLoop() {
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
