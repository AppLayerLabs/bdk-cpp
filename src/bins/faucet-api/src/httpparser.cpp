/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "httpparser.h"

namespace Faucet {
  std::string parseJsonRpcRequest(
    const std::string& body, Manager& faucet
  ) {
    json ret;
    uint64_t id = 0;
    try {
      json request = json::parse(body);
      if (!JsonRPC::Decoding::checkJsonRPCSpec(request)) {
        ret["error"]["code"] = -32600;
        ret["error"]["message"] = "Invalid request - does not conform to JSON-RPC 2.0 spec";
        return ret.dump();
      }

      auto RequestMethod = JsonRPC::Decoding::getMethod(request);
      switch (RequestMethod) {
        case JsonRPC::Methods::invalid:
          Utils::safePrint("INVALID METHOD: " + request["method"].get<std::string>());
          ret["error"]["code"] = -32601;
          ret["error"]["message"] = "Method not found";
        break;
        case JsonRPC::Methods::dripToAddress:
          JsonRPC::Decoding::dripToAddress(request, faucet);
          ret = JsonRPC::Encoding::dripToAddress();
        break;
        default:
          ret["error"]["code"] = -32601;
          ret["error"]["message"] = "Method not found";
        break;
      }
      if (request["id"].is_string()) {
        ret["id"] = request["id"].get<std::string>();
      } else if (request["id"].is_number()) {
        ret["id"] = request["id"].get<uint64_t>();
      } else if(request["id"].is_null()) {
        ret["id"] = nullptr;
      } else {
        throw DynamicException("Invalid id type");
      }
    } catch (std::exception &e) {
      json error;
      error["id"] = id;
      error["jsonrpc"] = 2.0;
      error["error"]["code"] = -32603;
      error["error"]["message"] = "Internal error: " + std::string(e.what());
      return error.dump();
    }
    // Set back to the original id
    return ret.dump();
  }
}

