#include "httpparser.h"

std::string parseJsonRpcRequest(
  const std::string& body,
  const std::unique_ptr<State>& state,
  const std::unique_ptr<Storage>& storage,
  const std::unique_ptr<P2P::ManagerNormal>& p2p
) {
  json ret;
  uint64_t id = 0;
  try {
    json request = json::parse(body);
    id = request["id"].get<uint64_t>();
    if (!JsonRPC::Decoding::checkJsonRPCSpec(request)) {
      ret["error"]["code"] = -32600;
      ret["error"]["message"] = "Invalid Request, does not conform to JSON-RPC 2.0 spec";
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
        ret = JsonRPC::Encoding::web3_clientVersion();
        break;
      case JsonRPC::Methods::web3_sha3:
        ret = JsonRPC::Encoding::web3_sha3(JsonRPC::Decoding::web3_sha3(request));
        break;
      case JsonRPC::Methods::net_version:
        JsonRPC::Decoding::net_version(request);
        ret = JsonRPC::Encoding::net_version();
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
        ret = JsonRPC::Encoding::eth_protocolVersion();
        break;
      case JsonRPC::Methods::eth_getBlockByHash:
        ret = JsonRPC::Encoding::eth_getBlockByHash(
          JsonRPC::Decoding::eth_getBlockByHash(request),
          storage
        );
        break;
      case JsonRPC::Methods::eth_getBlockByNumber:
        ret = JsonRPC::Encoding::eth_getBlockByNumber(
          JsonRPC::Decoding::eth_getBlockByNumber(request, storage),
          storage
        );
        break;
      case JsonRPC::Methods::eth_getBlockTransactionCountByHash:
        ret = JsonRPC::Encoding::eth_getBlockTransactionCountByHash(
          JsonRPC::Decoding::eth_getBlockTransactionCountByHash(request),
          storage
        );
        break;
      case JsonRPC::Methods::eth_getBlockTransactionCountByNumber:
        ret = JsonRPC::Encoding::eth_getBlockTransactionCountByNumber(
          JsonRPC::Decoding::eth_getBlockTransactionCountByNumber(request, storage),
          storage
        );
        break;
      case JsonRPC::Methods::eth_chainId:
        JsonRPC::Decoding::eth_chainId(request);
        ret = JsonRPC::Encoding::eth_chainId();
        break;
      case JsonRPC::Methods::eth_syncing:
        JsonRPC::Decoding::eth_syncing(request);
        ret = JsonRPC::Encoding::eth_syncing();
        break;
      case JsonRPC::Methods::eth_coinbase:
        JsonRPC::Decoding::eth_coinbase(request);
        ret = JsonRPC::Encoding::eth_coinbase();
        break;
      case JsonRPC::Methods::eth_blockNumber:
        JsonRPC::Decoding::eth_blockNumber(request);
        ret = JsonRPC::Encoding::eth_blockNumber(storage);
        break;
      case JsonRPC::Methods::eth_call:
        throw std::runtime_error("eth_call not implemented");
        break;
      case JsonRPC::Methods::eth_estimateGas:
        ret = JsonRPC::Encoding::eth_estimateGas(JsonRPC::Decoding::eth_estimateGas(request));
        break;
      case JsonRPC::Methods::eth_gasPrice:
        JsonRPC::Decoding::eth_gasPrice(request);
        ret = JsonRPC::Encoding::eth_gasPrice();
        break;
      case JsonRPC::Methods::eth_getBalance:
        ret = JsonRPC::Encoding::eth_getBalance(
          JsonRPC::Decoding::eth_getBalance(request),
          state
        );
        break;
      case JsonRPC::Methods::eth_getTransactionCount:
        ret = JsonRPC::Encoding::eth_getTransactionCount(
          JsonRPC::Decoding::eth_getTransactionCount(request),
          state
        );
        break;
      case JsonRPC::Methods::eth_getTransactionByHash:
        ret = JsonRPC::Encoding::eth_getTransactionByHash(
            JsonRPC::Decoding::eth_getTransactionByHash(request),
            storage,
            state
          );
        break;
      case JsonRPC::Methods::eth_getTransactionByBlockHashAndIndex:
        ret = JsonRPC::Encoding::eth_getTransactionByBlockHashAndIndex(
            JsonRPC::Decoding::eth_getTransactionByBlockHashAndIndex(request),
            storage
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
            JsonRPC::Decoding::eth_getTransactionReceipt(request),
            storage
          );
        break;
      default:
        ret["error"]["code"] = -32601;
        ret["error"]["message"] = "Method not found";
        break;
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
  ret["id"] = id;
  return ret.dump();
}




template<class Body, class Allocator, class Send> void handle_request(
  beast::string_view docroot,
  http::request<Body, http::basic_fields<Allocator>>&& req,
  Send&& send, const std::unique_ptr<State>& state, const std::unique_ptr<Storage>& storage,
  const std::unique_ptr<P2P::ManagerNormal>& p2p
) {
  // Returns a bad request response
  const auto bad_request = [&req](beast::string_view why){
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    return res;
  };

  // Returns a not found response
  const auto not_found = [&req](beast::string_view target){
    http::response<http::string_body> res{http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(target) + "' was not found.";
    res.prepare_payload();
    return res;
  };

  // Returns a server error response
  const auto server_error = [&req](beast::string_view what) {
    http::response<http::string_body> res{http::status::internal_server_error, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + std::string(what) + "'";
    res.prepare_payload();
    return res;
  };

  // Make sure we can handle the method
  if (req.method() != http::verb::post && req.method() != http::verb::options) {
    return send(bad_request("Unknown HTTP-method"));
  }

  // Request path must be absolute and not contain ".."
  if (
    req.target().empty() || req.target()[0] != '/' ||
    req.target().find("..") != beast::string_view::npos
  ) {
    return send(bad_request("Illegal request-target"));
  }

  // Respond to OPTIONS, Metamask requests it
  if (req.method() == http::verb::options) {
    http::response<http::empty_body> res{http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::access_control_allow_origin, "*");
    res.set(http::field::access_control_allow_methods, "POST, GET");
    res.set(http::field::access_control_allow_headers, "content-type");
    res.set(http::field::accept_encoding, "deflate");
    res.set(http::field::accept_language, "en-US");
    res.keep_alive(req.keep_alive());
    return send(std::move(res));
  }

  std::string request = req.body();

  std::string answer = parseJsonRpcRequest(request, state, storage, p2p);
  http::response<http::string_body> res{http::status::ok, req.version()};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::access_control_allow_origin, "*");
  res.set(http::field::access_control_allow_methods, "POST, GET");
  res.set(http::field::access_control_allow_headers, "content-type");
  res.set(http::field::content_type, "application/json");
  res.set(http::field::connection, "keep-alive");
  res.set(http::field::strict_transport_security, "max-age=0");
  res.set(http::field::vary, "Origin");
  res.set(http::field::access_control_allow_credentials, "true");
  // res.body() = answer;
  res.keep_alive(req.keep_alive());
  res.prepare_payload();
  return send(std::move(res));
}

