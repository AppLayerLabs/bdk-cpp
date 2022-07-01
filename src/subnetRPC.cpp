#include "subnet.h"

// Process a Metamask RPC message
// There are multiple edge cases that need to be handled.
std::string Subnet::processRPCMessage(std::string &req) { 
  json ret;
  json messageJson = json::parse(req);
  ret["id"] = messageJson["id"];
  ret["jsonrpc"] = "2.0";
  if (messageJson["method"] == "eth_blockNumber") {
    Block bestBlock = chainHead->latest();
    ret["result"] = "0x" + Utils::uintToHex(bestBlock.nHeight());
  }
  if(messageJson["method"] == "eth_chainId") {
    ret["result"] = "0x" + dev::toHex(this->initParams.chainId);
  }
  if(messageJson["method"] == "net_version") {
    ret["result"] = std::to_string(this->initParams.networkId);
  }
  if(messageJson["method"] == "eth_getBalance") {
    std::string address = messageJson["params"][0].get<std::string>();
    Utils::patchHex(address);
    auto balance = this->headState->getNativeBalance(address);
    std::string hexValue = "0x";
    hexValue += Utils::uintToHex(balance);
    ret["result"] = hexValue;
  }
  if(messageJson["method"] == "eth_getBlockByNumber") {
    std::string blockString = messageJson["params"][0].get<std::string>();
    std::unique_ptr<Block> block;
    if (blockString == "latest") {
      block = std::make_unique<Block>(chainHead->latest());
    } else {
      uint64_t blockNumber = boost::lexical_cast<uint64_t>(Utils::hexToUint(blockString));
      block = std::make_unique<Block>(chainHead->getBlock(blockNumber));
    }

    json answer;
    answer["number"] = std::string("0x") + Utils::uintToHex(block->nHeight());
    answer["hash"] = std::string("0x") + block->getBlockHash();
    answer["parentHash"] = std::string("0x") + block->prevBlockHash();
    answer["nonce"] = "0x00000000000000"; // any nonce should be good, metamask is not checking block validity.
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
    // Timestamp here is in seconds since epoch
    answer["timestamp"] = std::string("0x") + Utils::uintToHex(block->timestampInSeconds());
    answer["transactions"] = json::array();
    for (auto tx : block->transactions()) {
      answer["transactions"].push_back(std::string("0x") + tx.hash());
    }
    answer["uncles"] = json::array();
    ret["result"] = answer;
  }
  if(messageJson["method"] == "eth_getCode") {
    ret["result"] = "0x";
  }
  if(messageJson["method"] == "eth_gasPrice") {
    ret["result"] = "0x12a05f200"; // force to 5 gwet
  }
  if(messageJson["method"] == "eth_estimateGas") {
    ret["result"] = "0x5208";
  }
  if(messageJson["method"] == "eth_getTransactionCount") {
    Utils::logToFile("txCount 1");
    std::string address = messageJson["params"][0].get<std::string>();
    Utils::patchHex(address);
    auto addressNonce = this->headState->getNativeNonce(address);

    ret["result"] = std::string("0x") + Utils::uintToHex(addressNonce);
  }
  if(messageJson["method"] == "eth_sendRawTransaction") {
    std::string txRlp = messageJson["params"][0].get<std::string>();
    try {
      dev::eth::TransactionBase tx(dev::fromHex(txRlp), dev::eth::CheckTransaction::Everything);
      this->headState->validateTransaction(tx);
    } catch (std::exception &e) {
      Utils::logToFile(std::string("sendRawTransaction failed! ") + e.what());
    }
  }
  if(messageJson["method"] == "eth_getTransactionReceipt") {
    std::string txHash = messageJson["params"][0].get<std::string>();
    Utils::patchHex(txHash);
    dev::eth::TransactionBase tx = chainHead->getTransaction(txHash);

    ret["result"]["transactionHash"] = std::string("0x") + tx.hash();

    // TODO: Implement block transacion index.
    ret["result"]["transactionIndex"] = "0x1";
    Block block = chainHead->getBlockFromTx(txHash);
    ret["result"]["blockNumber"] = std::string("0x") + Utils::uintToHex(block.nHeight());
    ret["result"]["blockHash"] = std::string("0x") + block.getBlockHash();
    ret["result"]["cumulativeGasUsed"] = "0x" + Utils::uintToHex(tx.gas());
    ret["result"]["gasUsed"] = "0x" + Utils::uintToHex(tx.gas());
    // Does metamask checks if we called a contract?
    ret["result"]["contractAddress"] = "0x";
    ret["logs"] = json::array();
    ret["result"]["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    ret["result"]["status"] = "0x1"; 
  }
  if(messageJson["method"] == "eth_getBlockByHash") {
    std::string blockHash = messageJson["params"][0].get<std::string>();
    Utils::patchHex(blockHash);
    Block block = chainHead->getBlock(blockHash);
    json answer;
    answer["number"] = std::string("0x") + Utils::uintToHex(block.nHeight());
    answer["hash"] = std::string("0x") + block.getBlockHash();
    answer["parentHash"] = std::string("0x") + block.prevBlockHash();
    answer["nonce"] = "0x00000000000000"; // any nonce should be good, metamask is not checking block validity.
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
    answer["timestamp"] = std::string("0x") + Utils::uintToHex(block.timestampInSeconds());
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
    std::string address = messageJson["address"].get<std::string>();
    Utils::patchHex(address);
    this->headState->addBalance(address);
    ret["result"] = "SUCCESS";
  }
  return ret.dump();
}