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
  void uint8ToBytes();
  void bytesToUint256();
  void bytesToUint160();
  void bytesToUint64();
  void bytesToUint32();
  void bytesToUint8();

  // testBlockchain uses generateAddress, doBlocks, doTransactions, doBlocksAndTxs and addBalance to run.
  void testBlockchain();
  // These addresses are not random to test agains't invalid pubkey derivation.
  std::vector<std::pair<Address,std::string>> generateAddresses(uint64_t quantity);
  void doBlocks(uint32_t quantity, std::unique_ptr<ChainHead> &chainHead, std::unique_ptr<State> &state);
  void doTransactions(uint32_t txs, std::unique_ptr<ChainHead> &chainHead, std::unique_ptr<State> &state, std::vector<std::pair<Address,std::string>> accounts);
  void doBlocksAndTxs(uint32_t blocks, uint32_t txsPerBlock, std::unique_ptr<ChainHead> &chainHead, std::unique_ptr<State> &state);
  void addBalance(std::vector<std::pair<Address,std::string>> &accounts, std::unique_ptr<State> &state);
}

#endif // TESTS_H
