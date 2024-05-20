#ifndef JSONRPC_CALL_H
#define JSONRPC_CALL_H

#include "variadicparser.h"
#include "../../p2p/managernormal.h"

namespace jsonrpc {

json call(const json& request,
  State& state,
  const Storage& storage,
  P2P::ManagerNormal& p2p,
  const Options& options) noexcept;

} // namespace jsonrpc

#endif // JSONRPC_CALL_H
