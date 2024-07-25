#include "call.h"
#include "methods.h"

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

json call(const std::string& requestStr, State& state, const Storage& storage,
          P2P::ManagerNormal& p2p, const Options& options) noexcept {
  json ret;

  try {
    json request = json::parse(requestStr);
    checkJsonRPCSpec(request);
    json result;
    ret["jsonrpc"] = 2.0;
  
    json id = request["id"];

    if (!(id.is_string() || id.is_number() || id.is_null()))
      throw DynamicException("Invalid id type");

    ret["id"] = std::move(id);

    auto method = request.at("method").get<std::string_view>();

    if (method == "web3_clientVersion")
      result = jsonrpc::web3_clientVersion(request, options);
    else if (method == "web3_sha3")
      result = jsonrpc::web3_sha3(request);
    else if (method == "net_version")
      result = jsonrpc::net_version(request, options);
    else if (method == "net_listening")
      result = jsonrpc::net_listening(request);
    else if (method == "net_peerCount")
      result = jsonrpc::net_peerCount(request, p2p);
    else if (method == "eth_protocolVersion")
      result = jsonrpc::eth_protocolVersion(request, options);
    else if (method == "eth_getBlockByHash")
      result = jsonrpc::eth_getBlockByHash(request, storage);
    else if (method == "eth_getBlockByNumber")
      result = jsonrpc::eth_getBlockByNumber(request, storage);
    else if (method == "eth_getBlockTransactionCountByHash")
      result = jsonrpc::eth_getBlockTransactionCountByHash(request, storage);
    else if (method == "eth_getBlockTransactionCountByNumber")
      result = jsonrpc::eth_getBlockTransactionCountByNumber(request, storage);
    else if (method == "eth_chainId")
      result = jsonrpc::eth_chainId(request, options);
    else if (method == "eth_syncing")
      result = jsonrpc::eth_syncing(request);
    else if (method == "eth_coinbase")
      result = jsonrpc::eth_coinbase(request, options);
    else if (method == "eth_blockNumber")
      result = jsonrpc::eth_blockNumber(request, storage);
    else if (method == "eth_call")
      result = jsonrpc::eth_call(request, storage, state);
    else if (method == "eth_estimateGas")
      result = jsonrpc::eth_estimateGas(request, storage, state);
    else if (method == "eth_gasPrice")
      result = jsonrpc::eth_gasPrice(request);
    else if (method == "eth_feeHistory")
      result = jsonrpc::eth_feeHistory(request, storage);
    else if (method == "eth_getLogs")
      result = jsonrpc::eth_getLogs(request, storage, state);
    else if (method == "eth_getBalance")
      result = jsonrpc::eth_getBalance(request, storage, state);
    else if (method == "eth_getTransactionCount")
      result = jsonrpc::eth_getTransactionCount(request, storage, state);
    else if (method == "eth_getCode")
      result = jsonrpc::eth_getCode(request, storage, state);
    else if (method == "eth_sendRawTransaction")
      result = jsonrpc::eth_sendRawTransaction(request, options.getChainID(), state, p2p);
    else if (method == "eth_getTransactionByHash")
      result = jsonrpc::eth_getTransactionByHash(request, storage, state);
    else if (method == "eth_getTransactionByBlockHashAndIndex")
      result = jsonrpc::eth_getTransactionByBlockHashAndIndex(request, storage);
    else if (method == "eth_getTransactionByBlockNumberAndIndex")
      result = jsonrpc::eth_getTransactionByBlockNumberAndIndex(request, storage);
    else if (method == "eth_getTransactionReceipt")
      result = jsonrpc::eth_getTransactionReceipt(request, storage, state);
    else if (method == "eth_getUncleByBlockHashAndIndex")
      result = jsonrpc::eth_getUncleByBlockHashAndIndex();
    else if (method == "txpool_content")
      result = jsonrpc::txpool_content(request, state);
    else if (method == "debug_traceBlockByNumber")
      result = jsonrpc::debug_traceBlockByNumber(request, storage);
    else if (method == "debug_traceTransaction")
      result = jsonrpc::debug_traceTransaction(request, storage);
    else
      throw Error::methodNotAvailable(method);

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
