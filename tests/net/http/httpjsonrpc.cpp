/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../blockchainwrapper.hpp" // blockchain.h -> (net/http/httpserver.h -> net/p2p/managernormal.h), consensus.h -> state.h -> dump.h -> (storage.h -> utils/options.h), utils/db.h -> utils.h

#include "../../src/net/http/jsonrpc/call.h"

std::string makeHTTPRequest(
  const std::string& reqBody, const std::string& host, const std::string& port,
  const std::string& target, const std::string& requestType, const std::string& contentType
) {
  std::string result = "";
  using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
  namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
  namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

  try {
    // Create context and load certificates into it
    boost::system::error_code ec;
    boost::asio::io_context ioc;

    tcp::resolver resolver{ioc};
    beast::tcp_stream stream{ioc};

    auto const results = resolver.resolve(host, port);

    // Connect and Handshake
    stream.connect(results);

    // Set up an HTTP POST/GET request message
    http::request<http::string_body> req{
      (requestType == "POST") ? http::verb::post : http::verb::get, target, 11
    };
    if (requestType == "GET") {
      req.set(http::field::host, host);
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      req.set(http::field::content_type, contentType);
      req.body() = reqBody;
      req.prepare_payload();
    } else if (requestType == "POST") {
      req.set(http::field::host, host);
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      req.set(http::field::accept, "application/json");
      req.set(http::field::content_type, contentType);
      req.body() = reqBody;
      req.prepare_payload();
    }

    // Send the HTTP request to the remote host
    http::write(stream, req);
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res);

    // Write only the body answer to output
    std::string body {
      boost::asio::buffers_begin(res.body().data()),
      boost::asio::buffers_end(res.body().data())
    };
    result = body;

    stream.socket().shutdown(tcp::socket::shutdown_both, ec);

    if (ec)
      throw boost::system::system_error{ec};
  } catch (std::exception const& e) {
    throw std::string("Error while doing HTTP Custom Request: ") + e.what();
  }
  return result;
}

