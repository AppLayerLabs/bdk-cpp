/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

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
  /// @param storage the blockchain storage used for querying block information
  /// @return if the block is currently the latest in the chain
  bool isLatest(const Storage& storage) const;

  /// @brief retrives the block number (NHeight) in the chain
  /// @param storage the blockchain storage used for querying the block number from tags
  /// @return the block number (NHeight)
  uint64_t number(const Storage& storage) const;

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
