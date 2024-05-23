/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <csignal>
#include <condition_variable>

#include "net/p2p/managerdiscovery.h"
#include "utils/options.h"
#include "iostream"
#include "src/utils/clargs.h"

std::condition_variable cv;
std::mutex cv_m;
int signalCaught = 0;

void signalHandler(int signum) {
  {
    std::unique_lock<std::mutex> lk(cv_m);
    std::cout << std::endl << "Signal caught: " << Utils::getSignalName(signum) << std::endl;
    signalCaught = signum;
  }
  cv.notify_one();
}

/// Executable with a discovery node for the given Options default chain.
int main(int argc, char* argv[]) {
  Utils::logToCout = true;
  Utils::safePrint("bdkd-discovery: Blockchain Development Kit discovery node daemon");
  std::signal(SIGINT, signalHandler);
  std::signal(SIGHUP, signalHandler);

  // Parse command-line options
  ProcessOptions opt = parseCommandLineArgs(argc, argv, BDKTool::DISCOVERY_NODE);

  // Select a default log level for this program if none is specified
  if (opt.logLevel == "") opt.logLevel = "INFO";

  // Apply selected process options
  if (!applyProcessOptions(opt)) return 1;

  // Start the discovery node
  Utils::safePrint("Main thread starting node...");
  // Local binary path + /blockchain
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/discoveryNode");
  const auto options = Options::fromFile(blockchainPath);
  std::unique_ptr<P2P::ManagerDiscovery> p2p = std::make_unique<P2P::ManagerDiscovery>(options.getP2PIp(), options);
  p2p->start();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  p2p->startDiscovery();

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
  Logger::logToDebug(LogType::INFO, "MAIN", "MAIN", "Received signal " + std::to_string(exitCode));
  Utils::safePrint("Main thread stopping node...");
  p2p->stopDiscovery();
  Utils::safePrint("Main thread shutting down...");
  p2p.reset();

  // Return the signal code
  Utils::safePrint("Main thread exiting with code " + std::to_string(exitCode) + ".");
  return exitCode;
}

