/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "httpparser.h"
#include "jsonrpc/call.h"

std::string parseJsonRpcRequest(
  const std::string& body,
  State& state,
  const Storage& storage,
  P2P::ManagerNormal& p2p,
  const Options& options
) {
  json response = jsonrpc::call(body, state, storage, p2p, options);
  Utils::safePrint("HTTP Response: " + response.dump());
  Utils::safePrint("Properly returning...");
  return response.dump();
}

