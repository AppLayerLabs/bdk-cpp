#include "tests.h"
#include "../src/core/chainHead.h"
#include "../src/core/state.h"

void Tests::doBlocks(uint32_t quantity, std::shared_ptr<ChainHead> &chainHead, std::shared_ptr<ChainTip> &chainTip, std::shared_ptr<State> &state) {
  // TODO: Fix tests as chainHead and state now operates with a third class, ChainTip.
  const std::shared_ptr<const Block> latest = chainHead->latest();
  for (uint32_t i = 0; i < quantity; ++i) {
    if (!state->createNewBlock(chainHead, chainTip)) {
      std::runtime_error("doBlocks: createNewBlock failed!");
    }
  }

  // Validate blocks prevnHeight and prevBlockHash
  uint64_t end = latest->nHeight() + quantity;
  for (uint64_t index = latest->nHeight(); index < end; ++index) {
    auto nLatest = chainHead->getBlock(index);
    uint64_t nextBlock = (index + 1);
    auto newBestBlock = chainHead->getBlock(nextBlock);
    assert(newBestBlock->prevBlockHash() == nLatest->getBlockHash());
    assert(newBestBlock->nHeight() == (nLatest->nHeight() + 1));
  }

  std::cout << __func__ << " with " << quantity << " blocks OK" << std::endl;
}
