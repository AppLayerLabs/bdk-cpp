#include "decoding.h"
#include "../../../core/storage.h"

namespace JsonRPC {
  namespace Decoding {

    /// https://www.jsonrpc.org/specification
    bool checkJsonRPCSpec(const json& request) {
      try {
        /// "jsonrpc": "2.0" is a MUST
        if (!request.contains("jsonrpc")) return false;
        if (request["jsonrpc"].get<std::string>() != "2.0") return false;

        /// "method" is a MUST
        if (!request.contains("method")) return false;

        /// Params MUST be Object or Array.
        if (request.contains("params")) {
          if (!request["params"].is_object() && !request["params"].is_array()) return false;
        }

        return true;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while checking json RPC spec: ") + e.what());
        throw std::runtime_error("Error while checking json RPC spec: " + std::string(e.what()));
      }
    }

    Methods getMethod(const json& request) {
      try {
        const std::string& method = request["method"].get<std::string>();
        auto it = methodsLookupTable.find(method);
        if (it == methodsLookupTable.end()) return Methods::invalid;
        return it->second;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while getting method: ") + e.what());
        throw std::runtime_error("Error while checking json RPC spec: " + std::string(e.what()));
      }
    }

    void web3_clientVersion(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("web3_clientVersion does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding web3_clientVersion: ") + e.what());
        throw std::runtime_error("Error while decoding web3_clientVersion: " + std::string(e.what()));
      }
    }

    std::string web3_sha3(const json& request) {
      try {
        /// Data to hash will always be at index 0.
        if (request["params"].size() != 1) {
          throw std::runtime_error("web3_sha3 needs 1 param");
        }
        std::string data = request["params"].at(0).get<std::string>();
        if (!Hex::isValid(data, true)) {
          throw std::runtime_error("Invalid hex string");
        }
        return Hex::toBytes(data);
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding web3_sha3: ") + e.what());
        throw std::runtime_error("Error while decoding web3_sha3: " + std::string(e.what()));
      }
    }

    void net_version(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("net_version does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding net_version: ") + e.what());
        throw std::runtime_error("Error while decoding net_version: " + std::string(e.what()));
      }
    }

    void net_listening(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("net_listening does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding net_listening: ") + e.what());
        throw std::runtime_error("Error while decoding net_listening: " + std::string(e.what()));
      }
    }

    void net_peerCount(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("net_peerCount does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding net_peerCount: ") + e.what());
        throw std::runtime_error("Error while decoding net_peerCount: " + std::string(e.what()));
      }
    }

    void eth_protocolVersion(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("eth_protocolVersion does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_protocolVersion: ") + e.what());
        throw std::runtime_error("Error while decoding eth_protocolVersion: " + std::string(e.what()));
      }
    }

    std::pair<Hash,bool> eth_getBlockByHash(const json& request) {
      static const std::regex hashFilter("^0x[0-9a-f]{64}$");
      try {
        /// Defaults to false if not specified.
        bool includeTransactions = false;
        if (request["params"].size() == 2) {
          includeTransactions = request["params"].at(1).get<bool>();
        }
        /// Check block hash.
        std::string blockHash = request["params"].at(0).get<std::string>();
        if (!std::regex_match(blockHash, hashFilter)) {
          throw std::runtime_error("Invalid block hash hex");
        }
        return std::make_pair(Hash(Hex::toBytes(blockHash)), includeTransactions);
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getBlockByHash: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getBlockByHash: " + std::string(e.what()));
      }
    }

    std::pair<uint64_t,bool> eth_getBlockByNumber(const json& request, const std::unique_ptr<Storage> &storage) {
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        /// Defaults to false if not specified
        bool includeTransactions = false;
        if (request["params"].size() == 2) {
          includeTransactions = request["params"].at(1).get<bool>();
        }

        /// eth_getBlockByNumber support flags for it params instead of hex number.
        std::string blockNumber = request["params"].at(0).get<std::string>();
        if (blockNumber == "latest") {
          return std::make_pair(storage->latest()->getNHeight(), includeTransactions);
        }
        if (blockNumber == "earliest") {
          return std::make_pair(0, includeTransactions);
        }
        if (blockNumber == "pending") {
          throw std::runtime_error("Pending block is not supported");
        }
        if (!std::regex_match(blockNumber, numberFilter)) {
          throw std::runtime_error("Invalid block hash hex");
        }
        return std::make_pair(uint64_t(Hex(blockNumber).getUint()), includeTransactions);
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getBlockByNumber: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getBlockByNumber: " + std::string(e.what()));
      }
    }

    Hash eth_getBlockTransactionCountByHash(const json& request) {
      static const std::regex hashFilter("^0x[0-9a-f]{64}$");
      try {
        /// Check block hash.
        std::string blockHash = request["params"].at(0).get<std::string>();
        if (!std::regex_match(blockHash, hashFilter)) {
          throw std::runtime_error("Invalid block hash hex");
        }
        return Hash(Hex::toBytes(blockHash));
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getBlockTransactionCountByHash: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getBlockTransactionCountByHash: " + std::string(e.what()));
      }
    }

    uint64_t eth_getBlockTransactionCountByNumber(const json& request, const std::unique_ptr<Storage> &storage) {
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        /// eth_getBlockTransactionCountByNumber support flags for it params instead of hex number.
        std::string blockNumber = request["params"].at(0).get<std::string>();
        if (blockNumber == "latest") {
          return storage->latest()->getNHeight();
        }
        if (blockNumber == "earliest") {
          return 0;
        }
        if (blockNumber == "pending") {
          throw std::runtime_error("Pending block is not supported");
        }
        if (!std::regex_match(blockNumber, numberFilter)) {
          throw std::runtime_error("Invalid block hash hex");
        }
        return uint64_t(Hex(blockNumber).getUint());
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getBlockTransactionCountByNumber: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getBlockTransactionCountByNumber: " + std::string(e.what()));
      }
    }

    void eth_chainId(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("eth_chainId does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_chainId: ") + e.what());
        throw std::runtime_error("Error while decoding eth_chainId: " + std::string(e.what()));
      }
    }

    void eth_syncing(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("eth_syncing does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_syncing: ") + e.what());
        throw std::runtime_error("Error while decoding eth_syncing: " + std::string(e.what()));
      }
    }

    void eth_coinbase(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("eth_coinbase does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_coinbase: ") + e.what());
        throw std::runtime_error("Error while decoding eth_coinbase: " + std::string(e.what()));
      }
    }

    void eth_blockNumber(const json& request) {
      try {
        /// No params are needed.
        if (!request["params"].empty()) {
          throw std::runtime_error("eth_blockNumber does not need params");
        }
        return;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_blockNumber: ") + e.what());
        throw std::runtime_error("Error while decoding eth_blockNumber: " + std::string(e.what()));
      }
    }

    std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string> eth_call(const json& request, const std::unique_ptr<Storage> &storage) {
      std::tuple<Address, Address, uint64_t, uint256_t, uint256_t, std::string> result;
      auto& [from, to, gas, gasPrice, value, data] = result;
      static const std::regex addressFilter("^0x[0-9,a-f,A-F]{40}$");
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        json txObject;
        if (request["params"].is_array()) {
          txObject = request["params"].at(0);
          if (request["params"].size() > 1) {
            const auto block = request["params"].at(1).get<std::string>();
            if (block != "latest") {
              if (!std::regex_match(block, numberFilter)) {
                throw std::runtime_error("Invalid block number");
              }
              uint64_t blockNumber = uint64_t(Hex(block).getUint());
              if (blockNumber != storage->latest()->getNHeight()) {
                throw std::runtime_error("Only latest block is supported");
              }
            }
          }
        } else if (request["params"].is_object()) {
          txObject = request["params"];
        } else {
          throw std::runtime_error("Invalid params");
        }

        /// Check from address. (optional)
        if (txObject.contains("from") && !txObject["from"].is_null()) {
          std::string fromAddress = txObject["from"].get<std::string>();
          if (!std::regex_match(fromAddress, addressFilter)) {
            throw std::runtime_error("Invalid from address hex");
          }
          from = Address(Hex::toBytes(fromAddress), true);
        }
        /// Check to address.
        std::string toAddress = txObject["to"].get<std::string>();
        if (!std::regex_match(toAddress, addressFilter)) {
          throw std::runtime_error("Invalid to address hex");
        }
        to = Address(Hex::toBytes(toAddress), true);
        /// Check gas (optional)
        if (txObject.contains("gas") && !txObject["gas"].is_null()) {
          std::string gasHex = txObject["gas"].get<std::string>();
          if (!std::regex_match(gasHex, numberFilter)) {
            throw std::runtime_error("Invalid gas hex");
          }
          gas = uint64_t(Hex(gasHex).getUint());
        }
        /// Check gasPrice (optional)
        if (txObject.contains("gasPrice") && !txObject["gasPrice"].is_null()) {
          std::string gasPriceHex = txObject["gasPrice"].get<std::string>();
          if (!std::regex_match(gasPriceHex, numberFilter)) {
            throw std::runtime_error("Invalid gasPrice hex");
          }
          gasPrice = uint256_t(Hex(gasPriceHex).getUint());
        }
        /// Check value (optional)
        if (txObject.contains("value") && !txObject["value"].is_null()) {
          std::string valueHex = txObject["value"].get<std::string>();
          if (!std::regex_match(valueHex, numberFilter)) {
            throw std::runtime_error("Invalid value hex");
          }
          value = uint256_t(Hex(valueHex).getUint());
        }
        /// Check data (optional)
        if (txObject.contains("data") && !txObject["data"].is_null()) {
          std::string dataHex = txObject["data"].get<std::string>();
          if (!Hex::isValid(dataHex, true)) {
            throw std::runtime_error("Invalid data hex");
          }
          data = Hex::toBytes(dataHex);
        }
        return result;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_call: ") + e.what());
        throw std::runtime_error("Error while decoding eth_call: " + std::string(e.what()));
      }
    }

    std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string> eth_estimateGas(const json& request, const std::unique_ptr<Storage> &storage) {
      std::tuple<Address, Address, uint64_t, uint256_t, uint256_t, std::string> result;
      auto& [from, to, gas, gasPrice, value, data] = result;
      static const std::regex addressFilter("^0x[0-9,a-f,A-F]{40}$");
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        json txObject;
        if (request["params"].is_array()) {
          txObject = request["params"].at(0);
          if (request["params"].size() > 1) {
            const auto block = request["params"].at(1).get<std::string>();
            if (block != "latest") {
              if (!std::regex_match(block, numberFilter)) {
                throw std::runtime_error("Invalid block number");
              }
              uint64_t blockNumber = uint64_t(Hex(block).getUint());
              if (blockNumber != storage->latest()->getNHeight()) {
                throw std::runtime_error("Only latest block is supported");
              }
            }
          }
        } else if (request["params"].is_object()) {
          txObject = request["params"];
        } else {
          throw std::runtime_error("Invalid params");
        }
        /// Check from address. (optional)
        if (txObject.contains("from") && !txObject["from"].is_null()) {
          std::string fromAddress = txObject["from"].get<std::string>();
          if (!std::regex_match(fromAddress, addressFilter)) {
            throw std::runtime_error("Invalid from address hex");
          }
          from = Address(Hex::toBytes(fromAddress), true);
        }
        /// Check to address. (optional)
        if (txObject.contains("to") && !txObject["to"].is_null()) {
          std::string toAddress = txObject["to"].get<std::string>();
          if (!std::regex_match(toAddress, addressFilter)) {
            throw std::runtime_error("Invalid to address hex");
          }
          to = Address(Hex::toBytes(toAddress), true);
        }
        /// Check gas (optional)
        if (txObject.contains("gas") && !txObject["gas"].is_null()) {
          std::string gasHex = txObject["gas"].get<std::string>();
          if (!std::regex_match(gasHex, numberFilter)) {
            throw std::runtime_error("Invalid gas hex");
          }
          gas = uint64_t(Hex(gasHex).getUint());
        } else { /// eth_estimateGas set gas to max if not specified
          /// TODO: Change this if we ever change gas dynamics with the chain
          gas = std::numeric_limits<uint64_t>::max();
        }
        /// Check gasPrice (optional)
        if (txObject.contains("gasPrice") && !txObject["gasPrice"].is_null()) {
          std::string gasPriceHex = txObject["gasPrice"].get<std::string>();
          if (!std::regex_match(gasPriceHex, numberFilter)) {
            throw std::runtime_error("Invalid gasPrice hex");
          }
          gasPrice = uint256_t(Hex(gasPriceHex).getUint());
        }
        /// Check value (optional)
        if (txObject.contains("value") && !txObject["value"].is_null()) {
          std::string valueHex = txObject["value"].get<std::string>();
          if (!std::regex_match(valueHex, numberFilter)) {
            throw std::runtime_error("Invalid value hex");
          }
          value = uint256_t(Hex(valueHex).getUint());
        }
        /// Check data (optional)
        if (txObject.contains("data") && !txObject["data"].is_null()) {
          std::string dataHex = txObject["data"].get<std::string>();
          if (!Hex::isValid(dataHex, true)) {
            throw std::runtime_error("Invalid data hex");
          }
          data = Hex::toBytes(dataHex);
        }
        return result;
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_estimateGas: ") + e.what());
        throw std::runtime_error("Error while decoding eth_estimateGas: " + std::string(e.what()));
      }
    }

    void eth_gasPrice(const json& request) {
      try {
        if (!request["params"].empty()) {
          throw std::runtime_error("eth_gasPrice does not need params");
        }
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_gasPrice: ") + e.what());
        throw std::runtime_error("Error while decoding eth_gasPrice: " + std::string(e.what()));
      }
    }

    Address eth_getBalance(const json& request, const std::unique_ptr<Storage> &storage) {
      static const std::regex addressFilter("^0x[0-9,a-f,A-F]{40}$");
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        const auto address = request["params"].at(0).get<std::string>();
        const auto block = request["params"].at(1).get<std::string>();
        if (block != "latest") {
          if (!std::regex_match(block, numberFilter)) {
            throw std::runtime_error("Invalid block number");
          }
          uint64_t blockNumber = uint64_t(Hex(block).getUint());
          if (blockNumber != storage->latest()->getNHeight()) {
            throw std::runtime_error("Only latest block is supported");
          }
        }
        if (!std::regex_match(address, addressFilter)) {
          throw std::runtime_error("Invalid address hex");
        }
        return Address(Hex::toBytes(address), true);
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getBalance: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getBalance: " + std::string(e.what()));
      }
    }

    Address eth_getTransactionCount(const json& request, const std::unique_ptr<Storage>& storage) {
      static const std::regex addressFilter("^0x[0-9,a-f,A-F]{40}$");
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        const auto address = request["params"].at(0).get<std::string>();
        const auto block = request["params"].at(1).get<std::string>();
        if (block != "latest") {
          if (!std::regex_match(block, numberFilter)) {
            throw std::runtime_error("Invalid block number");
          }
          uint64_t blockNumber = uint64_t(Hex(block).getUint());
          if (blockNumber != storage->latest()->getNHeight()) {
            throw std::runtime_error("Only latest block is supported");
          }
        }
        if (!std::regex_match(address, addressFilter)) {
          throw std::runtime_error("Invalid address hex");
        }
        return Address(Hex::toBytes(address), true);
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getTransactionCount: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getTransactionCount: " + std::string(e.what()));
      }
    }

    Address eth_getCode(const json& request, const std::unique_ptr<Storage>& storage) {
      static const std::regex addressFilter("^0x[0-9,a-f,A-F]{40}$");
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        const auto address = request["params"].at(0).get<std::string>();
        const auto block = request["params"].at(1).get<std::string>();
        if (block != "latest") {
          if (!std::regex_match(block, numberFilter)) {
            throw std::runtime_error("Invalid block number");
          }
          uint64_t blockNumber = uint64_t(Hex(block).getUint());
          if (blockNumber != storage->latest()->getNHeight()) {
            throw std::runtime_error("Only latest block is supported");
          }
        }
        if (!std::regex_match(address, addressFilter)) {
          throw std::runtime_error("Invalid address hex");
        }
        return Address(Hex::toBytes(address), true);
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getCode: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getCode: " + std::string(e.what()));
      }
    }

    TxBlock eth_sendRawTransaction(const json& request, const uint64_t& requiredChainId) {
      try {
        const auto txHex = request["params"].at(0).get<std::string>();
        if (!Hex::isValid(txHex, true)) {
          throw std::runtime_error("Invalid transaction hex");
        }
        return TxBlock(Hex::toBytes(txHex), requiredChainId);
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_sendRawTransaction: ") + e.what());
        throw std::runtime_error("Error while decoding eth_sendRawTransaction: " + std::string(e.what()));
      }
    }

    Hash eth_getTransactionByHash(const json& request) {
      static const std::regex hashFilter("^0x[0-9,a-f,A-F]{64}$");
      try {
        const auto hash = request["params"].at(0).get<std::string>();
        if (!std::regex_match(hash, hashFilter)) {
          throw std::runtime_error("Invalid hash hex");
        }
        return Hash(Hex::toBytes(hash));
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getTransactionByHash: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getTransactionByHash: " + std::string(e.what()));
      }
    }

    std::pair<Hash,uint64_t> eth_getTransactionByBlockHashAndIndex(const json& request) {
      static const std::regex hashFilter("^0x[0-9,a-f,A-F]{64}$");
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        std::string blockHash = request["params"].at(0).get<std::string>();
        std::string index = request["params"].at(1).get<std::string>();
        if (!std::regex_match(blockHash, hashFilter)) {
          throw std::runtime_error("Invalid blockHash hex");
        }
        if (!std::regex_match(index, numberFilter)) {
          throw std::runtime_error("Invalid index hex");
        }
        return std::make_pair(Hash(Hex::toBytes(blockHash)), uint64_t(Hex(index).getUint()));
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getTransactionByBlockHashAndIndex: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getTransactionByBlockHashAndIndex: " + std::string(e.what()));
      }
    }

    std::pair<uint64_t,uint64_t> eth_getTransactionByBlockNumberAndIndex(const json& request, const std::unique_ptr<Storage> &storage) {
      static const std::regex numberFilter("^0x([1-9a-f]+[0-9a-f]*|0)$");
      try {
        std::string blockNumber = request["params"].at(0).get<std::string>();
        std::string index = request["params"].at(1).get<std::string>();
        if (!std::regex_match(index, numberFilter)) {
          throw std::runtime_error("Invalid index hex");
        }
        if (blockNumber == "latest") {
          return std::make_pair<uint64_t,uint64_t>(storage->latest()->getNHeight(), uint64_t(Hex(index).getUint()));
        }
        if (!std::regex_match(blockNumber, numberFilter)) {
          throw std::runtime_error("Invalid blockNumber hex");
        }
        return std::make_pair<uint64_t,uint64_t>(uint64_t(Hex(blockNumber).getUint()), uint64_t(Hex(index).getUint()));
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getTransactionByBlockNumberAndIndex: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getTransactionByBlockNumberAndIndex: " + std::string(e.what()));
      }
    }

    Hash eth_getTransactionReceipt(const json& request) {
      static const std::regex hashFilter("^0x[0-9,a-f,A-F]{64}$");
      try {
        std::string txHash = request["params"].at(0).get<std::string>();
        if (!std::regex_match(txHash, hashFilter)) {
          throw std::runtime_error("Invalid Hex");
        }
        return Hash(Hex::toBytes(txHash));
      } catch (std::exception &e) {
        Utils::logToDebug(Log::JsonRPCDecoding, __func__, std::string("Error while decoding eth_getTransactionReceipt: ") + e.what());
        throw std::runtime_error("Error while decoding eth_getTransactionReceipt: " + std::string(e.what()));
      }
    }
  }
}
