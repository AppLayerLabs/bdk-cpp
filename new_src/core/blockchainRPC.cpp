#include "blockchain.h"

// TODO: move this somewhere else outside of the Blockchain class? There are multiple edge cases that need to be handled.
std::string Blockchain::parseRPC(std::string& msg) {
  Utils::logToDebug(Log::blockchain, __func__, "Received RPC message: " + msg);
  json ret;
  try {
    json msgJson = json::parse(msg);
    ret["id"] = msgJson["id"];
    ret["jsonrpc"] = "2.0";
    // eth_blockNumber
    if (msgJson["method"] == "eth_blockNumber") {
      const std::shared_ptr<const Block> bestBlock = this->storage->latest();
      ret["result"] = "0x" + Utils::uintToHex(bestBlock->getNHeight());
      Utils::logToDebug(Log::blockchain, __func__,
        "eth_blockNumber: " + ret["result"].get<std::string>()
      );
    }
    // eth_chainId
    if (msgJson["method"] == "eth_chainId") ret["result"] = "0x2290";
    // net_version
    if (msgJson["method"] == "net_version") ret["result"] = "8848";
    // eth_getBalance
    if (msgJson["method"] == "eth_getBalance") {
      Address add(msgJson["params"][0].get<std::string>(), true);
      Utils::logToDebug(Log::blockchain, __func__, "eth_getBalance: ", add.hex());
      uint256_t balance = this->state->getNativeBalance(add);
      std::string hexValue = "0x";
      hexValue += Utils::uintToHex(balance);
      ret["result"] = hexValue;
      Utils::logToDebug(Log::blockchain, __func__, "eth_getBalance: ", ret.dump());
    }
    // eth_getBlockByNumber
    if (msgJson["method"] == "eth_getBlockByNumber") {
      std::string blockStr = msgJson["params"][0].get<std::string>();
      bool latest = (blockStr == "latest");
      uint64_t height = 0;
      if (blockStr != "latest") {
        height = boost::lexical_cast<uint64_t>(Utils::hexToUint(blockStr));
        Utils::logToDebug(Log::blockchain, __func__,
          "eth_getBlockByNumber: height ", std::to_string(height)
        );
        if (!this->storage->exists(height)) {
          ret["error"] = { {"code", -32000}, {"message", "Block not found"} };
          return ret.dump();
        }
      }

      const std::shared_ptr<const Block> block = (latest)
        ? this->storage->latest() : this->storage->getBlock(height);
      if (block == nullptr) {
        ret["error"] = { {"code", -32000}, {"message", "Block not found"} };
        return ret.dump();
      }
      Utils::logToDebug(Log::blockchain, __func__,
        "eth_getBlockByNumber: block ", dev::toHex(block->serializeToBytes(true))
      );
      bool includeTxs = (msgJson["params"].size() > 1) ? msgJson["params"][1].get<bool>() : false;

      json answer;
      answer["number"] = std::string("0x") + Utils::uintToHex(block->getNHeight());
      answer["hash"] = std::string("0x") + Utils::bytesToHex(block->getBlockHash().get());
      answer["parentHash"] = std::string("0x") + Utils::bytesToHex(block->getPrevBlockHash().get());
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
      for (auto tx : block->getTxs()) {
        if (includeTxs) {
          // https://www.quicknode.com/docs/ethereum/eth_getTransactionByHash
          json txJson;
          txJson["hash"] = std::string("0x") + Utils::bytesToHex(tx.second.hash().get());
          txJson["nonce"] = std::string("0x") + Utils::uintToHex(tx.second.getNonce());
          txJson["blockHash"] = std::string("0x") + Utils::bytesToHex(block->getBlockHash().get());
          txJson["blockNumber"] = std::string("0x") + Utils::uintToHex(block->getNHeight());
          txJson["txJsonIndex"] = std::string("0x") + Utils::uintToHex(tx.second.blockIndex());
          txJson["from"] = std::string("0x") + tx.second.getFrom().hex();
          txJson["to"] = std::string("0x") + tx.second.getTo().hex();
          txJson["value"] = std::string("0x") + Utils::uintToHex(tx.second.getValue());
          txJson["gasPrice"] = std::string("0x") + Utils::uintToHex(tx.second.getGasPrice());
          txJson["gas"] = std::string("0x") + Utils::uintToHex(tx.second.getGas());
          txJson["input"] = std::string("0x") + Utils::bytesToHex(tx.second.getData());
          txJson["v"] = std::string("0x") + Utils::uintToHex(tx.second.getV());
          txJson["standardV"] = std::string("0x") + Utils::uintToHex(tx.second.recoverId());
          txJson["r"] = std::string("0x") + Utils::uintToHex(tx.second.getR());
          txJson["raw"] = std::string("0x") + Utils::bytesToHex(tx.second.rlpSerialize(true));
          txJson["chainid"] = std::string("0x") + Utils::uintToHex(tx.second.getChainId());
          answer["transactions"].push_back(txJson);
        } else {
          answer["transactions"].push_back(std::string("0x") + Utils::bytesToHex(tx.second.hash().get()));
        }
      }
      answer["uncles"] = json::array();
      ret["result"] = answer;
      Utils::logToDebug(Log::blockchain, __func__, "eth_getBlockByNumber: ", ret.dump());
    }
    // eth_getCode
    if (msgJson["method"] == "eth_getCode") ret["result"] = "0x";
    // eth_gasPrice
    if (msgJson["method"] == "eth_gasPrice") ret["result"] = "0x12a05f200"; // Force to 5 Gwei
    // eth_estimateGas
    if (msgJson["method"] == "eth_estimateGas") ret["result"] = "0x5208"; // Force to 21000 Wei
    // eth_getTransactionCount
    if (msgJson["method"] == "eth_getTransactionCount") {
      Address add(msgJson["params"][0].get<std::string>(), true);
      uint256_t nonce = this->state->getNativeNonce(add);
      ret["result"] = std::string("0x") + Utils::uintToHex(nonce);
    }
    // eth_sendRawTransaction
    if (msgJson["method"] == "eth_sendRawTransaction") {
      std::string txRlp = msgJson["params"][0].get<std::string>();
      Utils::patchHex(txRlp);
      try {
        std::string txStr = Utils::hexToBytes(txRlp);
        TxBase tx(txStr, false);
        std::pair<int, std::string> txRet = this->validateTx(std::move(tx));
        if (txRet.first != 0) {
          ret["error"] = json({{"code", txRet.first}, {"message", txRet.second}});
        }
        ret["result"] = std::string("0x") + tx.hash().hex();
      } catch (std::exception &e) {
        Utils::logToDebug(Log::blockchain, __func__,
          std::string("sendRawTransaction: failed! ") + e.what()
        );
      }
    }
    // eth_getTransactionReceipt
    if (msgJson["method"] == "eth_getTransactionReceipt") {
      Hash txHash = Hash(Utils::hexToBytes(msgJson["params"][0].get<std::string>()));
      try {
        const std::shared_ptr<const TxBlock> tx = this->storage->getTx(txHash);
        ret["result"]["transactionHash"] = std::string("0x") + Utils::bytesToHex(tx->hash().get());
        ret["result"]["transactionIndex"] = "0x" + dev::toHex(Utils::uint32ToBytes(tx->blockIndex()));
        auto block = this->storage->getBlockFromTx(txHash);
        ret["result"]["blockNumber"] = std::string("0x") + Utils::uintToHex(block->getNHeight());
        ret["result"]["blockHash"] = std::string("0x") + dev::toHex(block->getBlockHash().get());
        ret["result"]["cumulativeGasUsed"] = "0x" + Utils::uintToHex(tx->getGas());
        ret["result"]["gasUsed"] = "0x" + Utils::uintToHex(tx->getGas());
        ret["result"]["contractAddress"] = "0x";  // TODO: does MetaMask check if we called a contract?
        ret["logs"] = json::array();
        ret["result"]["logsBloom"] = "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
        ret["result"]["status"] = "0x1";
      } catch (std::exception &e) {
        Utils::logToDebug(Log::blockchain, __func__,
          std::string("eth_getTransactionReceipt: tx not found: "), e.what()
        );
      }
    }
    // eth_getBlockByHash
    if (msgJson["method"] == "eth_getBlockByHash") {
      Hash blockHash = Hash(Utils::hexToBytes(msgJson["params"][0].get<std::string>()));
      const std::shared_ptr<const Block> block = this->storage->getBlock(blockHash);
      if (block == nullptr) {
        ret["error"] = { {"code", -32000}, {"message", "Block not found"} };
        return ret.dump();
      }
      bool includeTxs = (msgJson["params"].size() > 1) ? msgJson["params"][1].get<bool>() : false;

      json answer;
      answer["number"] = std::string("0x") + Utils::uintToHex(block->getNHeight());
      answer["hash"] = std::string("0x") + dev::toHex(block->getBlockHash().get());
      answer["parentHash"] = std::string("0x") + dev::toHex(block->getPrevBlockHash().get());
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
      for (auto tx : block->getTxs()) {
        if (includeTxs) {
          // https://www.quicknode.com/docs/ethereum/eth_getTransactionByHash
          json txJson;
          txJson["hash"] = std::string("0x") + Utils::bytesToHex(tx.second.hash().get());
          txJson["nonce"] = std::string("0x") + Utils::uintToHex(tx.second.getNonce());
          txJson["blockHash"] = std::string("0x") + Utils::bytesToHex(block->getBlockHash().get());
          txJson["blockNumber"] = std::string("0x") + Utils::uintToHex(block->getNHeight());
          txJson["txJsonIndex"] = std::string("0x") + Utils::uintToHex(tx.second.blockIndex());
          txJson["from"] = std::string("0x") + tx.second.getFrom().hex();
          txJson["to"] = std::string("0x") + tx.second.getTo().hex();
          txJson["value"] = std::string("0x") + Utils::uintToHex(tx.second.getValue());
          txJson["gasPrice"] = std::string("0x") + Utils::uintToHex(tx.second.getGasPrice());
          txJson["gas"] = std::string("0x") + Utils::uintToHex(tx.second.getGas());
          txJson["input"] = std::string("0x") + Utils::bytesToHex(tx.second.getData());
          txJson["v"] = std::string("0x") + Utils::uintToHex(tx.second.getV());
          txJson["standardV"] = std::string("0x") + Utils::uintToHex(tx.second.recoverId());
          txJson["r"] = std::string("0x") + Utils::uintToHex(tx.second.getR());
          txJson["raw"] = std::string("0x") + Utils::bytesToHex(tx.second.rlpSerialize(true));
          txJson["chainid"] = std::string("0x") + Utils::uintToHex(tx.second.getChainId());
          answer["transactions"].push_back(txJson);
        } else {
          answer["transactions"].push_back(std::string("0x") + Utils::bytesToHex(tx.second.hash().get()));
        }
      }
      answer["uncles"] = json::array();
      ret["result"] = answer;
    }
    // eth_call
    // TODO: Implement eth_call
    if (msgJson["method"] == "eth_call") ret["result"] = "0x";
    /**
     * IncreaseBalance
     * { "method" = "IncreaseBalance" "address" = "0x..." }
     * Will increase the balance in 1 SUBS. For testing purposes only.
     * TODO: this should be removed on release
     */
    if (msgJson["method"] == "IncreaseBalance") {
      Address add(msgJson["address"].get<std::string>(), true);
      this->state->addBalance(add);
      ret["result"] = "SUCCESS";
    }
    // getPeerList
    if (msgJson["method"] == "getPeerList") {
      json clientsJson = json::array();
      json serversJson = json::array();
      for (const auto& i : this->p2p->getConnServers()) {
        json conn = {
          {"host", i.getHost().to_string()},
          {"port", i.getPort()},
          {"version", i.getInfo().version},
          {"timestamp", i.getInfo().timestamp},
          {"latestBlockHeight", i.getInfo().latestBlockHeight},
          {"latestBlockHash", i.getInfo().latestBlockHash.hex()},
          {"nodes", i.getInfo().nodes},
          {"lastNodeCheck", i.getInfo().lastNodeCheck},
          {"clockDiff", i.getInfo().clockDiff}
        };
        clientsJson.push_back(conn);
      }
      for (const auto &i : this->p2p->getConnClients()) {
        json conn = {
          {"host", i.getHost().to_string()},
          {"port", i.getPort()},
          {"version", i.getInfo().version},
          {"timestamp", i.getInfo().timestamp},
          {"latestBlockHeight", i.getInfo().latestBlockHeight},
          {"latestBlockHash", i.getInfo().latestBlockHash.hex()},
          {"nodes", i.getInfo().nodes},
          {"lastNodeCheck", i.getInfo().lastNodeCheck},
          {"clockDiff", i.getInfo().clockDiff}
        };
        serversJson.push_back(conn);
      }
      ret["result"]["outbound"] = clientsJson;
      ret["result"]["inbound"] = serversJson;
    }
  } catch (std::exception &e) {
    ret["error"] = std::string("Exception: ") + e.what();
  }
  return ret.dump();
}

