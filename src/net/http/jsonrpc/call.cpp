/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "call.h"
#include "methods.h"

#include <unordered_map>
#include <string>
#include <functional>
#include "../utils/logger.h"

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
      {"eth_getUncleByBlockHashAndIndex", [](NodeRPCInterface& obj, const json&) { return obj.eth_getUncleByBlockHashAndIndex(); }}
    };

    // Look up the method in the map
    auto it = methodMap.find(std::string(method));
    if (it == methodMap.end()) {
      throw Error::methodNotAvailable(method);
    }
    result = it->second(rpc, request);

    /*
        FIXME: this goes into a JSON-RPC call handler object
        that is actually responsible for implementing a string method vector string params call
        not even a Blockchain& object should be given to httpserver really
        and we could simply pass around a method pointer (binds to blockchain) for
        the handler instead of using subclassing
        */
       //throw Error::methodNotAvailable(method);
    /*
    if (method == "web3_clientVersion")
      result = jsonrpc::web3_clientVersion(request);
    else if (method == "web3_sha3")
      result = jsonrpc::web3_sha3(request);
    else if (method == "net_version")
      result = jsonrpc::net_version(request);
    else if (method == "net_listening")
      result = jsonrpc::net_listening(request);
    else if (method == "net_peerCount")
      result = jsonrpc::net_peerCount(request);
    else if (method == "eth_protocolVersion")
      result = jsonrpc::eth_protocolVersion(request);
    else if (method == "eth_getBlockByHash")
      result = jsonrpc::eth_getBlockByHash(request);
    else if (method == "eth_getBlockByNumber")
      result = jsonrpc::eth_getBlockByNumber(request);
    else if (method == "eth_getBlockTransactionCountByHash")
      result = jsonrpc::eth_getBlockTransactionCountByHash(request);
    else if (method == "eth_getBlockTransactionCountByNumber")
      result = jsonrpc::eth_getBlockTransactionCountByNumber(request);
    else if (method == "eth_chainId")
      result = jsonrpc::eth_chainId(request);
    else if (method == "eth_syncing")
      result = jsonrpc::eth_syncing(request);
    else if (method == "eth_coinbase")
      result = jsonrpc::eth_coinbase(request);
    else if (method == "eth_blockNumber")
      result = jsonrpc::eth_blockNumber(request);
    else if (method == "eth_call")
      result = jsonrpc::eth_call(request);
    else if (method == "eth_estimateGas")
      result = jsonrpc::eth_estimateGas(request);
    else if (method == "eth_gasPrice")
      result = jsonrpc::eth_gasPrice(request);
    else if (method == "eth_feeHistory")
      result = jsonrpc::eth_feeHistory(request);
    else if (method == "eth_getLogs")
      result = jsonrpc::eth_getLogs(request);
    else if (method == "eth_getBalance")
      result = jsonrpc::eth_getBalance(request);
    else if (method == "eth_getTransactionCount")
      result = jsonrpc::eth_getTransactionCount(request);
    else if (method == "eth_getCode")
      result = jsonrpc::eth_getCode(request);
    else if (method == "eth_sendRawTransaction")
      result = jsonrpc::eth_sendRawTransaction(request);
    else if (method == "eth_getTransactionByHash")
      result = jsonrpc::eth_getTransactionByHash(request);
    else if (method == "eth_getTransactionByBlockHashAndIndex")
      result = jsonrpc::eth_getTransactionByBlockHashAndIndex(request);
    else if (method == "eth_getTransactionByBlockNumberAndIndex")
      result = jsonrpc::eth_getTransactionByBlockNumberAndIndex(request);
    else if (method == "eth_getTransactionReceipt")
      result = jsonrpc::eth_getTransactionReceipt(request);
    else if (method == "eth_getUncleByBlockHashAndIndex")
      result = jsonrpc::eth_getUncleByBlockHashAndIndex();
    else if (method == "txpool_content")
      result = jsonrpc::txpool_content(request);
    else if (method == "debug_traceBlockByNumber")
      result = jsonrpc::debug_traceBlockByNumber(request);
    else if (method == "debug_traceTransaction")
      result = jsonrpc::debug_traceTransaction(request);
    else
      throw Error::methodNotAvailable(method);
      */

    ret["result"] = std::move(result);

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
