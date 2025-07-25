/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_CALL_H
#define JSONRPC_CALL_H

#include "../../p2p/managernormal.h"

#include "variadicparser.h" // parser.h -> utils/utils.h -> libs/json.hpp

/// Namespace for JSON-RPC-related functionalities.
namespace jsonrpc {
  /**
   * Process a JSON-RPC call.
   * @param request The request in JSON format.
   * @param state Reference to the chain state.
   * @param storage Reference to the chain storage.
   * @param p2p Reference to the P2P manager.
   * @param options Reference to the global options.
   */
  json call(
    const json& request, State& state, const Storage& storage,
    P2P::ManagerNormal& p2p, const Options& options
  ) noexcept;

  /**
   * Check that the JSON RPC request object conforms to the formatting standards.
   * @param request The JSON RPC request object to check.
   * @throw DynamicException on any non-conforming field.
   */
  void checkJsonRPCSpec(const json& request);
} // namespace jsonrpc

#endif // JSONRPC_CALL_H
