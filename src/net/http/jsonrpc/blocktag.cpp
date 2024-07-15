#include "blocktag.h"
#include "error.h"

/**
 * Helper type for std::visit.
 * See https://en.cppreference.com/w/cpp/utility/variant/visit
*/
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };


/// Explicit deduction guide
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;


namespace jsonrpc {

bool BlockTagOrNumber::isLatest(const Storage& storage) const {
  return number(storage) == storage.latest()->getNHeight();
}

uint64_t BlockTagOrNumber::number(const Storage& storage) const {
  return std::visit(Overloaded(
    [](uint64_t number) -> uint64_t { return number; },
    [&storage](BlockTag tag) -> uint64_t {
      if (tag == BlockTag::LATEST) return storage.latest()->getNHeight();
      if (tag == BlockTag::EARLIEST) return 0u;
      throw Error(-32601, "Pending block not supported for operation");
    }
  ), tagOrNumber_);
}

BlockTag Parser<BlockTag>::operator()(const json& data) const {
  if (!data.is_string())
    throw Error::invalidType("string", data.type_name());

  auto value = data.get<std::string>();

  if (value == "latest")
    return BlockTag::LATEST;

  if (value == "earliest")  
    return BlockTag::EARLIEST;

  if (value == "pending")
    return BlockTag::PENDING;

  throw Error::invalidFormat(value);
}

BlockTagOrNumber Parser<BlockTagOrNumber>::operator()(const json& data) const {
  return BlockTagOrNumber(Parser<std::variant<uint64_t, BlockTag>>{}(data));
}

} // namespace jsonrpc
