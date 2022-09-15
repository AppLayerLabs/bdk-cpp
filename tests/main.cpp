#include <iostream>
#include "tests.h"

int main() {
  std::cout << "UNIT TESTING STARTED" << std::endl;

  Tests::transactions();
  Tests::transactionSign();
  Tests::uint256ToBytes();
  Tests::uint160ToBytes();
  Tests::uint64ToBytes();
  Tests::uint32ToBytes();
  Tests::uint16ToBytes();
  Tests::uint8ToBytes();
  Tests::bytesToUint256();
  Tests::bytesToUint160();
  Tests::bytesToUint64();
  Tests::bytesToUint32();
  Tests::bytesToUint16();
  Tests::bytesToUint8();
  Tests::toLowercaseAddress();
  Tests::toUppercaseAddress();
  Tests::toChecksumAddress();
  Tests::isAddress();
  Tests::checkAddressChecksum();
  Tests::testABIDecoder();

  // Create a simulation of running network, without the gRPC dependencies.
  Tests::testBlockchain();

  std::cout << "UNIT TESTING FINISHED" << std::endl;
  return 0;
}

