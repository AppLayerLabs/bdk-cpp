#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <cassert>

namespace Tests {
  // Tx::Base
  void transactions();

  // Utils
  void uint256ToBytes();
  void uint160ToBytes();
  void uint64ToBytes();
  void uint32ToBytes();
  void uint8ToBytes();
  void bytesToUint256();
  void bytesToUint160();
  void bytesToUint64();
  void bytesToUint32();
  void bytesToUint8();
}

#endif // TESTS_H
