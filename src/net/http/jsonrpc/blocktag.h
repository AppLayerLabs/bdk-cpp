/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_BLOCKTAG_H
#define JSONRPC_BLOCKTAG_H

#include "parser.h"

namespace jsonrpc {

/// Enum for identifying blocks using tags.
enum class BlockTag { LATEST, EARLIEST, PENDING };

/// Wrapper class for a block tag or number.
class BlockTagOrNumber {
  private:
    std::variant<uint64_t, BlockTag> tagOrNumber_; ///< The block tag or number.

  public:
    /**
     * Constructor.
     * @param tagOrNumber The block tag or number.
     */
    explicit BlockTagOrNumber(const std::variant<uint64_t, BlockTag>& tagOrNumber) : tagOrNumber_(tagOrNumber) {}

    /**
     * Check if the block tag or number is the latest in storage.
     * @param storage The blockchain storage used for querying block information.
     * @return `true` if block is currently the latest in the chain, `false` otherwise.
     */
    bool isLatest(const Storage& storage) const;

    /**
     * Retrieve the block number (nHeight) in the chain.
     * @param storage The blockchain storage used for querying the block number from tags.
     * @return The block number (nHeight).
     */
    uint64_t number(const Storage& storage) const;
};

/// Template specialization for parsing block tags (e.g. "latest", "pending", "earliest").
template<> struct Parser<BlockTag> {
  /**
   * Function call operator.
   * @param data The JSON object to parse.
   * @return The proper block tag.
   */
  BlockTag operator()(const json& data) const;
};

/// Template specialization for parsing block tags or numbers (e.g. "latest", "0x1B").
template<> struct Parser<BlockTagOrNumber> {
  /**
   * Function call operator.
   * @param data The JSON object to parse.
   * @return The proper block tag or number.
   */
  BlockTagOrNumber operator()(const json& data) const;
};

} // namespace jsonrpc

#endif // JSONRPC_BLOCKTAG_H
