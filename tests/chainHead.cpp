#include "tests.h"
#include "../src/chainHead.h"
#include "../src/state.h"

void Tests::doBlocks(uint32_t quantity, std::unique_ptr<ChainHead> &chainHead, std::unique_ptr<State> &state) {
  auto latest = chainHead->latest();
  for (uint32_t i = 0; i < quantity; ++i) {
    if (!state->createNewBlock(chainHead)) {
      std::runtime_error("doBlocks: createNewBlock failed!");
    }
  }

  // Validate blocks prevnHeight and prevBlockHash
  uint64_t end = latest.nHeight() + quantity;
  for (uint64_t index = latest.nHeight(); index < end; ++index) {
    latest = chainHead->getBlock(index);
    uint64_t nextBlock = (index + 1);
    Block newBestBlock = chainHead->getBlock(nextBlock);
    assert(newBestBlock.prevBlockHash() == latest.getBlockHash());
    assert(newBestBlock.nHeight() == (latest.nHeight() + 1));
    assert(newBestBlock.transactions().empty());
  }

  std::cout << __func__ << " with " << quantity << " blocks OK" << std::endl;
}