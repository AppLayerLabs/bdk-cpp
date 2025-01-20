#ifndef JSONRPC_BLOCKTAG_H
#define JSONRPC_BLOCKTAG_H

#include "parser.h"

namespace jsonrpc {


/// @brief used to identify blocks using tags
enum class BlockTag {
  LATEST,
  EARLIEST,
  PENDING
};

/// @brief Useful wrapper for a block tag or number
class BlockTagOrNumber {
public:

  /// @param tagOrNumber the block tag or block number
  explicit BlockTagOrNumber(const std::variant<uint64_t, BlockTag>& tagOrNumber)
    : tagOrNumber_(tagOrNumber) {}

  /// @brief checks if the block is the latest
  /// @return if the block is currently the latest in the chain
  bool isLatest(const uint64_t latestHeight) const;

  /// @brief retrives the block number (NHeight) in the chain
  /// @return the block number (NHeight)
  uint64_t number(const uint64_t latestHeight) const;

private:
  std::variant<uint64_t, BlockTag> tagOrNumber_;
};

/// @brief specialization for parsing block tags
/// @example "latest", "pending", "earliest"
template<> struct Parser<BlockTag> { BlockTag operator()(const json& data) const; };

/// @brief specialization for parsing block tags or number
/// @example "latest", "0x1B"
template<> struct Parser<BlockTagOrNumber> { BlockTagOrNumber operator()(const json& data) const; };

} // namespace jsonrpc

#endif // JSONRPC_BLOCKTAG_H
