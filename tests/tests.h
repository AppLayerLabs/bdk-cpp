#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <cassert>
#include <vector>
#include <memory>

// Keep includes in tests.h minimal.
// Forward declaration
class ChainHead;
class ChainTip;
class State;
class Address;

namespace Tests {
  // Tx::Base
  void transactions();

  // Tx::Base::Sign
  void transactionSign();

  // Utils
  void uint256ToBytes();
  void uint160ToBytes();
  void uint64ToBytes();
  void uint32ToBytes();
  void uint16ToBytes();
  void uint8ToBytes();
  void bytesToUint256();
  void bytesToUint160();
  void bytesToUint64();
  void bytesToUint32();
  void bytesToUint16();
  void bytesToUint8();
  void toLowercaseAddress();
  void toUppercaseAddress();
  void toChecksumAddress();
  void isAddress();
  void checkAddressChecksum();
  void testABIDecoder();
  void testABIJSONDecoder();
  void testSecp256k1();
}

#endif // TESTS_H
