/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "src/core/blockchain.h"
#include "src/utils/clargs.h"

std::mutex cv_m;
std::condition_variable cv;
int signalCaught = 0;
std::unique_ptr<Blockchain> blockchain = nullptr;

void signalHandler(int signum) {
  bool doInterrupt = true;
  {
    std::unique_lock<std::mutex> lk(cv_m);
    if (signalCaught != 0) {
      // If signal was already set, do not call blockchain->interrupt() again.
      // This guarantees "blockchain" is valid (not yet reset) when called here.
      doInterrupt = false;
    }
    signalCaught = signum;
    Utils::safePrint(" Signal caught: " + Utils::getSignalName(signum));
  }
  cv.notify_one();
  if (doInterrupt) {
    blockchain->interrupt(); // useful if setsid happens to not be avaliable
  }
}

int main(int argc, char* argv[]) {
  Log::logToCout = true;
  SLOGINFOP("bdkd-discovery: Blockchain Development Kit discovery node daemon");

  // Parse command-line options
  ProcessOptions opt = parseCommandLineArgs(argc, argv, BDKTool::DISCOVERY_NODE);

  // Set defaults for this program in case the option is not specified
  if (opt.logLevel == "") opt.logLevel = "INFO";
  if (opt.rootPath == "") opt.rootPath = "discoveryNode";

  // Apply selected process options
  if (!applyProcessOptions(opt)) return 1;

  // Check cometbft engine
  Comet::checkCometBFT();

  // Create Blockchain
  std::string blockchainPath = std::filesystem::current_path().string() + "/" + opt.rootPath;
  SLOGINFOP("Main thread creating blockchain");
  SLOGINFOP("RootPath: " + blockchainPath);
  blockchain = std::make_unique<Blockchain>(blockchainPath, "", true); // Force CometBFT option p2p.seed_mode = true

  // Install signal handlers *after* blockchain var is set
  std::signal(SIGINT, signalHandler);
  std::signal(SIGHUP, signalHandler);

  // Start Blockchain
  try {
    SLOGINFOP("Starting blockchain...");
    blockchain->start();
  } catch (const std::exception& ex) {
    SLOGINFOP("Blockchain start failed: " + std::string(ex.what()));
  }

  // Main thread waits for a non-zero signal code to be raised and caught
  SLOGINFOP("Main thread waiting for interrupt signal...");
  int exitCode = 0;
  {
    std::unique_lock<std::mutex> lock(cv_m);
    if (!signalCaught) {
      cv.wait(lock, [] { return signalCaught != 0; });
    }
    exitCode = signalCaught;
  }
  SLOGINFOP("Main thread stopping due to interrupt signal: " + Utils::getSignalName(exitCode));

  // Shut down the node
  SLOGINFOP("Main thread stopping blockchain...");
  blockchain->stop();
  SLOGINFOP("Main thread shutting down...");
  blockchain.reset(); // Destroy the blockchain object, calling the destructor of every module and dumping to DB.

  // Return the signal code
  SLOGINFOP("Main thread exiting with code " + std::to_string(exitCode) + ".");
  return exitCode;
}
