#include <iostream>
#include "src/core/blockchain.h"
#include <filesystem>
#include "src/contract/variables/safeuint64_t.h"
#include "src/contract/variables/safeuint256_t.h"
#include "src/contract/variables/safeaddress.h"
#include "src/contract/variables/safeunorderedmap.h"


int main() {
  uint256_t i ("1927831865120318940191371489123952378115126713");

  auto newValue = i % 10000;

  std::cout << newValue << std::endl;

  return 0;

  /// Local binary path + /blockchain
  std::string blockchainPath = std::filesystem::current_path().string() + std::string("/blockchain");
  Blockchain blockchain(blockchainPath);
  /// Start the blockchain syncing engine.
  blockchain.start();
  std::this_thread::sleep_for(std::chrono::hours(1000000));
  return 0;
}