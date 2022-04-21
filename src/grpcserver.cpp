#include "httpserver.h"
#include "validation.h"
#include "grpcserver.h"

Status VMServiceImplementation::Initialize(ServerContext* context, const vm::InitializeRequest* request,
                  vm::InitializeResponse* reply) {
    Utils::logToFile("Initialized Called");
    std::string jsonRequest;
    google::protobuf::util::JsonOptions options;
    google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
    Utils::logToFile(jsonRequest);
    // Open database with the nodeID as folder name.
    json req = json::parse(jsonRequest);
    std::string nodeID = req["nodeId"];
    Utils::logToFile("Creating validation pointer");
    validation = std::make_shared<Validation>(nodeID);
    auto bestBlock = validation->initialize();
    initialized = true;
    Utils::logToFile("Validation initialize completed");
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

    commClient = std::make_shared<VMCommClient>(grpc::CreateChannel(request->server_addr(), grpc::InsecureChannelCredentials()));
    try {
      boost::thread p(&startServer, shared_from_this());
      p.detach();
    } catch (std::exception &e) {
      Utils::logToFile(std::string("HTTP: ") + e.what());
    }

    Utils::logToFile("server started");
    return Status::OK;
  }

Status VMServiceImplementation::BuildBlock(ServerContext* context, const google::protobuf::Empty* request, vm::BuildBlockResponse* reply) {
    Utils::logToFile("BuildBlock called");
    auto newBestBlock = validation->createNewBlock();

    reply->set_id(Utils::hashToBytes(newBestBlock.blockHash()));
    reply->set_parent_id(Utils::hashToBytes(newBestBlock.prevBlockHash()));
    reply->set_bytes(newBestBlock.serializeToString());
    reply->set_height(boost::lexical_cast<uint64_t>(newBestBlock.nHeight()));
    reply->set_timestamp(Utils::secondsToGoTimeStamp(boost::lexical_cast<uint64_t>(newBestBlock.timestamp())));
    Utils::logToFile(request->DebugString());
    Utils::logToFile(reply->DebugString());
    isMining = false;
    return Status::OK;
}

Status VMServiceImplementation::Shutdown(ServerContext* context, const google::protobuf::Empty* request, google::protobuf::Empty* reply) {
  Utils::logToFile("Shutdown Called");
  Utils::logToFile(request->DebugString());
  if (initialized) {
    validation->cleanAndClose();
  }
  return Status::OK;
}

std::string VMServiceImplementation::processRPCMessage(std::string message) {
  json ret;
  json messageJson = json::parse(message);
  bool log = true;
  ret["id"] = messageJson["id"];
  Utils::logToFile(message);
  ret["jsonrpc"] = "2.0";
  if (messageJson["method"] == "eth_blockNumber") {
    Block bestBlockHash = validation->getLatestBlock();
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
      balance = validation->getAccountBalanceFromDB(address);
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
    std::string blockNumber = messageJson["params"][0].get<std::string>();
    if (blockNumber != "latest") {
      Utils::logToFile(std::string("blockNumber: ") + blockNumber + " " + boost::lexical_cast<std::string>(blockNumber.size()));
      blockNumber = Utils::uintFromHex(blockNumber);
    }
    Utils::logToFile(std::string("getblockbynumber: ") + blockNumber);
    Block block = validation->getBlock(blockNumber);
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
    std::string address = messageJson["params"][0].get<std::string>();
    std::string txCount = validation->getAccountNonce(address);
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
      if(!validation->validateTransaction(tx)) {
        ret["result"] = std::string("Invalid Tx");
      } else {
        validation->addTxToMempool(tx);
        ret["result"] = std::string("0x") + tx.sha3().hex();
        if (!isMining) {
          isMining = true;
          boost::thread miner(boost::bind(&VMServiceImplementation::blockRequester, this));
          miner.detach();
        }
      }
    } catch (std::exception &e) {
      Utils::logToFile(std::string("sendRawTransaction failed! ") + e.what());
    }
  }
  if(messageJson["method"] == "eth_getTransactionReceipt") {
    Utils::logToFile("god help me -1");
    std::string txHash = messageJson["params"][0].get<std::string>();
    Utils::logToFile("god help me 0");
    std::string txRlp = validation->getConfirmedTx(txHash);
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
      std::string blockHash = validation->getTxToBlock(txHash);
      Utils::logToFile("god help me 5");
      Utils::logToFile(blockHash);
      Block block = validation->getBlock(blockHash);
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
    Block block = validation->getBlock(blockHash);
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
  if (messageJson["method"] == "eth_call") {
    ret["result"] = validation->processEthCall(messageJson["params"]);
  }
  if (messageJson["method"] == "FAUCET") {
    std::string address = messageJson["address"].get<std::string>();
    validation->faucet(address);
  }

  if (messageJson["method"] == "createNewToken") {
    validation->createNewERC20(messageJson["params"]);
  }
  if (log) {
    Utils::logToFile(ret.dump());
  }
  return ret.dump();
}