const std::vector<Hash> validatorPrivKeysHttpJsonRpc {
  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

template <typename T> json requestMethod(std::string method, T params) {
  return json::parse(makeHTTPRequest(
    json({
      {"jsonrpc", "2.0"},
      {"id", 1},
      {"method", method},
      {"params", params}
    }).dump(),
    "127.0.0.1",
    std::to_string(9999), // Default port for HTTPJsonRPC
    "/",
    "POST",
    "application/json")
  );
}

namespace THTTPJsonRPC {
  TEST_CASE("HTTPJsonRPC Tests", "[net][http][jsonrpc]") {
    SECTION("checkJsonRPCSpec") {
      json ok = {{"jsonrpc", "2.0"}, {"method", "myMethod"}, {"params", json::array()}};
      json noJsonRpc = {{"method", "myMethod"}, {"params", json::array()}};
      json wrongJsonRpc = {{"jsonrpc", "1.0"}, {"method", "myMethod"}, {"params", json::array()}};
      json noMethod = {{"jsonrpc", "2.0"}, {"params", json::array()}};
      json wrongParams = {{"jsonrpc", "2.0"}, {"method", "myMethod"}, {"params", 12345}};
      jsonrpc::checkJsonRPCSpec(ok);
      REQUIRE_THROWS(jsonrpc::checkJsonRPCSpec(noJsonRpc));
      REQUIRE_THROWS(jsonrpc::checkJsonRPCSpec(wrongJsonRpc));
      REQUIRE_THROWS(jsonrpc::checkJsonRPCSpec(noMethod));
      REQUIRE_THROWS(jsonrpc::checkJsonRPCSpec(wrongParams));
    }

    SECTION("HTTPJsonRPC") {
      // One section to lead it all
      // Reasoning: we don't want to keep opening and closing everything per Section, just initialize once and run.
      std::string testDumpPath = Utils::getTestDumpPath();
      auto blockchainWrapper = initialize(validatorPrivKeysHttpJsonRpc, validatorPrivKeysHttpJsonRpc[0], 8080, true, testDumpPath + "/HTTPjsonRPC");

      // Make random transactions within a given block, we need to include requests for getting txs and blocks
      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      std::vector<TxBlock> transactions;
      for (auto &[privkey, val]: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        blockchainWrapper.state.addBalance(me);
        transactions.emplace_back(
          targetOfTransactions,
          me,
          Bytes(),
          8080,
          blockchainWrapper.state.getNativeNonce(me),
          1000000000000000000,
          1000000000,
          1000000000,
          21000,
          privkey
        );

        // Take note of expected balance and nonce
        val.first = blockchainWrapper.state.getNativeBalance(me) - (transactions.back().getMaxFeePerGas() * transactions.back().getGasLimit()) -
                    transactions.back().getValue();
        val.second = blockchainWrapper.state.getNativeNonce(me) + 1;
        targetExpectedValue += transactions.back().getValue();
      }

      // TODO: missing the following (as per coverage):
      // * eth_call
      // * eth_getLogs
      // * eth_getCode
      // * eth_getUncleByBlockHashAndIndex
      // * txpool_content
      // * debug_traceBlockByNumber
      // * debug_traceTransaction

      // We need to copy since createValidBlock will consume (move) the transactions
      auto transactionsCopy = transactions;

      auto newBestBlock = createValidBlock(validatorPrivKeysHttpJsonRpc, blockchainWrapper.state, blockchainWrapper.storage, std::move(transactionsCopy));

      REQUIRE(blockchainWrapper.state.tryProcessNextBlock(std::move(newBestBlock)) == BlockValidationStatus::valid);

      blockchainWrapper.http.start();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      blockchainWrapper.http.start(); // Attempt to start again, for coverage (runFuture is valid)

      json web3_clientVersionResponse = requestMethod("web3_clientVersion", json::array());

      REQUIRE(web3_clientVersionResponse["result"] == "BDK/cpp/linux_x86-64/0.2.0");

      json web3_sha3Response = requestMethod("web3_sha3", json::array({"0x68656c6c6f20776f726c64"}));

      REQUIRE(web3_sha3Response["result"] == "0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad");

      json net_versionResponse = requestMethod("net_version", json::array());

      REQUIRE(net_versionResponse["result"] == std::to_string(blockchainWrapper.options.getChainID()));

      json net_listeningResponse = requestMethod("net_listening", json::array());

      REQUIRE(net_listeningResponse["result"] == true);

      json net_peerCountResponse = requestMethod("net_peerCount", json::array());

      REQUIRE(net_peerCountResponse["result"] == "0x0");

      json eth_protocolVersionResponse = requestMethod("eth_protocolVersion", json::array());

      REQUIRE(eth_protocolVersionResponse["result"] == "0.2.0");

      json eth_getBlockByHashResponse = requestMethod("eth_getBlockByHash", json::array({newBestBlock.getHash().hex(true), true}));
      REQUIRE(eth_getBlockByHashResponse["result"]["number"] == "0x1");
      REQUIRE(eth_getBlockByHashResponse["result"]["hash"] == newBestBlock.getHash().hex(true));
      REQUIRE(eth_getBlockByHashResponse["result"]["parentHash"] == newBestBlock.getPrevBlockHash().hex(true));
      REQUIRE(eth_getBlockByHashResponse["result"]["nonce"] == "0x0000000000000000");
      REQUIRE(eth_getBlockByHashResponse["result"]["sha3Uncles"] == Hash().hex(true));
      REQUIRE(eth_getBlockByHashResponse["result"]["logsBloom"] == Hash().hex(true));
      REQUIRE(eth_getBlockByHashResponse["result"]["transactionsRoot"] == newBestBlock.getTxMerkleRoot().hex(true));
      REQUIRE(eth_getBlockByHashResponse["result"]["stateRoot"] == Hash().hex(true));
      REQUIRE(eth_getBlockByHashResponse["result"]["receiptsRoot"] == Hash().hex(true));
      REQUIRE(eth_getBlockByHashResponse["result"]["miner"] == Secp256k1::toAddress(newBestBlock.getValidatorPubKey()).hex(true));
      REQUIRE(eth_getBlockByHashResponse["result"]["difficulty"] == "0x1");
      REQUIRE(eth_getBlockByHashResponse["result"]["totalDifficulty"] == "0x1");
      REQUIRE(eth_getBlockByHashResponse["result"]["extraData"] == "0x0000000000000000000000000000000000000000000000000000000000000000");
      REQUIRE(eth_getBlockByHashResponse["result"]["size"] == Hex::fromBytes(Utils::uintToBytes(newBestBlock.serializeBlock().size()),true).forRPC());
      REQUIRE(eth_getBlockByHashResponse["result"]["gasLimit"] == Hex::fromBytes(Utils::uintToBytes(std::numeric_limits<uint64_t>::max()),true).forRPC());
      REQUIRE(eth_getBlockByHashResponse["result"]["gasUsed"] == Hex::fromBytes(Utils::uintToBytes(uint64_t(1000000000)),true).forRPC());
      REQUIRE(eth_getBlockByHashResponse["result"]["timestamp"] == Hex::fromBytes(Utils::uintToBytes((newBestBlock.getTimestamp()/1000000)),true).forRPC());
      REQUIRE(eth_getBlockByHashResponse["result"]["uncles"] == json::array());
      for (uint64_t i = 0; i < transactions.size(); ++i) {
        const auto &txJson = eth_getBlockByHashResponse["result"]["transactions"][i];
        const auto &tx = transactions[i];
        REQUIRE(txJson["blockHash"] == newBestBlock.getHash().hex(true));
        REQUIRE(txJson["blockNumber"] == "0x1");
        REQUIRE(txJson["hash"] == tx.hash().hex(true));
        REQUIRE(txJson["from"] == tx.getFrom().hex(true));
        REQUIRE(txJson["nonce"] == Hex::fromBytes(Utils::uintToBytes(tx.getNonce()),true).forRPC());
        REQUIRE(txJson["to"] == tx.getTo().hex(true));
        REQUIRE(txJson["gas"] == Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()),true).forRPC());
        REQUIRE(txJson["value"] == Hex::fromBytes(Utils::uintToBytes(tx.getValue()),true).forRPC());
        REQUIRE(txJson["input"] == Hex::fromBytes(tx.getData(), true));
        REQUIRE(txJson["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC());
        REQUIRE(txJson["v"] == Hex::fromBytes(Utils::uintToBytes(tx.getV()),true).forRPC());
        REQUIRE(txJson["r"] == Hex::fromBytes(Utils::uintToBytes(tx.getR()),true).forRPC());
        REQUIRE(txJson["s"] == Hex::fromBytes(Utils::uintToBytes(tx.getS()),true).forRPC());
      }

      json eth_getBlockByNumberResponse = requestMethod("eth_getBlockByNumber", json::array({"0x1", true}));
      REQUIRE(eth_getBlockByNumberResponse["result"]["number"] == "0x1");
      REQUIRE(eth_getBlockByNumberResponse["result"]["hash"] == newBestBlock.getHash().hex(true));
      REQUIRE(eth_getBlockByNumberResponse["result"]["parentHash"] == newBestBlock.getPrevBlockHash().hex(true));
      REQUIRE(eth_getBlockByNumberResponse["result"]["nonce"] == "0x0000000000000000");
      REQUIRE(eth_getBlockByNumberResponse["result"]["sha3Uncles"] == Hash().hex(true));
      REQUIRE(eth_getBlockByNumberResponse["result"]["logsBloom"] == Hash().hex(true));
      REQUIRE(eth_getBlockByNumberResponse["result"]["transactionsRoot"] == newBestBlock.getTxMerkleRoot().hex(true));
      REQUIRE(eth_getBlockByNumberResponse["result"]["stateRoot"] == Hash().hex(true));
      REQUIRE(eth_getBlockByNumberResponse["result"]["receiptsRoot"] == Hash().hex(true));
      REQUIRE(eth_getBlockByNumberResponse["result"]["miner"] == Secp256k1::toAddress(newBestBlock.getValidatorPubKey()).hex(true));
      REQUIRE(eth_getBlockByNumberResponse["result"]["difficulty"] == "0x1");
      REQUIRE(eth_getBlockByNumberResponse["result"]["totalDifficulty"] == "0x1");
      REQUIRE(eth_getBlockByNumberResponse["result"]["extraData"] == "0x0000000000000000000000000000000000000000000000000000000000000000");
      REQUIRE(eth_getBlockByNumberResponse["result"]["size"] == Hex::fromBytes(Utils::uintToBytes(newBestBlock.serializeBlock().size()),true).forRPC());
      REQUIRE(eth_getBlockByNumberResponse["result"]["gasLimit"] == Hex::fromBytes(Utils::uintToBytes(std::numeric_limits<uint64_t>::max()),true).forRPC());
      REQUIRE(eth_getBlockByNumberResponse["result"]["gasUsed"] == Hex::fromBytes(Utils::uintToBytes(uint64_t(1000000000)),true).forRPC());
      REQUIRE(eth_getBlockByNumberResponse["result"]["timestamp"] == Hex::fromBytes(Utils::uintToBytes(newBestBlock.getTimestamp()/1000000),true).forRPC());
      REQUIRE(eth_getBlockByNumberResponse["result"]["uncles"] == json::array());
      for (uint64_t i = 0; i < transactions.size(); ++i) {
        const auto &txJson = eth_getBlockByNumberResponse["result"]["transactions"][i];
        const auto &tx = transactions[i];
        REQUIRE(txJson["blockHash"] == newBestBlock.getHash().hex(true));
        REQUIRE(txJson["blockNumber"] == "0x1");
        REQUIRE(txJson["hash"] == tx.hash().hex(true));
        REQUIRE(txJson["from"] == tx.getFrom().hex(true));
        REQUIRE(txJson["nonce"] == Hex::fromBytes(Utils::uintToBytes(tx.getNonce()),true).forRPC());
        REQUIRE(txJson["to"] == tx.getTo().hex(true));
        REQUIRE(txJson["gas"] == Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()),true).forRPC());
        REQUIRE(txJson["value"] == Hex::fromBytes(Utils::uintToBytes(tx.getValue()),true).forRPC());
        REQUIRE(txJson["input"] == Hex::fromBytes(tx.getData(), true));
        REQUIRE(txJson["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC());
        REQUIRE(txJson["v"] == Hex::fromBytes(Utils::uintToBytes(tx.getV()),true).forRPC());
        REQUIRE(txJson["r"] == Hex::fromBytes(Utils::uintToBytes(tx.getR()),true).forRPC());
        REQUIRE(txJson["s"] == Hex::fromBytes(Utils::uintToBytes(tx.getS()),true).forRPC());
      }

      json eth_getBlockTransactionCountByHashResponse = requestMethod("eth_getBlockTransactionCountByHash", json::array({newBestBlock.getHash().hex(true)}));
      REQUIRE(eth_getBlockTransactionCountByHashResponse["result"] == Hex::fromBytes(Utils::uintToBytes(uint64_t(transactions.size())),true).forRPC());

      json eth_getBlockTransactionCountByNumberResponse = requestMethod("eth_getBlockTransactionCountByNumber", json::array({"0x1"}));
      REQUIRE(eth_getBlockTransactionCountByNumberResponse["result"] == Hex::fromBytes(Utils::uintToBytes(uint64_t(transactions.size())),true).forRPC());

      json eth_chainIdResponse = requestMethod("eth_chainId", json::array());
      REQUIRE(eth_chainIdResponse["result"] == "0x1f90");

      json eth_syncingResponse = requestMethod("eth_syncing", json::array());
      REQUIRE(eth_syncingResponse["result"] == false);

      json eth_coinbaseResponse = requestMethod("eth_coinbase", json::array());
      REQUIRE(eth_coinbaseResponse["result"] == Address(Hex::toBytes("0x1531bfdf7d48555a0034e4647fa46d5a04c002c3")).hex(true));

      json eth_blockNumberResponse = requestMethod("eth_blockNumber", json::array());
      REQUIRE(eth_blockNumberResponse["result"] == "0x1");

      json eth_estimateGasResponse = requestMethod("eth_estimateGas", json::array({json::object({
        {"from", blockchainWrapper.options.getChainOwner().hex(true) },
        {"to", "0xaaA85B2B2bD0bFdF6Bc5D0d61B6192c53818567b"},
        {"gas", "0xffffff"},
        {"gasPrice", "0x1"},
        {"value", "0x1"},
        {"data", "0x1"},
      }), "latest"}));

      REQUIRE(eth_estimateGasResponse["result"] == "0x5208");

      json eth_gasPriceResponse = requestMethod("eth_gasPrice", json::array());
      REQUIRE(eth_gasPriceResponse["result"] == "0x9502f900");

      for (const auto& [privKey, accInfo] : randomAccounts) {
        const auto& [ balance, nonce] = accInfo;
        json eth_getBalanceResponse = requestMethod("eth_getBalance", json::array({Secp256k1::toAddress(Secp256k1::toUPub(privKey)).hex(true), "latest"}));
        REQUIRE(eth_getBalanceResponse["result"] == Hex::fromBytes(Utils::uintToBytes(balance),true).forRPC());
        json eth_getTransactionCountResponse = requestMethod("eth_getTransactionCount", json::array({Secp256k1::toAddress(Secp256k1::toUPub(privKey)).hex(true), "latest"}));
        REQUIRE(eth_getTransactionCountResponse["result"] == Hex::fromBytes(Utils::uintToBytes(nonce),true).forRPC());
      }

      auto txToSend = TxBlock(
        targetOfTransactions,
        Secp256k1::toAddress(Secp256k1::toUPub(randomAccounts.begin()->first)),
        Bytes(),
        8080,
        blockchainWrapper.state.getNativeNonce(Secp256k1::toAddress(Secp256k1::toUPub(randomAccounts.begin()->first))),
        1000000000000000000,
        1000000000,
        1000000000,
        21000,
        randomAccounts.begin()->first
      );

      json eth_sendRawTransactionResponse = requestMethod("eth_sendRawTransaction", json::array({Hex::fromBytes(txToSend.rlpSerialize(),true).forRPC()}));
      REQUIRE(eth_sendRawTransactionResponse["result"] == txToSend.hash().hex(true));

      for (uint64_t i = 0; i < transactions.size(); ++i) {
        json eth_getTransactionByHash = requestMethod("eth_getTransactionByHash", json::array({transactions[i].hash().hex(true)}));
        REQUIRE(eth_getTransactionByHash["result"]["blockHash"] == newBestBlock.getHash().hex(true));
        REQUIRE(eth_getTransactionByHash["result"]["blockNumber"] == "0x1");
        REQUIRE(eth_getTransactionByHash["result"]["from"] == transactions[i].getFrom().hex(true));
        REQUIRE(eth_getTransactionByHash["result"]["gas"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getMaxFeePerGas()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["hash"] == transactions[i].hash().hex(true));
        REQUIRE(eth_getTransactionByHash["result"]["input"] == Hex::fromBytes(transactions[i].getData(),true));
        REQUIRE(eth_getTransactionByHash["result"]["nonce"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getNonce()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["to"] == transactions[i].getTo().hex(true));
        REQUIRE(eth_getTransactionByHash["result"]["transactionIndex"] == Hex::fromBytes(Utils::uintToBytes(i),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["value"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getValue()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["s"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getS()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["v"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getV()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["r"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getR()),true).forRPC());
      }

      for (uint64_t i = 0; i < transactions.size(); ++i) {
        json eth_getTransactionByBlockHashAndIndex = requestMethod("eth_getTransactionByBlockHashAndIndex", json::array({newBestBlock.getHash().hex(true), Hex::fromBytes(Utils::uintToBytes(i),true).forRPC()}));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["blockHash"] == newBestBlock.getHash().hex(true));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["blockNumber"] == "0x1");
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["from"] == transactions[i].getFrom().hex(true));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["gas"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getMaxFeePerGas()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["hash"] == transactions[i].hash().hex(true));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["input"] == Hex::fromBytes(transactions[i].getData(), true));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["nonce"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getNonce()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["to"] == transactions[i].getTo().hex(true));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["transactionIndex"] == Hex::fromBytes(Utils::uintToBytes(i),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["value"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getValue()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["s"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getS()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["v"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getV()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["r"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getR()),true).forRPC());
      }

      for (uint64_t i = 0; i < transactions.size(); ++i) {
        json eth_getTransactionByBlockNumberAndIndexResponse = requestMethod("eth_getTransactionByBlockNumberAndIndex", json::array({"0x1", Hex::fromBytes(Utils::uintToBytes(i),true).forRPC()}));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["blockHash"] == newBestBlock.getHash().hex(true));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["blockNumber"] == "0x1");
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["from"] == transactions[i].getFrom().hex(true));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["gas"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getMaxFeePerGas()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["hash"] == transactions[i].hash().hex(true));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["input"] == Hex::fromBytes(transactions[i].getData(),true));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["nonce"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getNonce()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["to"] == transactions[i].getTo().hex(true));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["transactionIndex"] == Hex::fromBytes(Utils::uintToBytes(i),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["value"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getValue()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["s"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getS()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["v"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getV()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["r"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getR()),true).forRPC());
      }

      for (uint64_t i = 0; i < transactions.size(); ++i) {
        json eth_getTransactionReceiptResponse = requestMethod("eth_getTransactionReceipt", json::array({transactions[i].hash().hex(true)}));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["transactionHash"] == transactions[i].hash().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["transactionIndex"] == Hex::fromBytes(Utils::uintToBytes(i),true).forRPC());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["blockHash"] == newBestBlock.getHash().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["blockNumber"] == "0x1");
        REQUIRE(eth_getTransactionReceiptResponse["result"]["from"] == transactions[i].getFrom().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["to"] == transactions[i].getTo().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["cumulativeGasUsed"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()), true).forRPC());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["effectiveGasPrice"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getMaxFeePerGas()), true).forRPC());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["gasUsed"] == "0x5208");
        REQUIRE(eth_getTransactionReceiptResponse["result"]["contractAddress"] == json::value_t::null);
        REQUIRE(eth_getTransactionReceiptResponse["result"]["logs"] == json::array());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["logsBloom"] == Hash().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["type"] == "0x2");
        REQUIRE(eth_getTransactionReceiptResponse["result"]["status"] == "0x1");
      }

      json eth_feeHistoryResponse = requestMethod("eth_feeHistory", json::array({ "0x2", "latest" }));
      REQUIRE(eth_feeHistoryResponse["result"]["baseFeePerGas"][0] == "0x9502f900");
      REQUIRE(eth_feeHistoryResponse["result"]["baseFeePerGas"][1] == "0x9502f900");
      REQUIRE(eth_feeHistoryResponse["result"]["gasUsedRatio"][0] == 1.0); // TODO: properly compare float pointing values
      REQUIRE(eth_feeHistoryResponse["result"]["gasUsedRatio"][1] == 1.0);
      REQUIRE(eth_feeHistoryResponse["result"]["oldestBlock"] == "0x0");

      eth_feeHistoryResponse = requestMethod("eth_feeHistory", json::array({ "0x1", "0x0" }));
      REQUIRE(eth_feeHistoryResponse["result"]["baseFeePerGas"][0] == "0x9502f900");
      REQUIRE(eth_feeHistoryResponse["result"]["baseFeePerGas"][1] == "0x9502f900");
      REQUIRE(eth_feeHistoryResponse["result"]["gasUsedRatio"][0] == 1.0); // TODO: properly compare float pointing values
      REQUIRE(eth_feeHistoryResponse["result"]["oldestBlock"] == "0x0");

      // Last part - cover the catch cases
      // Invalid JSON id type
      json wrongId = {{"jsonrpc", "2.0"}, {"id", json::array()}, {"method", "web3_clientVersion"}, {"params", json::array()}};
      json idErr = json::parse(makeHTTPRequest(
        wrongId.dump(), "127.0.0.1", std::to_string(9999), "/", "POST", "application/json"
      ));
      REQUIRE(idErr.contains("error"));
      REQUIRE(idErr["error"]["code"] == -32603);
      REQUIRE(idErr["error"]["message"] == "Internal error: Invalid id type");
      // Invalid method call
      json methodErr = requestMethod("lololol", json::array());
      REQUIRE(methodErr.contains("error"));
      REQUIRE(methodErr["error"]["code"] == -32601);
      REQUIRE(methodErr["error"]["message"] == "Method \"lololol\" not found/available");
    }
  }
}

