#ifndef JSONRPC_BLOCKTAG_H
#define JSONRPC_BLOCKTAG_H

#include "parser.h"

namespace jsonrpc {

enum class BlockTag {
  LATEST,
  EARLIEST,
  PENDING
};

class BlockTagOrNumber {
public:
  explicit BlockTagOrNumber(const std::variant<uint64_t, BlockTag>& tagOrNumber)
    : tagOrNumber_(tagOrNumber) {}

  bool isLatest(const Storage& storage) const;

  uint64_t number(const Storage& storage) const;

private:
  std::variant<uint64_t, BlockTag> tagOrNumber_;
};

template<> struct Parser<BlockTag> { BlockTag operator()(const json& data) const; };

template<> struct Parser<BlockTagOrNumber> { BlockTagOrNumber operator()(const json& data) const; };

} // namespace jsonrpc

#endif // JSONRPC_BLOCKTAG_H
