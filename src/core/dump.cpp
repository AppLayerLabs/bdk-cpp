/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dump.h"

DumpManager::DumpManager(DB& dbState, std::shared_mutex& stateMutex)
  : dbState_(dbState),
    stateMutex_(stateMutex)
{
}

void DumpManager::dumpAll()
{
  // state mutex lock
  std::unique_lock lock(stateMutex_);
  // Logs
  Logger::logToDebug(LogType::INFO, Log::dump, __func__, "DumpAll Called!");
  // call dump functions and put the operations ate the database
  for (const auto dumpable: dumpable_)
    this->dbState_.putBatch(dumpable.get().dump());
}

DumpWorker::DumpWorker(const Storage& storage,
                       DumpManager& dumpManager)
  : storage_(storage),
    dumpManager_(dumpManager)
{
  this->start();
  Logger::logToDebug(LogType::INFO, Log::dump, __func__, "DumpWorker Started.");
}

DumpWorker::~DumpWorker()
{
  this->stop();
  Logger::logToDebug(LogType::INFO, Log::dump, __func__, "DumpWorker Stopped.");
}

bool DumpWorker::workerLoop()
{
  uint64_t i = 1;
  while (!this->stopWorker_) {
    if ((100 * i) - (storage_.currentChainSize()) >= 0) {
      Logger::logToDebug(LogType::INFO,
                         Log::dump,
                         __func__,
                         "Current size >= 100");
      ++i;
      dumpManager_.dumpAll();
    }
  }
  return true;
}

void DumpWorker::start()
{
  if (!this->workerFuture_.valid()) {
    this->workerFuture_ = std::async(std::launch::async,
                                     &DumpWorker::workerLoop,
                                     this);
  }
}

void DumpWorker::stop()
{
  if (this->workerFuture_.valid()) {
    this->stopWorker_ = true;
    this->workerFuture_.wait();
    this->workerFuture_.get();
  }
}
