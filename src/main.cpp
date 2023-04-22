#include <iostream>
#include "src/core/blockchain.h"
#include <filesystem>

int main() {
  /// Local binary path + /blockchain
  Utils::logToCout = true;
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/blockchain");
  Blockchain blockchain(blockchainPath);
  /// Start the blockchain syncing engine.
  blockchain.start();
  std::this_thread::sleep_for(std::chrono::hours(1000000));
  return 0;
}