#ifndef JSONRPC_CALL_H
#define JSONRPC_CALL_H

#include "variadicparser.h"
#include "../../p2p/managernormal.h"

namespace jsonrpc {

/// @brief process a json RPC call
/// @param request the request in string format
/// @param state reference to the chain state
/// @param p2p reference to the P2P manager
/// @param options reference to the global options
json call(const std::string& requestStr,
  State& state,
  const Storage& storage,
  P2P::ManagerNormal& p2p,
  const Options& options) noexcept;

void checkJsonRPCSpec(const json& request);

} // namespace jsonrpc

#endif // JSONRPC_CALL_H
