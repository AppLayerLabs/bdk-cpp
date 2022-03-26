#include "main.h"
int main() {
  // Create genesis block.

  std::string genesisPrevBlock = "0000000000000000000000000000000000000000000000000000000000000000";
  dev::u256 genesisTimestamp = 0;
  dev::u256 genesisTxCount = 0;
  dev::u256 genesisnHeight = 0;
  dev::u256 genesisBlockData = 0;
  Block genesis(genesisPrevBlock, genesisTimestamp, genesisTxCount, genesisnHeight, genesisBlockData);

  std::cout << genesis.blockHash() << std::endl;
  return 0;
  RunServer();

  return 0;
}