/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <iostream>
#include <filesystem>

#include "src/core/blockchain.h"

std::unique_ptr<Blockchain> blockchain = nullptr;

void signalHandler(int signum) {
  Logger::logToDebug(LogType::INFO, "MAIN", "MAIN", "Received signal " + std::to_string(signum) + ". Stopping the blockchain.");
  blockchain->stop();
  blockchain = nullptr; // Destroy the blockchain object, calling the destructor of every module and dumping to DB.
  Utils::safePrint("Exiting...");
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  exit(signum);
}

int main() {
  Utils::logToCout = true;
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/blockchain");
  blockchain = std::make_unique<Blockchain>(blockchainPath);
  // Start the blockchain syncing engine.
  std::signal(SIGINT, signalHandler);
  std::signal(SIGHUP, signalHandler);
  blockchain->start();
  std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::hours(std::numeric_limits<int>::max()));
  return 0;
}

