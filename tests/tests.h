#ifndef TESTS_H
#define TESTS_H

#include <iostream>
#include <cassert>
#include <vector>
#include <memory>

// Keep includes in tests.h minimal.
// Forward declaration
class ChainHead;
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

  // testBlockchain uses generateAddress, doBlocks, doTransactions, doBlocksAndTxs and addBalance to run.
  void testBlockchain();

  // These addresses are not random to test against invalid pubkey derivation.
  std::vector<std::pair<Address,std::string>> generateAddresses(uint64_t quantity);
  void doBlocks(uint32_t quantity, std::shared_ptr<ChainHead> &chainHead, std::shared_ptr<State> &state);
  void doTransactions(uint32_t txs, std::shared_ptr<ChainHead> &chainHead, std::shared_ptr<State> &state, std::vector<std::pair<Address,std::string>> accounts);
  void doBlocksAndTxs(uint32_t blocks, uint32_t txsPerBlock, std::shared_ptr<ChainHead> &chainHead, std::shared_ptr<State> &state);
  void addBalance(std::vector<std::pair<Address,std::string>> &accounts, std::shared_ptr<State> &state);

  void testABIDecoder();
}

#endif // TESTS_H
