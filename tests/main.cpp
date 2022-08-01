#include <iostream>
#include "tests.h"

int main() {
  std::cout << "UNIT TESTING STARTED" << std::endl;

  Tests::transactions();
  Tests::uint256ToBytes();
  Tests::uint160ToBytes();
  Tests::uint64ToBytes();
  Tests::uint32ToBytes();
  Tests::uint8ToBytes();
  Tests::bytesToUint256();
  Tests::bytesToUint160();
  Tests::bytesToUint64();
  Tests::bytesToUint32();
  Tests::bytesToUint8();

  std::cout << "UNIT TESTING FINISHED" << std::endl;
  return 0;
}
