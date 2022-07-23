#include "subnet.h"

// Process a Metamask RPC message. There are multiple edge cases that need to be handled.
std::string Subnet::processRPCMessage(std::string &req) {
  Utils::LogPrint(Log::subnet, "processRPCMessage", "Received RPC message: " + req);
  json ret;
  json messageJson = json::parse(req);
  ret["id"] = messageJson["id"];
  ret["jsonrpc"] = "2.0";
  if (messageJson["method"] == "eth_blockNumber") {
    Block bestBlock = chainHead->latest();
    ret["result"] = "0x" + Utils::uintToHex(bestBlock.nHeight());
  }
  if (messageJson["method"] == "eth_chainId") {
    ret["result"] = "0x2290";
  }
  if (messageJson["method"] == "net_version") {
    ret["result"] = "8848";
  }
  if(messageJson["method"] == "eth_getBalance") {
    Address address(messageJson["params"][0].get<std::string>());
    Utils::LogPrint(Log::subnet, "eth_getBalance address: ", address.innerAddress);
    auto balance = this->headState->getNativeBalance(address);
    std::string hexValue = "0x";
    hexValue += Utils::uintToHex(balance);
    ret["result"] = hexValue;
    Utils::LogPrint(Log::subnet, "eth_getBalance: ", ret.dump());
  }
  if (messageJson["method"] == "eth_getBlockByNumber") {
    std::string blockString = messageJson["params"][0].get<std::string>();
    std::unique_ptr<Block> block;
    if (blockString == "latest") {
      block = std::make_unique<Block>(chainHead->latest());
    } else {
      uint64_t blockNumber = boost::lexical_cast<uint64_t>(Utils::hexToUint(blockString));
      Utils::LogPrint(Log::subnet, "eth_getBlockByNumber blockNumber: ", std::to_string(blockNumber));
      block = std::make_unique<Block>(chainHead->getBlock(blockNumber));
      Utils::LogPrint(Log::subnet, "eth_getBlockByNumber block: ", dev::toHex(block->serializeToBytes()));
    }

    json answer;
    answer["number"] = std::string("0x") + Utils::uintToHex(block->nHeight());
    answer["hash"] = std::string("0x") + dev::toHex(block->getBlockHash());

    answer["parentHash"] = std::string("0x") + dev::toHex(block->prevBlockHash());
    answer["nonce"] = "0x00000000000000"; // Any nonce should be good, MetaMask is not checking block validity.
    answer["sha3Uncles"] = "0x";
    answer["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    answer["transactionsRoot"] = "0x0000000000000000000000000000000000000000000000000000000000000000"; // No equivalent
    answer["stateRoot"] = "0x0000000000000000000000000000000000000000000000000000000000000000"; // No equivalent
    answer["miner"] = "0x0000000000000000000000000000000000000000";
    answer["difficulty"] = "0x000000000";
    answer["totalDifficulty"] = "0x00000000000";
    answer["extraData"] = "0x000000000000000000000000000000000000000000000000000000000000";
    answer["size"] = "0xfffff";
    answer["gasLimit"] = "0xfffff";
    answer["gasUsed"] = "0xfffff";
    answer["timestamp"] = std::string("0x") + Utils::uintToHex(block->timestampInSeconds());  // Seconds since epoch
    answer["transactions"] = json::array();
    for (auto tx : block->transactions()) {
      answer["transactions"].push_back(std::string("0x") + tx.hash());
    }
    answer["uncles"] = json::array();
    ret["result"] = answer;
  }
  if (messageJson["method"] == "eth_getCode") {
    ret["result"] = "0x";
  }
  if (messageJson["method"] == "eth_gasPrice") {
    ret["result"] = "0x12a05f200"; // Force to 5 Gwei
  }
  if (messageJson["method"] == "eth_estimateGas") {
    ret["result"] = "0x5208";
  }
  if(messageJson["method"] == "eth_getTransactionCount") {
    Address address(messageJson["params"][0].get<std::string>());
    auto addressNonce = this->headState->getNativeNonce(address);

    ret["result"] = std::string("0x") + Utils::uintToHex(addressNonce);
  }
  if (messageJson["method"] == "eth_sendRawTransaction") {
    std::string txRlp = messageJson["params"][0].get<std::string>();
    try {
      dev::eth::TransactionBase tx(dev::fromHex(txRlp), dev::eth::CheckTransaction::Everything);
      std::pair<int, std::string> txRet = this->headState->validateTransaction(tx);
      if (txRet.first != 0) {
        ret["error"] = json({{"code", txRet.first}, {"message", txRet.second}});
      }
      ret["result"] = std::string("0x") + tx.hash();
    } catch (std::exception &e) {
      Utils::logToFile(std::string("sendRawTransaction failed! ") + e.what());
    }
  }
  if (messageJson["method"] == "eth_getTransactionReceipt") {
    std::string txHash = messageJson["params"][0].get<std::string>();
    Utils::patchHex(txHash);
    try {
      dev::eth::TransactionBase tx = chainHead->getTransaction(txHash);
      ret["result"]["transactionHash"] = std::string("0x") + tx.hash();

      // TODO: Implement block transaction index (requires rewriting TransactionBase)
      ret["result"]["transactionIndex"] = "0x1";
      Block block = chainHead->getBlockFromTx(txHash);
      ret["result"]["blockNumber"] = std::string("0x") + Utils::uintToHex(block.nHeight());
      ret["result"]["blockHash"] = std::string("0x") + dev::toHex(block.getBlockHash());
      ret["result"]["cumulativeGasUsed"] = "0x" + Utils::uintToHex(tx.gas());
      ret["result"]["gasUsed"] = "0x" + Utils::uintToHex(tx.gas());
      ret["result"]["contractAddress"] = "0x";  // TODO: does MetaMask check if we called a contract?
      ret["logs"] = json::array();
      ret["result"]["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
      ret["result"]["status"] = "0x1";
    } catch (std::exception &e) {
      // TODO: proper error handling
      Utils::LogPrint(Log::subnet, "eth_getTransactionReceipt: tx not found ", e.what());
    }
  }
  if (messageJson["method"] == "eth_getBlockByHash") {
    std::string blockHash = messageJson["params"][0].get<std::string>();
    Utils::patchHex(blockHash);
    blockHash = Utils::hexToBytes(blockHash);
    Block block = chainHead->getBlock(blockHash);
    json answer;
    answer["number"] = std::string("0x") + Utils::uintToHex(block.nHeight());
    answer["hash"] = std::string("0x") + dev::toHex(block.getBlockHash());
    answer["parentHash"] = std::string("0x") + dev::toHex(block.prevBlockHash());
    answer["nonce"] = "0x00000000000000"; // Any nonce should be good, MetaMask is not checking block validity.
    answer["sha3Uncles"] = "0x";
    answer["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    answer["transactionsRoot"] = "0x0000000000000000000000000000000000000000000000000000000000000000"; // No equivalent
    answer["stateRoot"] = "0x0000000000000000000000000000000000000000000000000000000000000000"; // No equivalent
    answer["miner"] = "0x0000000000000000000000000000000000000000";
    answer["difficulty"] = "0x000000000";
    answer["totalDifficulty"] = "0x00000000000";
    answer["extraData"] = "0x000000000000000000000000000000000000000000000000000000000000";
    answer["size"] = "0xfffff";
    answer["gasLimit"] = "0xfffff";
    answer["gasUsed"] = "0xfffff";
    answer["timestamp"] = std::string("0x") + Utils::uintToHex(block.timestampInSeconds()); // Seconds since epoch
    answer["transactions"] = json::array();
    for (auto tx : block.transactions()) {
      answer["transactions"].push_back(std::string("0x") + tx.hash());
    }
    answer["uncles"] = json::array();
    ret["result"] = answer;
  }
  if (messageJson["method"] == "eth_call") {
    // TODO: Implement eth_call
    Utils::logToFile(std::string("EthCall: ") + ret["result"].get<std::string>());
  }

  //
  // {
  //   "method" = "IncreaseBalance"
  //   "address" = "0x..."
  // }
  // Will increase the balance in 1 SUBS.
  if(messageJson["method"] == "IncreaseBalance") {
    Address address(messageJson["address"].get<std::string>());
    this->headState->addBalance(address);
    ret["result"] = "SUCCESS";
  }
  return ret.dump();
}

