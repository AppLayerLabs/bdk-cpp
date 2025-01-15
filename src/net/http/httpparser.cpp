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
  Utils::safePrint("HTTP Request: " + body);
  json ret;
  try {
    json request = json::parse(body);
    if (request.is_array()) {
      ret = json::array();
      for (const auto& req : request) {
        ret.emplace_back(jsonrpc::call(req, state, storage, p2p, options));
      }
    } else {
      ret = jsonrpc::call(request, state, storage, p2p, options);
    }
  } catch (const std::exception& e) {
    ret["error"]["code"] = -32603;
    ret["error"]["message"] = std::string("Internal error: ") + std::string(e.what());
  }

  Utils::safePrint("HTTP Response: " + ret.dump());
  Utils::safePrint("Properly returning...");
  return ret.dump();
}

