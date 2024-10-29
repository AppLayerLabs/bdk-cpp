/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_CALL_H
#define JSONRPC_CALL_H

#include "../../p2p/managernormal.h"

#include "variadicparser.h" // parser.h -> utils/utils.h -> libs/json.hpp

namespace jsonrpc {

/// @brief process a json RPC call
/// @param request the request in JSON format
/// @param state reference to the chain state
/// @param p2p reference to the P2P manager
/// @param options reference to the global options
json call(const json& request,
  State& state,
  const Storage& storage,
  P2P::ManagerNormal& p2p,
  const Options& options) noexcept;

void checkJsonRPCSpec(const json& request);

} // namespace jsonrpc

#endif // JSONRPC_CALL_H
