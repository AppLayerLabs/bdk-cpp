#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sstream>
#include <fstream>
#include <chrono>
#include <csignal>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>


#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/message.h>
#include "../proto/vm.grpc.pb.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/thread.hpp>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#include "json.hpp"

#include "block.h"
#include "utils.h"
#include "db.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

Block genesis("0000000000000000000000000000000000000000000000000000000000000000",
  1648317800,
  0, // 0 tx's
  0,
  ""
);

class VMServiceImplementation;

// global pointer, this is stupid but shared_from_this is returning a bad_weak_ptr
// as far I researched, shared_from_this can be only used inside the class constructor
// which is stupid, as the server doesn't start before it is initialized, I don't see a reason to not
// have a global pointer... but still, stupid stupid stupid.

VMServiceImplementation *globalPointer;

void startServer();


std::mutex lock;

// Logic and data behind the server's behavior.
class VMServiceImplementation final : public vm::VM::Service {
  public:
  std::string dbName;
  bool initialized = false;
  Database blocksDb; // Key -> blockHash()
                      // Value -> json block
  
  Database confirmedTxs; // key -> txHash.
                          // value -> "" (if exists == confirmed);

  Database txToBlock; // key -> txhash.
                      // value -> blockHash.

  Database accountsDb; // Key -> underscored user address "0x..."
                        // Value -> balance in wei.

  Database nonceDb;  // key -> address
                      // value -> nonce. (string).

  std::vector<dev::eth::TransactionBase> mempool;

  Status Initialize(ServerContext* context, const vm::InitializeRequest* request,
                  vm::InitializeResponse* reply) override {
    Utils::logToFile("Initialized Called");
    initialized = true;
    std::string jsonRequest;
    google::protobuf::util::JsonOptions options;
    google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
    Utils::logToFile(jsonRequest);
    // Open database with the nodeID as folder name.
    json req = json::parse(jsonRequest);
    dbName = req["nodeId"];
    blocksDb.setAndOpenDB(dbName + "-blocks");
    accountsDb.setAndOpenDB(dbName + "-balances");
    confirmedTxs.setAndOpenDB(dbName + "-txs");
    nonceDb.setAndOpenDB(dbName + "-nonces");
    txToBlock.setAndOpenDB(dbName + "-txToBlocks");
    if (blocksDb.isEmpty()) {
      blocksDb.putKeyValue(genesis.blockHash(), genesis.serializeToString());
      blocksDb.putKeyValue(boost::lexical_cast<std::string>(genesis.nHeight()), genesis.serializeToString());
      blocksDb.putKeyValue("latest", genesis.serializeToString());
    }
    if(accountsDb.isEmpty()) {
      accountsDb.putKeyValue("0x2993d9eadb91c588bebbc1da7179b607ac3ff699", "1000000000000000000000000000");
    }
    Block bestBlock(blocksDb.getKeyValue("latest"));
    Utils::logToFile(blocksDb.getKeyValue("latest"));

    auto hash = bestBlock.blockHash();
    Utils::logToFile(std::string("size: ") + boost::lexical_cast<std::string>(hash.size()));
    Utils::logToFile(bestBlock.blockHash());
    reply->set_last_accepted_id(Utils::hashToBytes(bestBlock.blockHash()));
    reply->set_last_accepted_parent_id(Utils::hashToBytes(bestBlock.prevBlockHash()));
    reply->set_status(1);
    reply->set_height(1);
    // bytes -> last block bytes.
    reply->set_bytes(bestBlock.serializeToString()); 
    reply->set_timestamp(Utils::secondsToGoTimeStamp(boost::lexical_cast<uint64_t>(bestBlock.timestamp())));
    std::string jsonAnswer;
    google::protobuf::util::MessageToJsonString(*reply, &jsonAnswer, options);
    Utils::logToFile(std::string("jsonAnswer:" + jsonAnswer));

    try {
      boost::thread p(&startServer);
      p.detach();
    } catch (std::exception &e) {
      Utils::logToFile(std::string("HTTP: ") + e.what());
    }

    Utils::logToFile("server started");
    return Status::OK;
  }

