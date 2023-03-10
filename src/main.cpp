#include <iostream>
#include "utils/block.h"
#include "utils/strings.h"
#include "utils/utils.h"

// Dummy main.cpp file
int main() {
  PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
  Hash nPrevBlockHash(Hex::toBytes("97a5ebd9bbb5e330b0b3c74b9816d595ffb7a04d4a29fb117ea93f8a333b43be"));
  uint64_t timestamp = 1678400843315;
  uint64_t nHeight = 100;
  Block newBlock = Block(nPrevBlockHash, timestamp, nHeight);
  TxBlock tx(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"));

  for (uint64_t i = 0; i < 20000; i++) newBlock.appendTx(tx);

  newBlock.finalize(validatorPrivKey);

  Block reconstructedBlock(newBlock.serializeBlock());

  assert(reconstructedBlock.getTxs() == newBlock.getTxs());

  return 0;
}

