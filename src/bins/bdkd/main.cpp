/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <iostream>
#include <filesystem>

#include <csignal>
#include <condition_variable>

#include "src/core/blockchain.h"
#include "src/utils/clargs.h"

#include "src/utils/logger.h"

std::unique_ptr<Blockchain> blockchain = nullptr;

std::condition_variable cv;
std::mutex cv_m;
int signalCaught = 0;

void signalHandler(int signum) {
  {
    std::unique_lock<std::mutex> lk(cv_m);
    Utils::safePrint("Signal caught: " + Utils::getSignalName(signum));
    signalCaught = signum;
  }
  cv.notify_one();
}

#include "src/contract/templates/btvcommon.h"

int main(int argc, char* argv[]) {

  std::cout << "sizeof(BTVUtils::ChunkCoord2D): " << sizeof(BTVUtils::ChunkCoord2D) << std::endl;

  return 0;
  Log::logToCout = true;
  Utils::safePrint("bdkd: Blockchain Development Kit full node daemon");
  std::signal(SIGINT, signalHandler);
  std::signal(SIGHUP, signalHandler);

  // Parse command-line options
  ProcessOptions opt = parseCommandLineArgs(argc, argv, BDKTool::FULL_NODE);

  // Select a default log level for this program if none is specified
  if (opt.logLevel == "") opt.logLevel = "INFO";

  // Apply selected process options
  if (!applyProcessOptions(opt)) return 1;

  // Start the blockchain syncing engine.
  Utils::safePrint("Main thread starting node...");
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/blockchain");
  blockchain = std::make_unique<Blockchain>(blockchainPath);
  blockchain->start();

  // Main thread waits for a non-zero signal code to be raised and caught
  Utils::safePrint("Main thread waiting for interrupt signal...");
  int exitCode = 0;
  {
    std::unique_lock<std::mutex> lk(cv_m);
    cv.wait(lk, [] { return signalCaught != 0; });
    exitCode = signalCaught;
  }
  Utils::safePrint("Main thread stopping due to interrupt signal [" + Utils::getSignalName(exitCode) + "], shutting down node...");

  // Shut down the node
  SLOGINFO("Received signal " + std::to_string(exitCode));
  Utils::safePrint("Main thread stopping node...");
  blockchain->stop();
  Utils::safePrint("Main thread shutting down...");
  blockchain = nullptr; // Destroy the blockchain object, calling the destructor of every module and dumping to DB.

  // Return the signal code
  Utils::safePrint("Main thread exiting with code " + std::to_string(exitCode) + ".");
  return exitCode;
}
