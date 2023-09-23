/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <iostream>
#include <filesystem>

#include "src/core/blockchain.h"
#include "src/contract/event.h"

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
  Event testEvent(
    "TestEvent", 1, Hash::random(), 2, Hash::random(), 3,
    Address("0x1234567890abcdefecbd1234567890abcdefecbd", false),
    {
      std::make_pair(uint256_t(12345), true),
      std::make_pair(Address("0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef", false), true),
      std::make_pair(true, false),
      std::make_pair(Hash::random().asBytes(), false),
      std::make_pair(std::string("Hello World"), false),
    }
  );
  EventManager eventManager;
  eventManager.registerEvent(testEvent);
  return 0;

  /* TODO: uncomment this when done with events
  Utils::logToCout = true;
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/blockchain");
  blockchain = std::make_unique<Blockchain>(blockchainPath);
  /// Start the blockchain syncing engine.
  std::signal(SIGINT, signalHandler);
  std::signal(SIGHUP, signalHandler);
  blockchain->start();
  std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::hours(std::numeric_limits<int>::max()));
  return 0;
  */
}

