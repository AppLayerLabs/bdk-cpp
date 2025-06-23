/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "call.h"

#include <unordered_map>
#include <string>
#include <functional>
#include "../utils/logger.h"
#include "../utils/utils.h"


namespace jsonrpc {
void checkJsonRPCSpec(const json& request) {
  try {
    // "jsonrpc": "2.0" is a MUST
    if (!request.contains("jsonrpc")) {
      throw DynamicException("jsonrpc field is missing");
    }
    if (request["jsonrpc"].get<std::string>() != "2.0") {
      throw DynamicException("jsonrpc field is not 2.0");
    }
    // "method" is a MUST
    if (!request.contains("method")) {
      throw DynamicException("method field is missing");
    }
    // Params MUST be Object or Array.
    if (
      request.contains("params") && (!request["params"].is_object() && !request["params"].is_array())
    ) throw DynamicException("params field is not an object or array");
  } catch (const std::exception& e) {
    SLOGERROR("Error while checking json RPC spec: " + std::string(e.what()));
    throw DynamicException("Error while checking json RPC spec: " + std::string(e.what()));
  }
}

json call(const json& request, NodeRPCInterface& rpc) noexcept {
  json ret;
  try {
    checkJsonRPCSpec(request);
    json result;
    ret["jsonrpc"] = "2.0";
  
    json id = request["id"];

    if (!(id.is_string() || id.is_number() || id.is_null()))
      throw DynamicException("Invalid id type");

    ret["id"] = std::move(id);

    auto method = request.at("method").get<std::string_view>();

    using MethodHandler = std::function<json(NodeRPCInterface&, const json&)>;

    static const std::unordered_map<std::string, MethodHandler> methodMap = {
      {"web3_clientVersion", [](NodeRPCInterface& obj, const json& req) { return obj.web3_clientVersion(req); }},
      {"web3_sha3", [](NodeRPCInterface& obj, const json& req) { return obj.web3_sha3(req); }},
      {"net_version", [](NodeRPCInterface& obj, const json& req) { return obj.net_version(req); }},
      {"net_listening", [](NodeRPCInterface& obj, const json& req) { return obj.net_listening(req); }},
      {"eth_protocolVersion", [](NodeRPCInterface& obj, const json& req) { return obj.eth_protocolVersion(req); }},
      {"net_peerCount", [](NodeRPCInterface& obj, const json& req) { return obj.net_peerCount(req); }},
      {"eth_getBlockByHash", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getBlockByHash(req); }},
      {"eth_getBlockByNumber", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getBlockByNumber(req); }},
      {"eth_getBlockTransactionCountByHash", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getBlockTransactionCountByHash(req); }},
      {"eth_getBlockTransactionCountByNumber", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getBlockTransactionCountByNumber(req); }},
      {"eth_chainId", [](NodeRPCInterface& obj, const json& req) { return obj.eth_chainId(req); }},
      {"eth_syncing", [](NodeRPCInterface& obj, const json& req) { return obj.eth_syncing(req); }},
      {"eth_coinbase", [](NodeRPCInterface& obj, const json& req) { return obj.eth_coinbase(req); }},
      {"eth_blockNumber", [](NodeRPCInterface& obj, const json& req) { return obj.eth_blockNumber(req); }},
      {"eth_call", [](NodeRPCInterface& obj, const json& req) { return obj.eth_call(req); }},
      {"eth_estimateGas", [](NodeRPCInterface& obj, const json& req) { return obj.eth_estimateGas(req); }},
      {"eth_gasPrice", [](NodeRPCInterface& obj, const json& req) { return obj.eth_gasPrice(req); }},
      {"eth_feeHistory", [](NodeRPCInterface& obj, const json& req) { return obj.eth_feeHistory(req); }},
      {"eth_getLogs", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getLogs(req); }},
      {"eth_getBalance", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getBalance(req); }},
      {"eth_getTransactionCount", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getTransactionCount(req); }},
      {"eth_getCode", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getCode(req); }},
      {"eth_sendRawTransaction", [](NodeRPCInterface& obj, const json& req) { return obj.eth_sendRawTransaction(req); }},
      {"eth_getTransactionByHash", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getTransactionByHash(req); }},
      {"eth_getTransactionByBlockHashAndIndex", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getTransactionByBlockHashAndIndex(req); }},
      {"eth_getTransactionByBlockNumberAndIndex", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getTransactionByBlockNumberAndIndex(req); }},
      {"eth_getTransactionReceipt", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getTransactionReceipt(req); }},
      {"eth_getUncleByBlockHashAndIndex", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getUncleByBlockHashAndIndex(req); }},
      {"eth_maxPriorityFeePerGas", [](NodeRPCInterface& obj, const json& req) { return obj.eth_maxPriorityFeePerGas(req); }},
      {"debug_traceBlockByNumber", [](NodeRPCInterface& obj, const json& req) { return obj.debug_traceBlockByNumber(req); }},
      {"debug_traceTransaction", [](NodeRPCInterface& obj, const json& req) { return obj.eth_getUncleByBlockHashAndIndex(req); }},
      {"txpool_content", [](NodeRPCInterface& obj, const json& req) { return obj.txpool_content(req); }}
    };

    // Look up the method in the map
    auto it = methodMap.find(std::string(method));
    if (it == methodMap.end()) {
      throw Error::methodNotAvailable(method);
    }
    result = it->second(rpc, request);

    ret["result"] = std::move(result);
  } catch (const VMExecutionError& err) {
    ret["error"]["code"] = err.code();
    ret["error"]["message"] = err.message();
    ret["error"]["data"] = err.data();
    ret["error"]["name"] = "CallError";
  } catch (const Error& err) {
    ret["error"]["code"] = err.code();
    ret["error"]["message"] = err.message();
  } catch (const std::exception& e) {
    ret["error"]["code"] = -32603;
    ret["error"]["message"] = std::string("Internal error: ") + std::string(e.what());
  }

  return ret;
}

} // namespace jsonrpc