  Status SetState(ServerContext* context, const vm::SetStateRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("SetState called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Shutdown(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("Shutdown Called");
    Utils::logToFile(request->DebugString());
    if (initialized) {
      blocksDb.cleanCloseDB();
      accountsDb.cleanCloseDB();
    }
    return Status::OK;
  }

  Status CreateHandlers(ServerContext* context, const google::protobuf::Empty* request, vm::CreateHandlersResponse* reply) override {
    Utils::logToFile("CreateHandlers Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status CreateStaticHandlers(ServerContext* context, const google::protobuf::Empty* request, vm::CreateStaticHandlersResponse* reply) override {
    Utils::logToFile("CreateStaticHandlers Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Connected(ServerContext* context, const vm::ConnectedRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("Connected Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Disconnected(ServerContext* context, const vm::DisconnectedRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("Disconnected Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BuildBlock(ServerContext* context, const google::protobuf::Empty* request, vm::BuildBlockResponse* reply) override {
    lock.lock();
    Utils::logToFile("BuildBlock called");
    Block pastBlock(blocksDb.getKeyValue("latest"));
    const auto p1 = std::chrono::system_clock::now();
    uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(p1.time_since_epoch()).count();
    auto nextHeight = pastBlock.nHeight() + 1;
    Block newBestBlock(pastBlock.blockHash(),
                       boost::lexical_cast<dev::u256>(now),
                       0,
                       nextHeight,
                       "");
    std::vector<std::string> transactionHexes;
    for (auto tx : mempool) {
      newBestBlock.addTx(tx);
      confirmedTxs.putKeyValue(std::string("0x") + tx.sha3().hex(), dev::toHex(tx.rlp()));
      transactionHexes.push_back(std::string("0x") + tx.sha3().hex());
    }
    // Hash blockheader again
    newBestBlock.serializeToString();
    
    for (auto tx : transactionHexes) {
      txToBlock.putKeyValue(tx, std::string("0x") + newBestBlock.blockHash());
    }

    // All transaction added, clear mempool.
    mempool.clear();

    reply->set_id(Utils::hashToBytes(newBestBlock.blockHash()));
    reply->set_parent_id(Utils::hashToBytes(newBestBlock.prevBlockHash()));
    reply->set_bytes(newBestBlock.serializeToString());
    reply->set_height(boost::lexical_cast<uint64_t>(newBestBlock.nHeight()));
    reply->set_timestamp(Utils::secondsToGoTimeStamp(boost::lexical_cast<uint64_t>(newBestBlock.timestamp())));

    blocksDb.deleteKeyValue("latest");
    blocksDb.putKeyValue(std::string("0x") + newBestBlock.blockHash(), newBestBlock.serializeToString());
    blocksDb.putKeyValue(boost::lexical_cast<std::string>(newBestBlock.nHeight()), newBestBlock.serializeToString());
    blocksDb.putKeyValue("latest", newBestBlock.serializeToString());
    Utils::logToFile(request->DebugString());
    Utils::logToFile(reply->DebugString());
    lock.unlock();
    return Status::OK;
  }

  Status ParseBlock(ServerContext* context, const vm::ParseBlockRequest* request, vm::ParseBlockResponse* reply) override {
    Utils::logToFile("ParseBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetBlock(ServerContext* context, const vm::GetBlockRequest* request, vm::GetBlockResponse* reply) override {
    Utils::logToFile("GetBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK; 
  }

  Status SetPreference(ServerContext* context, const vm::SetPreferenceRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("SetPreference Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Health(ServerContext* context, const vm::HealthRequest* request, vm::HealthResponse* reply) override {
    Utils::logToFile("Health called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Version(ServerContext* context, const google::protobuf::Empty* request, vm::VersionResponse* reply) override {
    Utils::logToFile("Version Called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppRequest(ServerContext* context, const vm::AppRequestMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppRequest called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppRequestFailed(ServerContext* context, const vm::AppRequestFailedMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppRequestFailed called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppResponse(ServerContext* context, const vm::AppResponseMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppResponse called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status AppGossip(ServerContext* context, const vm::AppGossipMsg* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("AppGossip called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status Gather(ServerContext* context, const google::protobuf::Empty* request, vm::GatherResponse* reply) override {
    Utils::logToFile("Gather called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockVerify(ServerContext* context, const vm::BlockVerifyRequest* request, vm::BlockVerifyResponse* reply) override {
    Utils::logToFile("BlockVerify called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockAccept(ServerContext* context, const vm::BlockAcceptRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("BlockAccept called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BlockReject(ServerContext* context, const vm::BlockRejectRequest* request, google::protobuf::Empty* reply) override {
    Utils::logToFile("BlockReject called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetAncestors(ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply) override {
    Utils::logToFile("GetAncestors called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status BatchedParseBlock(ServerContext* context, const vm::BatchedParseBlockRequest* request, vm::BatchedParseBlockResponse* reply) override {
    Utils::logToFile("BatchedParseBlock called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status VerifyHeightIndex(ServerContext* context, const google::protobuf::Empty* request, vm::VerifyHeightIndexResponse* reply) override {
    Utils::logToFile("VerifyHeightIndex called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }

  Status GetBlockIDAtHeight(ServerContext* context, const vm::GetBlockIDAtHeightRequest* request, vm::GetBlockIDAtHeightResponse* reply) override {
    Utils::logToFile("GetBlockIDAtHeight called");
    Utils::logToFile(request->DebugString());
    return Status::OK;
  }
  
  std::string processRPCMessage(std::string message) {
    lock.lock();
    json ret;
    json messageJson = json::parse(message);
    bool log = true;
    ret["id"] = messageJson["id"];
    Utils::logToFile(message);
    ret["jsonrpc"] = "2.0";
    if (messageJson["method"] == "eth_blockNumber") {
      Block bestBlockHash(blocksDb.getKeyValue("latest"));
      std::string blockHeight = boost::lexical_cast<std::string>(bestBlockHash.nHeight());
      std::string blockHeightHex = "0x";
      blockHeightHex += Utils::uintToHex(blockHeight, false);
      ret["result"] = blockHeightHex;
      log = false;
    }

    if(messageJson["method"] == "eth_chainId") {
      log = false;
      ret["result"] = "0x2290";
    }

    if(messageJson["method"] == "net_version") {
      log = false;
      ret["result"] = "8848";
    }

    if(messageJson["method"] == "eth_getBalance") {
      log = false;
      std::string address = messageJson["params"][0].get<std::string>();
      Utils::logToFile(address);
      std::string balance;
      try {
        balance = accountsDb.getKeyValue(address);
      } catch (std::exception &e) {
        Utils::logToFile(std::string("eth_getBalance_error: ") + e.what());
        balance = "";
      }
      Utils::logToFile(balance);
      std::string hexValue = "0x";
      if (balance != "") { 
        hexValue += Utils::uintToHex(balance, false);
      } else {
        hexValue += "0";
      }
      ret["result"] = hexValue;
    }

    if(messageJson["method"] == "eth_getBlockByNumber") {
      log = true;
      std::string blockNumber = Utils::uintFromHex(messageJson["params"][0].get<std::string>());
      Utils::logToFile(std::string("getblockbynumber: ") + blockNumber);
      Block block(blocksDb.getKeyValue(blockNumber));
      json answer;
      answer["number"] = std::string("0x") + Utils::uintToHex(boost::lexical_cast<std::string>(block.nHeight()), false);
      answer["hash"] = std::string("0x") + block.blockHash();
      answer["parentHash"] = std::string("0x") + block.prevBlockHash();
      answer["nonce"] = "0x689056015818adbe"; // any nonce should be good
      answer["sha3Uncles"] = "0x";
      answer["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
      answer["transactionsRoot"] = "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421"; // No equivalent
      answer["stateRoot"] = "0xddc8b0234c2e0cad087c8b389aa7ef01f7d79b2570bccb77ce48648aa61c904d"; // No equivalent
      answer["miner"] = "0x24ff7cb43b80a5fb44c5ddeb5510d2a4b62ae26d";
      answer["difficulty"] = "0x4ea3f27bc";
      answer["totalDifficulty"] = "0x78ed983323d";
      answer["extraData"] = "0x476574682f4c5649562f76312e302e302f6c696e75782f676f312e342e32";
      answer["size"] = "0x220";
      answer["gasLimit"] = "0x7a120";
      answer["gasUsed"] = "0x0";
      answer["timestamp"] = std::string("0x") + Utils::uintToHex(boost::lexical_cast<std::string>(block.timestamp()), false);
      answer["transactions"] = json::array();
      auto transaction = block.transactions();
      for (auto tx : transaction) {
        answer["transactions"].push_back(std::string("0x") + tx.sha3().hex());
      }
      answer["uncles"] = json::array();
      ret["result"] = answer;
    }

    if(messageJson["method"] == "eth_getCode") {
      log = false;
      ret["result"] = "0x";
    }

    if(messageJson["method"] == "eth_gasPrice") {
      log = false;
      ret["result"] = "0x12a05f200"; // force to 5 gwet
    }

    if(messageJson["method"] == "eth_estimateGas") {
      log = false;
      ret["result"] = "0x5208";
    }

    if(messageJson["method"] == "eth_getTransactionCount") {
      log = false;
      Utils::logToFile("txCount 1");
      std::string txCount = nonceDb.getKeyValue(messageJson["params"][0].get<std::string>());
      Utils::logToFile(txCount);
      Utils::logToFile("txCount 2");
      if (txCount == "") {
        Utils::logToFile("txCount 3");
        ret["result"] = "0x0";
      } else {
        Utils::logToFile("txCount 4");
        ret["result"] = std::string("0x") + Utils::uintToHex(txCount, false);
        Utils::logToFile("txCount 5");
      }
    }

    if(messageJson["method"] == "eth_sendRawTransaction") {
      log = true;
      std::string txRlp = messageJson["params"][0].get<std::string>();
      try {
        dev::eth::TransactionBase tx(dev::fromHex(txRlp), dev::eth::CheckTransaction::Everything);
        std::string address = std::string("0x") + tx.from().hex();
        std::string targetAddress = std::string("0x") + tx.to().hex();
        dev::u256 userBalance = boost::lexical_cast<dev::u256>(accountsDb.getKeyValue(address));
        Utils::logToFile("error 1");
        auto txValue = tx.value();
        if (txValue > userBalance) {
          // this should never happen ayyy lmao
          throw "";
        }
        // We actually take off the user balance before actually confirming the transaction, it should confirm regardless.
        userBalance = userBalance - txValue;
        Utils::logToFile("error 2");
        accountsDb.putKeyValue(address, boost::lexical_cast<std::string>(userBalance));
        Utils::logToFile("error 3");
        std::string pastBalanceStr = accountsDb.getKeyValue(targetAddress);
        dev::u256 newBal = txValue;
        if (pastBalanceStr != "") {
          newBal = newBal + boost::lexical_cast<dev::u256>(pastBalanceStr);
        }
        accountsDb.putKeyValue(targetAddress, boost::lexical_cast<std::string>(newBal));
        Utils::logToFile("error 4");
        std::string oldNonce = nonceDb.getKeyValue(address);
        Utils::logToFile("error 5");
        if (oldNonce == "") {
          nonceDb.putKeyValue(address, boost::lexical_cast<std::string>(1));
          Utils::logToFile("error 6");
        } else {
          std::string newNonce = boost::lexical_cast<std::string>(uint64_t(1 + boost::lexical_cast<uint64_t>(oldNonce)));
          Utils::logToFile("error 7");
          nonceDb.putKeyValue(address, newNonce);
          Utils::logToFile("error 8");
        }

        Utils::logToFile("error 9");
        globalPointer->mempool.push_back(tx);
        Utils::logToFile("error 10");
        ret["result"] = std::string("0x") + tx.sha3().hex();
        Utils::logToFile("error 11");
      } catch (std::exception &e) {
        Utils::logToFile(std::string("sendRawTransaction failed! ") + e.what());
      }
    }

    if(messageJson["method"] == "eth_getTransactionReceipt") {
      Utils::logToFile("god help me -1");
      std::string txHash = messageJson["params"][0].get<std::string>();
      Utils::logToFile("god help me 0");
      std::string txRlp = confirmedTxs.getKeyValue(txHash);
      if (txRlp == "") {
        // wait to try again
        // Answering 0x0 means tx got rejected!
      } else {
        Utils::logToFile("god help me 1");
        dev::eth::TransactionBase tx(dev::fromHex(txRlp), dev::eth::CheckTransaction::Everything);
        Utils::logToFile("god help me 2");
        ret["result"]["transactionHash"] = std::string("0x") + tx.sha3().hex();
        Utils::logToFile("god help me 3");
        ret["result"]["transactionIndex"] = "0x1";
        Utils::logToFile("god help me 4");
        std::string blockHash = txToBlock.getKeyValue(txHash);
        Utils::logToFile("god help me 5");
        Utils::logToFile(blockHash);
        Block block(blocksDb.getKeyValue(blockHash));
        Utils::logToFile("god help me 6");
        std::string blockNumber = std::string("0x") + Utils::uintToHex(boost::lexical_cast<std::string>(block.nHeight()), false);
        Utils::logToFile("god help me 7");
        ret["result"]["blockNumber"] = blockNumber;
        Utils::logToFile("god help me 8");
        ret["result"]["blockHash"] = blockHash;
        Utils::logToFile("god help me 9");
        ret["result"]["cumulativeGasUsed"] = "0x5208";
        Utils::logToFile("god help me 10");
        ret["result"]["gasUsed"] = "0x5208";
        Utils::logToFile("god help me 11");
        ret["result"]["contractAddress"] = "0x";
        Utils::logToFile("god help me 12");
        ret["logs"] = json::array();
        Utils::logToFile("god help me 13");
        ret["result"]["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
        Utils::logToFile("god help me 14");
        ret["result"]["status"] = "0x1"; 
        Utils::logToFile("god help me 15");
      }
    }

    if(messageJson["method"] == "eth_getBlockByHash") {
      std::string blockHash = messageJson["params"][0].get<std::string>();
      Block block(blocksDb.getKeyValue(blockHash));
      json answer;
      answer["number"] = std::string("0x") + Utils::uintToHex(boost::lexical_cast<std::string>(block.nHeight()), false);
      answer["hash"] = std::string("0x") + block.blockHash();
      answer["parentHash"] = std::string("0x") + block.prevBlockHash();
      answer["nonce"] = "0x689056015818adbe"; // any nonce should be good
      answer["sha3Uncles"] = "0x";
      answer["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
      answer["transactionsRoot"] = "0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421"; // No equivalent
      answer["stateRoot"] = "0xddc8b0234c2e0cad087c8b389aa7ef01f7d79b2570bccb77ce48648aa61c904d"; // No equivalent
      answer["miner"] = "0x24ff7cb43b80a5fb44c5ddeb5510d2a4b62ae26d";
      answer["difficulty"] = "0x4ea3f27bc";
      answer["totalDifficulty"] = "0x78ed983323d";
      answer["extraData"] = "0x476574682f4c5649562f76312e302e302f6c696e75782f676f312e342e32";
      answer["size"] = "0x220";
      answer["gasLimit"] = "0x7a120";
      answer["gasUsed"] = "0x0";
      answer["timestamp"] = std::string("0x") + Utils::uintToHex(boost::lexical_cast<std::string>(block.timestamp()), false);
      answer["transactions"] = json::array();
      auto transaction = block.transactions();
      for (auto tx : transaction) {
        answer["transactions"].push_back(std::string("0x") + tx.sha3().hex());
      }
      answer["uncles"] = json::array();
      ret["result"] = answer;
    }

    if (messageJson["method"] == "FAUCET") {
      std::string address = messageJson["address"].get<std::string>();
      std::string prevBal = accountsDb.getKeyValue(address);
      dev::u256 bal = 1000000000000000000;
      if (prevBal != "") {
        bal = bal + boost::lexical_cast<dev::u256>(prevBal);
      }
      accountsDb.putKeyValue(address, boost::lexical_cast<std::string>(bal));
    }

    lock.unlock();
    if (log) {
      Utils::logToFile(ret.dump());
    }
    return ret.dump();
  }
};

template<
    class Body, class Allocator,
    class Send>
void
handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send)
{
    // Returns a bad request response
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return send(bad_request("Illegal request-target"));

    std::string request = req.body();

    // Respond to OPTIONS.
    // there is absolute no fkin documentation over this
    // someone plz write some :pray:
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
    // Respond to POST
    std::string answer = globalPointer->processRPCMessage(request);
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
    res.body() = answer;
    res.keep_alive(req.keep_alive());
    res.prepare_payload();
    return send(std::move(res));
}

template<class Stream>
struct send_lambda
{
    Stream& stream_;
    bool& close_;
    beast::error_code& ec_;

    explicit
    send_lambda(
        Stream& stream,
        bool& close,
        beast::error_code& ec)
        : stream_(stream)
        , close_(close)
        , ec_(ec)
    {
    }

    template<bool isRequest, class Body, class Fields>
    void
    operator()(http::message<isRequest, Body, Fields>&& msg) const
    {
        // Determine if we should close the connection after
        close_ = msg.need_eof();

        // We need the serializer here because the serializer requires
        // a non-const file_body, and the message oriented version of
        // http::write only works with const messages.
        http::serializer<isRequest, Body, Fields> sr{msg};
        http::write(stream_, sr, ec_);
    }
};

void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

void
do_session(
    tcp::socket& socket,
    std::shared_ptr<std::string const> const& doc_root)
{
    bool close = false;
    beast::error_code ec;

    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    // This lambda is used to send messages
    send_lambda<tcp::socket> lambda{socket, close, ec};

    for(;;)
    {
        // Read a request
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if(ec == http::error::end_of_stream)
            break;
        if(ec)
            return fail(ec, "read");

        // Send the response
        handle_request(*doc_root, std::move(req), lambda);
        if(ec)
            return fail(ec, "write");
        if(close)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    // Send a TCP shutdown
    socket.shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}

void startServer()
{
    try
    {
        // Check command line arguments.
        auto const address = net::ip::make_address("0.0.0.0");
        auto const port = 80;
        auto const doc_root = std::make_shared<std::string>("/");

        // The io_context is required for all I/O
        net::io_context ioc{1};

        // The acceptor receives incoming connections
        tcp::acceptor acceptor{ioc, {address, port}};
        for(;;)
        {
            // This will receive the new connection
            tcp::socket socket{ioc};

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread{std::bind(
                &do_session,
                std::move(socket),
                doc_root)}.detach();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return;
    }
}

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  VMServiceImplementation service;

  globalPointer = &service;
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "1|11|tcp|" << server_address << "|grpc\n"<< std::flush;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return
  server->Wait();
}

#endif // MAIN_H