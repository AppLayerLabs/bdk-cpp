#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/options.h"
#include "../../src/net/p2p/managernormal.h"
#include "../../src/net/http/httpserver.h"
#include "../../src/core/state.h"
#include "../../src/core/storage.h"

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


const std::vector<Hash> validatorPrivKeys {
  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),
  Hash(Hex::toBytes("0xb254f12b4ca3f0120f305cabf1188fe74f0bd38e58c932a3df79c4c55df8fa66")),
  Hash(Hex::toBytes("0x8a52bb289198f0bcf141688a8a899bf1f04a02b003a8b1aa3672b193ce7930da")),
  Hash(Hex::toBytes("0x9048f5e80549e244b7899e85a4ef69512d7d68613a3dba828266736a580e7745")),
  Hash(Hex::toBytes("0x0b6f5ad26f6eb79116da8c98bed5f3ed12c020611777d4de94c3c23b9a03f739")),
  Hash(Hex::toBytes("0xa69eb3a3a679e7e4f6a49fb183fb2819b7ab62f41c341e2e2cc6288ee22fbdc7")),
  Hash(Hex::toBytes("0xd9b0613b7e4ccdb0f3a5ab0956edeb210d678db306ab6fae1e2b0c9ebca1c2c5")),
  Hash(Hex::toBytes("0x426dc06373b694d8804d634a0fd133be18e4e9bcbdde099fce0ccf3cb965492f"))
};

// This creates a valid block given the state within the rdPoS class.
// Should not be used during network/thread testing, as it will automatically sign all TxValidator transactions within the block
// And that is not the purpose of network/thread testing.
// Definition from state.cpp, when linking, the compiler should find the function.
Block createValidBlock(std::unique_ptr<rdPoS>& rdpos, std::unique_ptr<Storage>& storage, const std::vector<TxBlock>& txs = {});

