/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "decoding.h"
#include "../../../core/storage.h"

namespace JsonRPC::Decoding {
  // https://www.jsonrpc.org/specification
  bool checkJsonRPCSpec(const json& request) {
    try {
      // "jsonrpc": "2.0" is a MUST
      if (!request.contains("jsonrpc")) return false;
      if (request["jsonrpc"].get<std::string>() != "2.0") return false;

      // "method" is a MUST
      if (!request.contains("method")) return false;

      // Params MUST be Object or Array.
      if (
        request.contains("params") && (!request["params"].is_object() && !request["params"].is_array())
      ) return false;

      return true;
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while checking json RPC spec: ") + e.what()
      );
      throw DynamicException("Error while checking json RPC spec: " + std::string(e.what()));
    }
  }

  Methods getMethod(const json& request) {
    try {
      const std::string& method = request["method"].get<std::string>();
      auto it = methodsLookupTable.find(method);
      if (it == methodsLookupTable.end()) return Methods::invalid;
      return it->second;
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while getting method: ") + e.what()
      );
      throw DynamicException("Error while checking json RPC spec: " + std::string(e.what()));
    }
  }

  void web3_clientVersion(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException(
        "web3_clientVersion does not need params"
      );
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding web3_clientVersion: ") + e.what()
      );
      throw DynamicException(
        "Error while decoding web3_clientVersion: " + std::string(e.what())
      );
    }
  }

  Bytes web3_sha3(const json& request) {
    try {
      // Data to hash will always be at index 0.
      if (request["params"].size() != 1) throw DynamicException(
        "web3_sha3 needs 1 param"
      );
      std::string data = request["params"].at(0).get<std::string>();
      if (!Hex::isValid(data, true)) throw DynamicException("Invalid hex string");
      return Hex::toBytes(data);
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding web3_sha3: ") + e.what()
      );
      throw DynamicException("Error while decoding web3_sha3: " + std::string(e.what()));
    }
  }

  void net_version(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException(
        "net_version does not need params"
      );
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding net_version: ") + e.what()
      );
      throw DynamicException("Error while decoding net_version: " + std::string(e.what()));
    }
  }

  void net_listening(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException(
        "net_listening does not need params"
      );
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding net_listening: ") + e.what()
      );
      throw DynamicException("Error while decoding net_listening: " + std::string(e.what()));
    }
  }

  void net_peerCount(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException(
        "net_peerCount does not need params"
      );
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding net_peerCount: ") + e.what()
      );
      throw DynamicException("Error while decoding net_peerCount: " + std::string(e.what()));
    }
  }

  void eth_protocolVersion(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException(
        "eth_protocolVersion does not need params"
      );
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_protocolVersion: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_protocolVersion: " + std::string(e.what()));
    }
  }

  std::pair<Hash,bool> eth_getBlockByHash(const json& request) {
    static const std::regex hashFilter("^0x[0-9a-f]{64}$");
    try {
      bool includeTxs = (request["params"].size() == 2) ? request["params"].at(1).get<bool>() : false;
      std::string blockHash = request["params"].at(0).get<std::string>();
      if (!std::regex_match(blockHash, hashFilter)) throw DynamicException("Invalid block hash hex");
      return std::make_pair(Hash(Hex::toBytes(blockHash)), includeTxs);
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getBlockByHash: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_getBlockByHash: " + std::string(e.what()));
    }
  }

  std::pair<uint64_t,bool> eth_getBlockByNumber(
    const json& request, const Storage& storage
  ) {
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    try {
      bool includeTxs = (request["params"].size() == 2) ? request["params"].at(1).get<bool>() : false;
      // eth_getBlockByNumber has flags for its params instead of hex numbers.
      std::string blockNum = request["params"].at(0).get<std::string>();
      if (blockNum == "latest") return std::make_pair(storage.latest()->getNHeight(), includeTxs);
      if (blockNum == "earliest") return std::make_pair(0, includeTxs);
      if (blockNum == "pending") throw DynamicException("Pending block is not supported");
      if (!std::regex_match(blockNum, numFilter)) throw DynamicException("Invalid block hash hex");
      return std::make_pair(uint64_t(Hex(blockNum).getUint()), includeTxs);
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getBlockByNumber: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_getBlockByNumber: " + std::string(e.what()));
    }
  }

  Hash eth_getBlockTransactionCountByHash(const json& request) {
    static const std::regex hashFilter("^0x[0-9a-f]{64}$");
    try {
      // Check block hash.
      std::string blockHash = request["params"].at(0).get<std::string>();
      if (!std::regex_match(blockHash, hashFilter)) throw DynamicException("Invalid block hash hex");
      return Hash(Hex::toBytes(blockHash));
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getBlockTransactionCountByHash: ") + e.what()
      );
      throw DynamicException(
        "Error while decoding eth_getBlockTransactionCountByHash: " + std::string(e.what())
      );
    }
  }

  uint64_t eth_getBlockTransactionCountByNumber(const json& request, const Storage& storage) {
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    try {
      // eth_getBlockTransactionCountByNumber has flags for its params instead of hex numbers.
      std::string blockNum = request["params"].at(0).get<std::string>();
      if (blockNum == "latest") return storage.latest()->getNHeight();
      if (blockNum == "earliest") return 0;
      if (blockNum == "pending") throw DynamicException("Pending block is not supported");
      if (!std::regex_match(blockNum, numFilter)) throw DynamicException("Invalid block hash hex");
      return uint64_t(Hex(blockNum).getUint());
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getBlockTransactionCountByNumber: ") + e.what()
      );
      throw DynamicException(
        "Error while decoding eth_getBlockTransactionCountByNumber: " + std::string(e.what())
      );
    }
  }

  void eth_chainId(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException("eth_chainId does not need params");
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_chainId: ") + e.what());
      throw DynamicException("Error while decoding eth_chainId: " + std::string(e.what()));
    }
  }

  void eth_syncing(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException("eth_syncing does not need params");
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_syncing: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_syncing: " + std::string(e.what()));
    }
  }

  void eth_coinbase(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException("eth_coinbase does not need params");
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_coinbase: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_coinbase: " + std::string(e.what()));
    }
  }

  void eth_blockNumber(const json& request) {
    try {
      // No params are needed.
      if (!request["params"].empty()) throw DynamicException("eth_blockNumber does not need params");
      return;
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_blockNumber: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_blockNumber: " + std::string(e.what()));
    }
  }

  evmc_message eth_call(const json& request, const Storage& storage, Bytes& fullData) {
    evmc_message result;
    auto& [kind, flags, depth, gasLimit, recipient, sender, input_data, input_size, value, create2_salt, code_address] = result;
    kind = EVMC_CALL;
    flags = 0;
    depth = 0;

    static const std::regex addFilter("^0x[0-9,a-f,A-F]{40}$");
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    try {
      json txObj;
      if (request["params"].is_array()) {
        txObj = request["params"].at(0);
        if (request["params"].size() > 1) {
          const auto block = request["params"].at(1).get<std::string>();
          if (block != "latest") {
            if (!std::regex_match(block, numFilter)) throw DynamicException(
              "Invalid block number"
            );
            auto blockNum = uint64_t(Hex(block).getUint());
            if (blockNum != storage.latest()->getNHeight()) throw DynamicException(
              "Only latest block is supported"
            );
          }
        }
      } else if (request["params"].is_object()) {
        txObj = request["params"];
      } else {
        throw DynamicException("Invalid params");
      }

      // Optional: Check from address
      if (txObj.contains("from") && !txObj["from"].is_null()) {
        std::string fromAdd = txObj["from"].get<std::string>();
        if (!std::regex_match(fromAdd, addFilter)) throw DynamicException("Invalid from address hex");
        sender = Address(Hex::toBytes(fromAdd)).toEvmcAddress();
      } else {
        sender = {};
      }
      // Check to address
      std::string toAdd = txObj["to"].get<std::string>();
      if (!std::regex_match(toAdd, addFilter)) throw DynamicException("Invalid to address hex");
      recipient = Address(Hex::toBytes(toAdd)).toEvmcAddress();
      if (evmc::is_zero(recipient)) kind = EVMC_CREATE;
      // Optional: Check gas
      if (txObj.contains("gas") && !txObj["gas"].is_null()) {
        std::string gasHex = txObj["gas"].get<std::string>();
        if (!std::regex_match(gasHex, numFilter)) throw DynamicException("Invalid gas hex");
        gasLimit = int64_t(Hex(gasHex).getUint());
      } else {
        gasLimit = 10000000;
      }
      // Optional: Check gasPrice
      if (txObj.contains("gasPrice") && !txObj["gasPrice"].is_null()) {
        std::string gasPriceHex = txObj["gasPrice"].get<std::string>();
        if (!std::regex_match(gasPriceHex, numFilter)) throw DynamicException("Invalid gasPrice hex");
        // We actually don't give a damn about the gas price, chain is fixed at 1 GWEI
      }
      // Optional: Check value
      if (txObj.contains("value") && !txObj["value"].is_null()) {
        std::string valueHex = txObj["value"].get<std::string>();
        if (!std::regex_match(valueHex, numFilter)) throw DynamicException("Invalid value hex");
        value = Utils::uint256ToEvmcUint256(uint256_t(Hex(valueHex).getUint()));
      } else {
        value = {};
      }
      // Optional: Check data
      if (txObj.contains("data") && !txObj["data"].is_null()) {
        std::string dataHex = txObj["data"].get<std::string>();
        if (!Hex::isValid(dataHex, true)) throw DynamicException("Invalid data hex");
        fullData = Hex::toBytes(dataHex);
        input_data = fullData.data();
        input_size = fullData.size();
      } else {
        input_data == nullptr;
        input_size = 0;
      }
      return result;
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_call: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_call: " + std::string(e.what()));
    }
  }

  evmc_message eth_estimateGas(const json& request, const Storage& storage, Bytes& fullData) {
    evmc_message result;
    auto& [kind, flags, depth, gasLimit, recipient, sender, input_data, input_size, value, create2_salt, code_address] = result;
    static const std::regex addFilter("^0x[0-9,a-f,A-F]{40}$");
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    flags = 0;
    depth = 0;
    try {
      json txObj;
      if (request["params"].is_array()) {
        txObj = request["params"].at(0);
        if (request["params"].size() > 1) {
          const auto block = request["params"].at(1).get<std::string>();
          if (block != "latest") {
            if (!std::regex_match(block, numFilter)) throw DynamicException(
              "Invalid block number"
            );
            auto blockNum = uint64_t(Hex(block).getUint());
            if (blockNum != storage.latest()->getNHeight()) throw DynamicException(
              "Only latest block is supported"
            );
          }
        }
      } else if (request["params"].is_object()) {
        txObj = request["params"];
      } else {
        throw DynamicException("Invalid params");
      }

      // Optional: Check from address
      if (txObj.contains("from") && !txObj["from"].is_null()) {
        std::string fromAdd = txObj["from"].get<std::string>();
        if (!std::regex_match(fromAdd, addFilter)) throw DynamicException("Invalid from address hex");
        sender = Address(Hex::toBytes(fromAdd)).toEvmcAddress();
      } else {
        sender = {};
      }
      // Optional: Check to address
      if (txObj.contains("to") && !txObj["to"].is_null()) {
        std::string toAdd = txObj["to"].get<std::string>();
        if (!std::regex_match(toAdd, addFilter)) throw DynamicException("Invalid to address hex");
        recipient = Address(Hex::toBytes(toAdd)).toEvmcAddress();
      } else {
        recipient = {};
      }

      if (evmc::is_zero(recipient)) kind = EVMC_CREATE;

      // Optional: Check gas
      if (txObj.contains("gas") && !txObj["gas"].is_null()) {
        std::string gasHex = txObj["gas"].get<std::string>();
        if (!std::regex_match(gasHex, numFilter)) throw DynamicException("Invalid gas hex");
        gasLimit = uint64_t(Hex(gasHex).getUint());
      } else { // eth_estimateGas set gas to max if not specified
        gasLimit = 100000000;
      }
      // Optional: Check gasPrice
      if (txObj.contains("gasPrice") && !txObj["gasPrice"].is_null()) {
        std::string gasPriceHex = txObj["gasPrice"].get<std::string>();
        if (!std::regex_match(gasPriceHex, numFilter)) throw DynamicException("Invalid gasPrice hex");
        // We actually don't give a damn about the gas price, chain is fixed at 1 GWEI
      }
      // Optional: Check value
      if (txObj.contains("value") && !txObj["value"].is_null()) {
        std::string valueHex = txObj["value"].get<std::string>();
        if (!std::regex_match(valueHex, numFilter)) throw DynamicException("Invalid value hex");
        value = Utils::uint256ToEvmcUint256(uint256_t(Hex(valueHex).getUint()));
      } else {
        value = {};
      }
      // Optional: Check data
      if (txObj.contains("data") && !txObj["data"].is_null()) {
        std::string dataHex = txObj["data"].get<std::string>();
        if (!Hex::isValid(dataHex, true)) throw DynamicException("Invalid data hex");
        fullData = Hex::toBytes(dataHex);
        input_data = fullData.data();
        input_size = fullData.size();
      } else {
        input_data = nullptr;
        input_size = 0;
      }
      return result;
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_estimateGas: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_estimateGas: " + std::string(e.what()));
    }
  }

  void eth_gasPrice(const json& request) {
    try {
      if (!request["params"].empty()) throw DynamicException("eth_gasPrice does not need params");
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_gasPrice: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_gasPrice: " + std::string(e.what()));
    }
  }

  std::tuple<uint64_t, uint64_t, Address, std::vector<Hash>> eth_getLogs(
    const json& request, const Storage& storage
  ) {
    static const std::regex addFilter("^0x[0-9,a-f,A-F]{40}$");
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    static const std::regex hashFilter("^0x[0-9a-f]{64}$");
    try {
      uint64_t fromBlock = ContractGlobals::getBlockHeight(); // "latest" by default
      uint64_t toBlock = ContractGlobals::getBlockHeight(); // "latest" by default
      auto address = Address();  // Empty by default
      std::vector<Hash> topics = {}; // Empty by default
      json logsObject = request["params"].at(0);

      if (logsObject.contains("blockHash")) {
        std::string blockHashHex = logsObject["blockHash"].get<std::string>();
        if (!std::regex_match(blockHashHex, hashFilter)) throw DynamicException("Invalid block hash hex");
        const std::shared_ptr<const FinalizedBlock> block = storage.getBlock(Hash(Hex::toBytes(blockHashHex)));
        fromBlock = toBlock = block->getNHeight();
      } else {
        if (logsObject.contains("fromBlock")) {
          std::string fromBlockHex = logsObject["fromBlock"].get<std::string>();
          if (fromBlockHex == "latest") {
            fromBlock = storage.latest()->getNHeight();
          } else if (fromBlockHex == "earliest") {
            fromBlock = 0;
          } else if (fromBlockHex == "pending") {
            throw DynamicException("Pending block is not supported");
          } else if (std::regex_match(fromBlockHex, numFilter)) {
            fromBlock = uint64_t(Hex(fromBlockHex).getUint());
          } else {
            throw DynamicException("Invalid fromBlock hex");
          }
        }
        if (logsObject.contains("toBlock")) {
          std::string toBlockHex = logsObject["toBlock"].get<std::string>();
          if (toBlockHex == "latest") {
            toBlock = storage.latest()->getNHeight();
          } else if (toBlockHex == "earliest") {
            toBlock = 0;
          } else if (toBlockHex == "pending") {
            throw DynamicException("Pending block is not supported");
          } else if (std::regex_match(toBlockHex, numFilter)) {
            toBlock = uint64_t(Hex(toBlockHex).getUint());
          } else {
            throw DynamicException("Invalid fromBlock hex");
          }
        }
      }

      if (logsObject.contains("address")) {
        std::string addressHex = logsObject["address"].get<std::string>();
        if (!std::regex_match(addressHex, addFilter)) throw DynamicException("Invalid address hex");
        address = Address(Hex::toBytes(addressHex));
      }

      if (logsObject.contains("topics")) {
        if (!logsObject.at("topics").is_array()) {
          throw DynamicException("topics is not an array");
        }
        auto topicsArray = logsObject.at("topics").get<std::vector<std::string>>();
        for (const auto& topic : topicsArray) {
          if (!std::regex_match(topic, hashFilter)) throw DynamicException("Invalid topic hex");
          topics.emplace_back(Hex::toBytes(topic));
        }
      }

      return std::make_tuple(fromBlock, toBlock, address, topics);
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getLogs: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_getLogs: " + std::string(e.what()));
    }
  }

  Address eth_getBalance(const json& request, const Storage& storage) {
    static const std::regex addFilter("^0x[0-9,a-f,A-F]{40}$");
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    try {
      const auto address = request["params"].at(0).get<std::string>();
      const auto block = request["params"].at(1).get<std::string>();
      if (block != "latest") {
        if (!std::regex_match(block, numFilter)) throw DynamicException(
          "Invalid block number"
        );
        auto blockNum = uint64_t(Hex(block).getUint());
        if (blockNum != storage.latest()->getNHeight()) throw DynamicException(
          "Only latest block is supported"
        );
      }
      if (!std::regex_match(address, addFilter)) throw DynamicException("Invalid address hex");
      return Address(Hex::toBytes(address));
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getBalance: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_getBalance: " + std::string(e.what()));
    }
  }

  Address eth_getTransactionCount(const json& request, const Storage& storage) {
    static const std::regex addFilter("^0x[0-9,a-f,A-F]{40}$");
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    try {
      const auto address = request["params"].at(0).get<std::string>();
      const auto block = request["params"].at(1).get<std::string>();
      if (block != "latest") {
        if (!std::regex_match(block, numFilter)) throw DynamicException(
          "Invalid block number"
        );
        auto blockNum = uint64_t(Hex(block).getUint());
        if (blockNum != storage.latest()->getNHeight()) throw DynamicException(
          "Only latest block is supported"
        );
      }
      if (!std::regex_match(address, addFilter)) throw DynamicException("Invalid address hex");
      return Address(Hex::toBytes(address));
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getTransactionCount: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_getTransactionCount: " + std::string(e.what()));
    }
  }

  Address eth_getCode(const json& request, const Storage& storage) {
    static const std::regex addFilter("^0x[0-9,a-f,A-F]{40}$");
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    try {
      const auto address = request["params"].at(0).get<std::string>();
      const auto block = request["params"].at(1).get<std::string>();
      if (block != "latest") {
        if (!std::regex_match(block, numFilter)) throw DynamicException(
          "Invalid block number"
        );
        auto blockNum = uint64_t(Hex(block).getUint());
        if (blockNum != storage.latest()->getNHeight()) throw DynamicException(
          "Only latest block is supported"
        );
      }
      if (!std::regex_match(address, addFilter)) throw DynamicException("Invalid address hex");
      return Address(Hex::toBytes(address));
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getCode: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_getCode: " + std::string(e.what()));
    }
  }

  TxBlock eth_sendRawTransaction(const json& request, const uint64_t& requiredChainId) {
    try {
      const auto txHex = request["params"].at(0).get<std::string>();
      if (!Hex::isValid(txHex, true)) throw DynamicException("Invalid transaction hex");
      return TxBlock(Hex::toBytes(txHex), requiredChainId);
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_sendRawTransaction: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_sendRawTransaction: " + std::string(e.what()));
    }
  }

  Hash eth_getTransactionByHash(const json& request) {
    static const std::regex hashFilter("^0x[0-9,a-f,A-F]{64}$");
    try {
      const auto hash = request["params"].at(0).get<std::string>();
      if (!std::regex_match(hash, hashFilter)) throw DynamicException("Invalid hash hex");
      return Hash(Hex::toBytes(hash));
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getTransactionByHash: ") + e.what()
      );
      throw DynamicException("Error while decoding eth_getTransactionByHash: " + std::string(e.what()));
    }
  }

  std::pair<Hash,uint64_t> eth_getTransactionByBlockHashAndIndex(const json& request) {
    static const std::regex hashFilter("^0x[0-9,a-f,A-F]{64}$");
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    try {
      std::string blockHash = request["params"].at(0).get<std::string>();
      std::string index = request["params"].at(1).get<std::string>();
      if (!std::regex_match(blockHash, hashFilter)) throw DynamicException("Invalid blockHash hex");
      if (!std::regex_match(index, numFilter)) throw DynamicException("Invalid index hex");
      return std::make_pair(Hash(Hex::toBytes(blockHash)), uint64_t(Hex(index).getUint()));
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getTransactionByBlockHashAndIndex: ") + e.what()
      );
      throw DynamicException(
        "Error while decoding eth_getTransactionByBlockHashAndIndex: " + std::string(e.what())
      );
    }
  }

  std::pair<uint64_t,uint64_t> eth_getTransactionByBlockNumberAndIndex(
    const json& request, const Storage& storage
  ) {
    static const std::regex numFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
    try {
      std::string blockNum = request["params"].at(0).get<std::string>();
      std::string index = request["params"].at(1).get<std::string>();
      if (!std::regex_match(index, numFilter)) throw DynamicException("Invalid index hex");
      if (blockNum == "latest") return std::make_pair(
        storage.latest()->getNHeight(), uint64_t(Hex(index).getUint())
      );
      if (!std::regex_match(blockNum, numFilter)) throw DynamicException("Invalid blockNumber hex");
      return std::make_pair<uint64_t,uint64_t>(
        uint64_t(Hex(blockNum).getUint()), uint64_t(Hex(index).getUint())
      );
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getTransactionByBlockNumberAndIndex: ") + e.what()
      );
      throw DynamicException(
        "Error while decoding eth_getTransactionByBlockNumberAndIndex: " + std::string(e.what())
      );
    }
  }

  Hash eth_getTransactionReceipt(const json& request) {
    static const std::regex hashFilter("^0x[0-9,a-f,A-F]{64}$");
    try {
      std::string txHash = request["params"].at(0).get<std::string>();
      if (!std::regex_match(txHash, hashFilter)) throw DynamicException("Invalid Hex: " + txHash);
      return Hash(Hex::toBytes(txHash));
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding eth_getTransactionReceipt: ") + e.what()
      );
      throw DynamicException(
        "Error while decoding eth_getTransactionReceipt: " + std::string(e.what())
      );
    }
  }
}

