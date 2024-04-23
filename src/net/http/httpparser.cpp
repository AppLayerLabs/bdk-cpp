/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "httpparser.h"

std::string parseJsonRpcRequest(
  const std::string& body,
  State& state,
  const Storage& storage,
  P2P::ManagerNormal& p2p,
  const Options& options
) {
  json ret;
  uint64_t id = 0;
  try {
    Utils::safePrint("HTTP Request: " + body);
    json request = json::parse(body);
    Utils::safePrint("HTTP Request Parsed!");
    if (!JsonRPC::Decoding::checkJsonRPCSpec(request)) {
      ret["error"]["code"] = -32600;
      ret["error"]["message"] = "Invalid request - does not conform to JSON-RPC 2.0 spec";
      return ret.dump();
    }

    auto RequestMethod = JsonRPC::Decoding::getMethod(request);
    switch (RequestMethod) {
      case JsonRPC::Methods::invalid:
        ret["error"]["code"] = -32601;
        ret["error"]["message"] = "Method not found";
        break;
      case JsonRPC::Methods::web3_clientVersion:
        JsonRPC::Decoding::web3_clientVersion(request);
        ret = JsonRPC::Encoding::web3_clientVersion(options);
        break;
      case JsonRPC::Methods::web3_sha3:
        ret = JsonRPC::Encoding::web3_sha3(JsonRPC::Decoding::web3_sha3(request));
        break;
      case JsonRPC::Methods::net_version:
        JsonRPC::Decoding::net_version(request);
        ret = JsonRPC::Encoding::net_version(options);
        break;
      case JsonRPC::Methods::net_listening:
        JsonRPC::Decoding::net_listening(request);
        ret = JsonRPC::Encoding::net_listening();
        break;
      case JsonRPC::Methods::net_peerCount:
        JsonRPC::Decoding::net_peerCount(request);
        ret = JsonRPC::Encoding::net_peerCount(p2p);
        break;
      case JsonRPC::Methods::eth_protocolVersion:
        JsonRPC::Decoding::eth_protocolVersion(request);
        ret = JsonRPC::Encoding::eth_protocolVersion(options);
        break;
      case JsonRPC::Methods::eth_getBlockByHash:
        ret = JsonRPC::Encoding::eth_getBlockByHash(
          JsonRPC::Decoding::eth_getBlockByHash(request), storage
        );
        break;
      case JsonRPC::Methods::eth_getBlockByNumber:
        ret = JsonRPC::Encoding::eth_getBlockByNumber(
          JsonRPC::Decoding::eth_getBlockByNumber(request, storage), storage
        );
        break;
      case JsonRPC::Methods::eth_getBlockTransactionCountByHash:
        ret = JsonRPC::Encoding::eth_getBlockTransactionCountByHash(
          JsonRPC::Decoding::eth_getBlockTransactionCountByHash(request), storage
        );
        break;
      case JsonRPC::Methods::eth_getBlockTransactionCountByNumber:
        ret = JsonRPC::Encoding::eth_getBlockTransactionCountByNumber(
          JsonRPC::Decoding::eth_getBlockTransactionCountByNumber(request, storage), storage
        );
        break;
      case JsonRPC::Methods::eth_chainId:
        JsonRPC::Decoding::eth_chainId(request);
        ret = JsonRPC::Encoding::eth_chainId(options);
        break;
      case JsonRPC::Methods::eth_syncing:
        JsonRPC::Decoding::eth_syncing(request);
        ret = JsonRPC::Encoding::eth_syncing();
        break;
      case JsonRPC::Methods::eth_coinbase:
        JsonRPC::Decoding::eth_coinbase(request);
        ret = JsonRPC::Encoding::eth_coinbase(options);
        break;
      case JsonRPC::Methods::eth_blockNumber:
        JsonRPC::Decoding::eth_blockNumber(request);
        ret = JsonRPC::Encoding::eth_blockNumber(storage);
        break;
      case JsonRPC::Methods::eth_call:
        {
          // We actually need to allocate the Bytes object containing the call data
          // As evmc_message only holds a *pointer* to the data
          Bytes fullData;
          ret = JsonRPC::Encoding::eth_call(
            JsonRPC::Decoding::eth_call(request, storage, fullData), state
          );
        }
        break;
      case JsonRPC::Methods::eth_estimateGas:
        {
          // Same as before
          Bytes fullData;
          ret = JsonRPC::Encoding::eth_estimateGas(
            JsonRPC::Decoding::eth_estimateGas(request, storage, fullData), state
          );
        }
        break;
      case JsonRPC::Methods::eth_gasPrice:
        JsonRPC::Decoding::eth_gasPrice(request);
        ret = JsonRPC::Encoding::eth_gasPrice();
        break;
      case JsonRPC::Methods::eth_getLogs:
        ret = JsonRPC::Encoding::eth_getLogs(
          JsonRPC::Decoding::eth_getLogs(request, storage), state
        );
        break;
      case JsonRPC::Methods::eth_getBalance:
        ret = JsonRPC::Encoding::eth_getBalance(
          JsonRPC::Decoding::eth_getBalance(request, storage), state
        );
        break;
      case JsonRPC::Methods::eth_getTransactionCount:
        ret = JsonRPC::Encoding::eth_getTransactionCount(
          JsonRPC::Decoding::eth_getTransactionCount(request, storage), state
        );
        break;
      case JsonRPC::Methods::eth_getCode:
        ret = JsonRPC::Encoding::eth_getCode(
          JsonRPC::Decoding::eth_getCode(request, storage)
        );
        break;
      case JsonRPC::Methods::eth_sendRawTransaction:
        ret = JsonRPC::Encoding::eth_sendRawTransaction(
          JsonRPC::Decoding::eth_sendRawTransaction(request, options.getChainID()),
          state, p2p
        );
        break;
      case JsonRPC::Methods::eth_getTransactionByHash:
        ret = JsonRPC::Encoding::eth_getTransactionByHash(
          JsonRPC::Decoding::eth_getTransactionByHash(request),
          storage, state
        );
        break;
      case JsonRPC::Methods::eth_getTransactionByBlockHashAndIndex:
        ret = JsonRPC::Encoding::eth_getTransactionByBlockHashAndIndex(
          JsonRPC::Decoding::eth_getTransactionByBlockHashAndIndex(request), storage
        );
        break;
      case JsonRPC::Methods::eth_getTransactionByBlockNumberAndIndex:
        ret = JsonRPC::Encoding::eth_getTransactionByBlockNumberAndIndex(
          JsonRPC::Decoding::eth_getTransactionByBlockNumberAndIndex(request, storage),
          storage
        );
        break;
      case JsonRPC::Methods::eth_getTransactionReceipt:
        ret = JsonRPC::Encoding::eth_getTransactionReceipt(
          JsonRPC::Decoding::eth_getTransactionReceipt(request), storage, state
        );
        break;
      default:
        ret["error"]["code"] = -32601;
        ret["error"]["message"] = "Method not found";
        break;
    }
    Utils::safePrint("HTTP Response: " + ret.dump());
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
  Utils::safePrint("Properly returning...");
  // Set back to the original id
  return ret.dump();
}

