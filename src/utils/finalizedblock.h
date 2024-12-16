#ifndef _FINALIZEDBLOCK_H_
#define _FINALIZEDBLOCK_H_

#include <vector>

#include "uintconv.h"

/**
 * A block produced by the consensus engine.
 */
struct FinalizedBlock {
  std::vector<Bytes> txs; ///< Transactions in the block
  uint64_t height = 0; ///< Block height (0 = unknown/unset)
};

#endif
