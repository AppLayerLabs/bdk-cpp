/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "src/core/blockchain.h"
#include "src/utils/clargs.h"

std::mutex mut;
std::condition_variable cv;
int signalCaught = 0;
std::unique_ptr<Blockchain> blockchain = nullptr;

void signalHandler(int signum) {
  {
    std::unique_lock<std::mutex> lk(mut);
    Utils::safePrint(" Signal caught: " + Utils::getSignalName(signum));
    signalCaught = signum;
    if (blockchain) {
      blockchain->interrupt(); // useful is setsid is not avaliable for some reason
    }
  }
  cv.notify_one();
}

int main(int argc, char* argv[]) {
  Log::logToCout = true;
  SLOGINFOP("bdkd: Blockchain Development Kit full node daemon");
  std::signal(SIGINT, signalHandler);
  std::signal(SIGHUP, signalHandler);

  // Parse command-line options
  ProcessOptions opt = parseCommandLineArgs(argc, argv, BDKTool::FULL_NODE);

  // Set defaults for this program in case the option is not specified
  if (opt.logLevel == "") opt.logLevel = "INFO";
  if (opt.rootPath == "") opt.rootPath = "blockchain";

  // Apply selected process options
  if (!applyProcessOptions(opt)) return 1;

  // Check cometbft engine
  Comet::checkCometBFT();

  // Start the blockchain syncing engine.
  SLOGINFOP("Main thread starting node...");
  std::string blockchainPath = std::filesystem::current_path().string() + "/" + opt.rootPath;
  GLOGINFO("Full rootPath: " + blockchainPath);
  {
    std::unique_lock<std::mutex> lock(mut);
    blockchain = std::make_unique<Blockchain>(blockchainPath, "", false); // set CometBFT p2p.seed_mode = false
  }
  try {
    blockchain->start();
  } catch (const std::exception& ex) {
    SLOGINFOP("Blockchain start failed: " + std::string(ex.what()));
  }

  // Main thread waits for a non-zero signal code to be raised and caught
  SLOGINFOP("Main thread waiting for interrupt signal...");
  int exitCode = 0;
  {
    std::unique_lock<std::mutex> lock(mut);
    if (!signalCaught) {
      cv.wait(lock, [] { return signalCaught != 0; });
    }
    exitCode = signalCaught;
  }
  SLOGINFOP("Main thread stopping due to interrupt signal [" + Utils::getSignalName(exitCode) + "], shutting down node...");

  // Shut down the node
  SLOGINFOP("Main thread stopping node...");
  blockchain->stop();
  SLOGINFOP("Main thread shutting down...");
  {
    std::unique_lock<std::mutex> lock(mut);
    blockchain.reset(); // Destroy the blockchain object, calling the destructor of every module and dumping to DB.
  }
  // Return the signal code
  SLOGINFOP("Main thread exiting with code " + std::to_string(exitCode) + ".");
  return exitCode;
}
