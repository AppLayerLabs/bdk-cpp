#ifndef GRPCSERVER_H
#define GRPCSERVER_H

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
#include "grpcclient.h"

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

class Subnet;

class VMServiceImplementation final : public vm::VM::Service, public std::enable_shared_from_this<VMServiceImplementation> {
  public:

  std::string dbName;
  bool initialized = false;
  std::shared_ptr<Subnet> subnet_ptr;
  std::shared_ptr<VMCommClient> commClient;

  VMServiceImplementation (std::shared_ptr<Subnet> subnet_ptr_) : subnet_ptr(subnet_ptr_) {};

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

  void blockRequester() {
    commClient->requestBlock();
    return;
  }

  Status Initialize(ServerContext* context, const vm::InitializeRequest* request, vm::InitializeResponse* reply) override;

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
    reply->set_version("1");
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
        this->mempool.push_back(tx);
        Utils::logToFile("error 10");
        ret["result"] = std::string("0x") + tx.sha3().hex();
        Utils::logToFile("error 11");
        boost::thread miner(boost::bind(&VMServiceImplementation::blockRequester, this));
        miner.detach();
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

    if (log) {
      Utils::logToFile(ret.dump());
    }
    return ret.dump();
  }
};

#endif // GRPCSERVER_H