// We initialize the blockchain database
// To make sure that if the genesis is changed within the main source code
// The tests will still work, as tests uses own genesis block.
void initialize(std::unique_ptr<DB>& db,
                std::unique_ptr<Storage>& storage,
                std::unique_ptr<P2P::ManagerNormal>& p2p,
                std::unique_ptr<rdPoS>& rdpos,
                std::unique_ptr<State>& state,
                std::unique_ptr<HTTPServer>& httpServer,
                std::unique_ptr<Options>& options,
                PrivKey validatorKey,
                uint64_t serverPort,
                uint64_t httpServerPort,
                bool clearDb,
                std::string folderPath) {
  std::string dbName = folderPath + "/db";
  if (clearDb) {
    if (std::filesystem::exists(dbName)) {
      std::filesystem::remove_all(dbName);
    }
  }
  db = std::make_unique<DB>(dbName);
  if (clearDb) {
    Block genesis(Hash(Utils::uint256ToBytes(0)), 1678887537000000, 0);

    // Genesis Keys:
    // Private: 0xe89ef6409c467285bcae9f80ab1cfeb348  Hash(Hex::toBytes("0x0a0415d68a5ec2df57aab65efc2a7231b59b029bae7ff1bd2e40df9af96418c8")),7cfe61ab28fb7d36443e1daa0c2867
    // Address: 0x00dead00665771855a34155f5e7405489df2c3c6
    genesis.finalize(PrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867")),1678887538000000);
    db->put(Utils::stringToBytes("latest"), genesis.serializeBlock(), DBPrefix::blocks);
    db->put(Utils::uint64ToBytes(genesis.getNHeight()), genesis.hash().get(), DBPrefix::blockHeightMaps);
    db->put(genesis.hash().get(), genesis.serializeBlock(), DBPrefix::blocks);

    // Populate rdPoS DB with unique rdPoS, not default.
    for (uint64_t i = 0; i < validatorPrivKeys.size(); ++i) {
      db->put(Utils::uint64ToBytes(i), Address(Secp256k1::toAddress(Secp256k1::toUPub(validatorPrivKeys[i]))).get(),
              DBPrefix::rdPoS);
    }
    // Populate State DB with one address.
    /// Initialize with 0x00dead00665771855a34155f5e7405489df2c3c6 with nonce 0.
    Address dev1(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"));
    /// See ~State for encoding
    uint256_t desiredBalance("1000000000000000000000");
    Bytes value = Utils::uintToBytes(Utils::bytesRequired(desiredBalance));
    Utils::appendBytes(value, Utils::uintToBytes(desiredBalance));
    value.insert(value.end(), 0x00);
    db->put(dev1.get(), value, DBPrefix::nativeAccounts);
  }
  std::vector<std::pair<boost::asio::ip::address, uint64_t>> discoveryNodes;
  options = std::make_unique<Options>(
    folderPath,
    "OrbiterSDK/cpp/linux_x86-64/0.1.2",
    1,
    8080,
    serverPort,
    httpServerPort,
    discoveryNodes
  );
  storage = std::make_unique<Storage>(db, options);
  p2p = std::make_unique<P2P::ManagerNormal>(boost::asio::ip::address::from_string("127.0.0.1"), rdpos, options, storage, state);
  rdpos = std::make_unique<rdPoS>(db, storage, p2p, options, state);
  state = std::make_unique<State>(db, storage, rdpos, p2p, options);
  httpServer = std::make_unique<HTTPServer>(state, storage, p2p, options);
}

template <typename T>
json requestMethod(std::string method, T params) {
  return json::parse(makeHTTPRequest(
    json({
           {"jsonrpc", "2.0"},
           {"id", 1},
           {"method", method},
           {"params", params}
         }).dump(),
    "127.0.0.1",
    std::to_string(8081),
    "/",
    "POST",
    "application/json"));
}

namespace THTTPJsonRPC{
  TEST_CASE("HTTPJsonRPC Tests", "[net][http][jsonrpc]") {
    SECTION("HTTPJsonRPC") {
      /// One section to lead it all
      /// Reasoning: we don't want to keep opening and closing everything per Section, just initialize once and run.
      std::unique_ptr<DB> db;
      std::unique_ptr<Storage> storage;
      std::unique_ptr<P2P::ManagerNormal> p2p;
      std::unique_ptr<rdPoS> rdpos;
      std::unique_ptr<State> state;
      std::unique_ptr<HTTPServer> httpServer;
      std::unique_ptr<Options> options;
      std::string testDumpPath = Utils::getTestDumpPath();
      initialize(db, storage, p2p, rdpos, state, httpServer, options, validatorPrivKeys[0], 8080, 8081, true, testDumpPath + "/HTTPjsonRPC");


      /// Make random transactions within a given block, we need to include requests for getting txs and blocks
      Address targetOfTransactions = Address(Utils::randBytes(20));
      uint256_t targetExpectedValue = 0;
      std::unordered_map<PrivKey, std::pair<uint256_t, uint64_t>, SafeHash> randomAccounts;
      for (uint64_t i = 0; i < 100; ++i) {
        randomAccounts.insert({PrivKey(Utils::randBytes(32)), std::make_pair(0, 0)});
      }

      std::vector<TxBlock> transactions;
      for (auto &[privkey, val]: randomAccounts) {
        Address me = Secp256k1::toAddress(Secp256k1::toUPub(privkey));
        state->addBalance(me);
        transactions.emplace_back(
          targetOfTransactions,
          me,
          Bytes(),
          8080,
          state->getNativeNonce(me),
          1000000000000000000,
          21000,
          1000000000,
          1000000000,
          privkey
        );

        /// Take note of expected balance and nonce
        val.first = state->getNativeBalance(me) - (transactions.back().getMaxFeePerGas() * transactions.back().getGasLimit()) -
                    transactions.back().getValue();
        val.second = state->getNativeNonce(me) + 1;
        targetExpectedValue += transactions.back().getValue();
      }

      auto newBestBlock = createValidBlock(rdpos, storage, transactions);

      REQUIRE(state->validateNextBlock(newBestBlock));

      state->processNextBlock(Block(newBestBlock));

      httpServer->start();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      json web3_clientVersionResponse = requestMethod("web3_clientVersion", json::array());

      REQUIRE(web3_clientVersionResponse["result"] == "OrbiterSDK/cpp/linux_x86-64/0.1.2");

      json web3_sha3Response = requestMethod("web3_sha3", json::array({"0x68656c6c6f20776f726c64"}));

      REQUIRE(web3_sha3Response["result"] == "0x47173285a8d7341e5e972fc677286384f802f8ef42a5ec5f03bbfa254cb01fad");

      json net_versionResponse = requestMethod("net_version", json::array());

      REQUIRE(net_versionResponse["result"] == "1");

      json net_listeningResponse = requestMethod("net_listening", json::array());

      REQUIRE(net_listeningResponse["result"] == true);

      json net_peerCountResponse = requestMethod("net_peerCount", json::array());

      REQUIRE(net_peerCountResponse["result"] == "0x0");

      json eth_protocolVersionResponse = requestMethod("eth_protocolVersion", json::array());

      REQUIRE(eth_protocolVersionResponse["result"] == "0.1.2");

      json eth_getBlockByHashResponse = requestMethod("eth_getBlockByHash", json::array({newBestBlock.hash().hex(true), true}));
      REQUIRE(eth_getBlockByHashResponse["result"]["number"] == "0x1");
      REQUIRE(eth_getBlockByHashResponse["result"]["hash"] == newBestBlock.hash().hex(true));
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
      REQUIRE(eth_getBlockByHashResponse["result"]["timestamp"] == Hex::fromBytes(Utils::uintToBytes(newBestBlock.getTimestamp()),true).forRPC());
      REQUIRE(eth_getBlockByHashResponse["result"]["uncles"] == json::array());
      for (uint64_t i = 0; i < transactions.size(); ++i) {
        const auto &txJson = eth_getBlockByHashResponse["result"]["transactions"][i];
        const auto &tx = transactions[i];
        REQUIRE(txJson["type"] == "0x0");
        REQUIRE(txJson["nonce"] == Hex::fromBytes(Utils::uintToBytes(tx.getNonce()),true).forRPC());
        REQUIRE(txJson["to"] == tx.getTo().hex(true));
        REQUIRE(txJson["gas"] == Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()),true).forRPC());
        REQUIRE(txJson["value"] == Hex::fromBytes(Utils::uintToBytes(tx.getValue()),true).forRPC());
        REQUIRE(txJson["input"] == Hex::fromBytes(tx.getData(),true).forRPC());
        REQUIRE(txJson["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC());
        REQUIRE(txJson["chainId"] == Hex::fromBytes(Utils::uintToBytes(tx.getChainId()),true).forRPC());
        REQUIRE(txJson["v"] == Hex::fromBytes(Utils::uintToBytes(tx.getV()),true).forRPC());
        REQUIRE(txJson["r"] == Hex::fromBytes(Utils::uintToBytes(tx.getR()),true).forRPC());
        REQUIRE(txJson["s"] == Hex::fromBytes(Utils::uintToBytes(tx.getS()),true).forRPC());
      }

      json eth_getBlockByNumberResponse = requestMethod("eth_getBlockByNumber", json::array({"0x1", true}));
      REQUIRE(eth_getBlockByNumberResponse["result"]["number"] == "0x1");
      REQUIRE(eth_getBlockByNumberResponse["result"]["hash"] == newBestBlock.hash().hex(true));
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
      REQUIRE(eth_getBlockByNumberResponse["result"]["timestamp"] == Hex::fromBytes(Utils::uintToBytes(newBestBlock.getTimestamp()),true).forRPC());
      REQUIRE(eth_getBlockByNumberResponse["result"]["uncles"] == json::array());
      for (uint64_t i = 0; i < transactions.size(); ++i) {
        const auto &txJson = eth_getBlockByNumberResponse["result"]["transactions"][i];
        const auto &tx = transactions[i];
        REQUIRE(txJson["type"] == "0x0");
        REQUIRE(txJson["nonce"] == Hex::fromBytes(Utils::uintToBytes(tx.getNonce()),true).forRPC());
        REQUIRE(txJson["to"] == tx.getTo().hex(true));
        REQUIRE(txJson["gas"] == Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()),true).forRPC());
        REQUIRE(txJson["value"] == Hex::fromBytes(Utils::uintToBytes(tx.getValue()),true).forRPC());
        REQUIRE(txJson["input"] == Hex::fromBytes(tx.getData(),true).forRPC());
        REQUIRE(txJson["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC());
        REQUIRE(txJson["chainId"] == Hex::fromBytes(Utils::uintToBytes(tx.getChainId()),true).forRPC());
        REQUIRE(txJson["v"] == Hex::fromBytes(Utils::uintToBytes(tx.getV()),true).forRPC());
        REQUIRE(txJson["r"] == Hex::fromBytes(Utils::uintToBytes(tx.getR()),true).forRPC());
        REQUIRE(txJson["s"] == Hex::fromBytes(Utils::uintToBytes(tx.getS()),true).forRPC());
      }

      json eth_getBlockTransactionCountByHashResponse = requestMethod("eth_getBlockTransactionCountByHash", json::array({newBestBlock.hash().hex(true)}));
      REQUIRE(eth_getBlockTransactionCountByHashResponse["result"] == Hex::fromBytes(Utils::uintToBytes(uint64_t(transactions.size())),true).forRPC());

      json eth_getBlockTransactionCountByNumberResponse = requestMethod("eth_getBlockTransactionCountByNumber", json::array({"0x1"}));
      REQUIRE(eth_getBlockTransactionCountByNumberResponse["result"] == Hex::fromBytes(Utils::uintToBytes(uint64_t(transactions.size())),true).forRPC());

      json eth_chainIdResponse = requestMethod("eth_chainId", json::array());
      REQUIRE(eth_chainIdResponse["result"] == "0x1f90");

      json eth_syncingResponse = requestMethod("eth_syncing", json::array());
      REQUIRE(eth_syncingResponse["result"] == false);

      json eth_coinbaseResponse = requestMethod("eth_coinbase", json::array());
      REQUIRE(eth_coinbaseResponse["result"] == Address().hex(true));

      json eth_blockNumberResponse = requestMethod("eth_blockNumber", json::array());
      REQUIRE(eth_blockNumberResponse["result"] == "0x1");

      /// TODO: eth_call
      json eth_estimateGasResponse = requestMethod("eth_estimateGas", json::array({json::object({
        {"from", Address().hex(true)},
        {"to", Address().hex(true)},
        {"gas", "0x1"},
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
        state->getNativeNonce(Secp256k1::toAddress(Secp256k1::toUPub(randomAccounts.begin()->first))),
        1000000000000000000,
        21000,
        1000000000,
        1000000000,
        randomAccounts.begin()->first
      );

      json eth_sendRawTransactionResponse = requestMethod("eth_sendRawTransaction", json::array({Hex::fromBytes(txToSend.rlpSerialize(),true).forRPC()}));
      REQUIRE(eth_sendRawTransactionResponse["result"] == txToSend.hash().hex(true));

      for (uint64_t i = 0; i < transactions.size(); ++i) {
        json eth_getTransactionByHash = requestMethod("eth_getTransactionByHash", json::array({transactions[i].hash().hex(true)}));
        REQUIRE(eth_getTransactionByHash["result"]["blockHash"] == newBestBlock.hash().hex(true));
        REQUIRE(eth_getTransactionByHash["result"]["blockNumber"] == "0x1");
        REQUIRE(eth_getTransactionByHash["result"]["from"] == transactions[i].getFrom().hex(true));
        REQUIRE(eth_getTransactionByHash["result"]["gas"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getMaxFeePerGas()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["hash"] == transactions[i].hash().hex(true));
        REQUIRE(eth_getTransactionByHash["result"]["input"] == Hex::fromBytes(transactions[i].getData(),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["nonce"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getNonce()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["to"] == transactions[i].getTo().hex(true));
        REQUIRE(eth_getTransactionByHash["result"]["transactionIndex"] == Hex::fromBytes(Utils::uintToBytes(i),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["value"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getValue()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["s"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getS()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["v"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getV()),true).forRPC());
        REQUIRE(eth_getTransactionByHash["result"]["r"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getR()),true).forRPC());
      }

      for (uint64_t i = 0; i < transactions.size(); ++i) {
        json eth_getTransactionByBlockHashAndIndex = requestMethod("eth_getTransactionByBlockHashAndIndex", json::array({newBestBlock.hash().hex(true), Hex::fromBytes(Utils::uintToBytes(i),true).forRPC()}));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["blockHash"] == newBestBlock.hash().hex(true));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["blockNumber"] == "0x1");
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["from"] == transactions[i].getFrom().hex(true));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["gas"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getMaxFeePerGas()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["hash"] == transactions[i].hash().hex(true));
        REQUIRE(eth_getTransactionByBlockHashAndIndex["result"]["input"] == Hex::fromBytes(transactions[i].getData(),true).forRPC());
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
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["blockHash"] == newBestBlock.hash().hex(true));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["blockNumber"] == "0x1");
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["from"] == transactions[i].getFrom().hex(true));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["gas"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["gasPrice"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getMaxFeePerGas()),true).forRPC());
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["hash"] == transactions[i].hash().hex(true));
        REQUIRE(eth_getTransactionByBlockNumberAndIndexResponse["result"]["input"] == Hex::fromBytes(transactions[i].getData(),true).forRPC());
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
        REQUIRE(eth_getTransactionReceiptResponse["result"]["blockHash"] == newBestBlock.hash().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["blockNumber"] == "0x1");
        REQUIRE(eth_getTransactionReceiptResponse["result"]["from"] == transactions[i].getFrom().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["to"] == transactions[i].getTo().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["cumulativeGasUsed"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()), true).forRPC());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["effectiveGasUsed"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()), true).forRPC());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["effectiveGasPrice"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getMaxFeePerGas()), true).forRPC());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["gasUsed"] == Hex::fromBytes(Utils::uintToBytes(transactions[i].getGasLimit()), true).forRPC());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["contractAddress"] == json::value_t::null);
        REQUIRE(eth_getTransactionReceiptResponse["result"]["logs"] == json::array());
        REQUIRE(eth_getTransactionReceiptResponse["result"]["logsBloom"] == Hash().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["type"] == "0x00");
        REQUIRE(eth_getTransactionReceiptResponse["result"]["root"] == Hash().hex(true));
        REQUIRE(eth_getTransactionReceiptResponse["result"]["status"] == "0x1");
      }
    }
  }
}
