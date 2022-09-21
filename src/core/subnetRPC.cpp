#include "subnet.h"

// Process a Metamask RPC message. There are multiple edge cases that need to be handled.
std::string Subnet::processRPCMessage(std::string &req) {
  Utils::LogPrint(Log::subnet, "processRPCMessage", "Received RPC message: " + req);
  json ret;
  json messageJson = json::parse(req);
  ret["id"] = messageJson["id"];
  ret["jsonrpc"] = "2.0";
  if (messageJson["method"] == "eth_blockNumber") {
    auto bestBlock = chainHead->latest();
    ret["result"] = "0x" + Utils::uintToHex(bestBlock->nHeight());
    Utils::logToFile("eth_blockNumber: " + ret["result"].get<std::string>());
  }
  if (messageJson["method"] == "eth_chainId") {
    ret["result"] = "0x2290";
  }
  if (messageJson["method"] == "net_version") {
    ret["result"] = "8848";
  }
  if(messageJson["method"] == "eth_getBalance") {
    Address address(messageJson["params"][0].get<std::string>(), true);
    Utils::LogPrint(Log::subnet, "eth_getBalance address: ", address.hex());
    auto balance = this->headState->getNativeBalance(address);
    std::string hexValue = "0x";
    hexValue += Utils::uintToHex(balance);
    ret["result"] = hexValue;
    Utils::LogPrint(Log::subnet, "eth_getBalance: ", ret.dump());
  }
  if (messageJson["method"] == "eth_getBlockByNumber") {
    std::string blockString = messageJson["params"][0].get<std::string>();
    bool includeTxs = false;
    bool latest = false;
    uint64_t height = 0;
    if (blockString == "latest") {
      latest = true;
    } else {
      height = boost::lexical_cast<uint64_t>(Utils::hexToUint(blockString));
      Utils::LogPrint(Log::subnet, "eth_getBlockByNumber blockNumber: ", std::to_string(height));
      if (!chainHead->exists(height)) {
        ret["error"] = { {"code", -32000}, {"message", "Block not found"} };
        return ret.dump();
      }
    }

    auto block = (latest ? chainHead->latest() : chainHead->getBlock(height));
    if (block == nullptr) {
      ret["error"] = { {"code", -32000}, {"message", "Block not found"} };
      return ret.dump();
    }

    Utils::LogPrint(Log::subnet, "eth_getBlockByNumber block: ", dev::toHex(block->serializeToBytes(true)));
    if (messageJson["params"].size() > 1) {
      includeTxs = messageJson["params"][1].get<bool>();
    }
    json answer;
    answer["number"] = std::string("0x") + Utils::uintToHex(block->nHeight());
    answer["hash"] = std::string("0x") + Utils::bytesToHex(block->getBlockHash().get());

    answer["parentHash"] = std::string("0x") + Utils::bytesToHex(block->prevBlockHash().get());
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
      if(includeTxs) {
        // https://www.quicknode.com/docs/ethereum/eth_getTransactionByHash
        json transaction;
        transaction["hash"] = std::string("0x") + Utils::bytesToHex(tx.second.hash().get());
        transaction["nonce"] = std::string("0x") + Utils::uintToHex(tx.second.nonce());
        transaction["blockHash"] = std::string("0x") + Utils::bytesToHex(block->getBlockHash().get());
        transaction["blockNumber"] = std::string("0x") + Utils::uintToHex(block->nHeight());
        transaction["transactionIndex"] = std::string("0x") + Utils::uintToHex(tx.second.blockIndex());
        transaction["from"] = std::string("0x") + tx.second.from().hex();
        transaction["to"] = std::string("0x") + tx.second.to().hex();
        transaction["value"] = std::string("0x") + Utils::uintToHex(tx.second.value());
        transaction["gasPrice"] = std::string("0x") + Utils::uintToHex(tx.second.gasPrice());
        transaction["gas"] = std::string("0x") + Utils::uintToHex(tx.second.gas());
        transaction["input"] = std::string("0x") + Utils::bytesToHex(tx.second.data());
        transaction["v"] = std::string("0x") + Utils::uintToHex(tx.second.v());
        transaction["standardV"] = std::string("0x") + Utils::uintToHex(tx.second.recoverId());
        transaction["r"] = std::string("0x") + Utils::uintToHex(tx.second.r());
        transaction["raw"] = std::string("0x") + Utils::bytesToHex(tx.second.rlpSerialize(true));
        transaction["chainid"] = std::string("0x") + Utils::uintToHex(tx.second.chainId());
        answer["transactions"].push_back(transaction);
      } else {
        answer["transactions"].push_back(std::string("0x") + Utils::bytesToHex(tx.second.hash().get()));
      }
    }
    answer["uncles"] = json::array();
    ret["result"] = answer;
    Utils::LogPrint(Log::subnet, "eth_getBlockByNumber: ", ret.dump());
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
  if (messageJson["method"] == "eth_getTransactionCount") {
    Address address(messageJson["params"][0].get<std::string>(), true);
    auto addressNonce = this->headState->getNativeNonce(address);
    ret["result"] = std::string("0x") + Utils::uintToHex(addressNonce);
  }
  if (messageJson["method"] == "eth_sendRawTransaction") {
    std::string txRlp = messageJson["params"][0].get<std::string>();
    Utils::patchHex(txRlp);
    try {
      std::string txStr = Utils::hexToBytes(txRlp);
      Tx::Base tx(txStr, false);
      std::pair<int, std::string> txRet = this->headState->validateTransactionForRPC(std::move(tx), true);
      if (txRet.first != 0) {
        ret["error"] = json({{"code", txRet.first}, {"message", txRet.second}});
      }
      ret["result"] = std::string("0x") + Utils::bytesToHex(tx.hash().get());
    } catch (std::exception &e) {
      Utils::logToFile(std::string("sendRawTransaction failed! ") + e.what());
    }
  }
  if (messageJson["method"] == "eth_getTransactionReceipt") {
    Hash txHash = Hash(Utils::hexToBytes(messageJson["params"][0].get<std::string>()));
    try {
      auto tx = chainHead->getTransaction(txHash);
      ret["result"]["transactionHash"] = std::string("0x") + Utils::bytesToHex(tx->hash().get());
      ret["result"]["transactionIndex"] = "0x" + dev::toHex(Utils::uint32ToBytes(tx->blockIndex()));
      auto block = chainHead->getBlockFromTx(txHash);
      ret["result"]["blockNumber"] = std::string("0x") + Utils::uintToHex(block->nHeight());
      ret["result"]["blockHash"] = std::string("0x") + dev::toHex(block->getBlockHash().get());
      ret["result"]["cumulativeGasUsed"] = "0x" + Utils::uintToHex(tx->gas());
      ret["result"]["gasUsed"] = "0x" + Utils::uintToHex(tx->gas());
      ret["result"]["contractAddress"] = "0x";  // TODO: does MetaMask check if we called a contract?
      ret["logs"] = json::array();
      ret["result"]["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
      ret["result"]["status"] = "0x1";
    } catch (std::exception &e) {
      Utils::LogPrint(Log::subnet, "eth_getTransactionReceipt: tx not found: ", e.what());
    }
  }
  if (messageJson["method"] == "eth_getBlockByHash") {
    Hash blockHash = Hash(Utils::hexToBytes(messageJson["params"][0].get<std::string>()));
    auto block = chainHead->getBlock(blockHash);
    if (block == nullptr) {
      ret["error"] = { {"code", -32000}, {"message", "Block not found"} };
      return ret.dump();
    }
    json answer;
    bool includeTxs = false;
    if (messageJson["params"].size() > 1) {
      includeTxs = messageJson["params"][1].get<bool>();
    }
    answer["number"] = std::string("0x") + Utils::uintToHex(block->nHeight());
    answer["hash"] = std::string("0x") + dev::toHex(block->getBlockHash().get());
    answer["parentHash"] = std::string("0x") + dev::toHex(block->prevBlockHash().get());
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
    answer["timestamp"] = std::string("0x") + Utils::uintToHex(block->timestampInSeconds()); // Seconds since epoch
    answer["transactions"] = json::array();
    for (auto tx : block->transactions()) {
      if (includeTxs) {
        // https://www.quicknode.com/docs/ethereum/eth_getTransactionByHash
        json transaction;
        transaction["hash"] = std::string("0x") + Utils::bytesToHex(tx.second.hash().get());
        transaction["nonce"] = std::string("0x") + Utils::uintToHex(tx.second.nonce());
        transaction["blockHash"] = std::string("0x") + Utils::bytesToHex(block->getBlockHash().get());
        transaction["blockNumber"] = std::string("0x") + Utils::uintToHex(block->nHeight());
        transaction["transactionIndex"] = std::string("0x") + Utils::uintToHex(tx.second.blockIndex());
        transaction["from"] = std::string("0x") + tx.second.from().hex();
        transaction["to"] = std::string("0x") + tx.second.to().hex();
        transaction["value"] = std::string("0x") + Utils::uintToHex(tx.second.value());
        transaction["gasPrice"] = std::string("0x") + Utils::uintToHex(tx.second.gasPrice());
        transaction["gas"] = std::string("0x") + Utils::uintToHex(tx.second.gas());
        transaction["input"] = std::string("0x") + Utils::bytesToHex(tx.second.data());
        transaction["v"] = std::string("0x") + Utils::uintToHex(tx.second.v());
        transaction["standardV"] = std::string("0x") + Utils::uintToHex(tx.second.recoverId());
        transaction["r"] = std::string("0x") + Utils::uintToHex(tx.second.r());
        transaction["raw"] = std::string("0x") + Utils::bytesToHex(tx.second.rlpSerialize(true));
        transaction["chainid"] = std::string("0x") + Utils::uintToHex(tx.second.chainId());
        answer["transactions"].push_back(transaction);
      } else {
        answer["transactions"].push_back(std::string("0x") + Utils::bytesToHex(tx.second.hash().get()));
      }
    }
    answer["uncles"] = json::array();
    ret["result"] = answer;
  }
  if (messageJson["method"] == "eth_call") {
    // TODO: Implement eth_call
    ret["result"] = "0x";
  }

  //
  // {
  //   "method" = "IncreaseBalance"
  //   "address" = "0x..."
  // }
  // Will increase the balance in 1 SUBS.
  if(messageJson["method"] == "IncreaseBalance") {
    Address address(messageJson["address"].get<std::string>(), true);
    this->headState->addBalance(address);
    ret["result"] = "SUCCESS";
  }
  return ret.dump();
}

