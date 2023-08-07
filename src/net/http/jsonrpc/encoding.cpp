#include "encoding.h"

#include "../../../core/storage.h"
#include "../../../core/state.h"

namespace JsonRPC {
  namespace Encoding {
    json getBlockJson(const std::shared_ptr<const Block>& block, bool includeTransactions) {
      json ret;
      ret["jsonrpc"] = 2.0;
      try {
        if (block == nullptr) { ret["result"] = json::value_t::null; return ret; }
        ret["result"]["hash"] = block->hash().hex(true);
        ret["result"]["parentHash"] = block->getPrevBlockHash().hex(true);
        ret["result"]["sha3Uncles"] = Hash().hex(true); // Uncles do not exist.
        ret["result"]["miner"] = Secp256k1::toAddress(block->getValidatorPubKey()).hex(true);
        ret["result"]["stateRoot"] = Hash().hex(true); // No State root.
        ret["result"]["transactionsRoot"] = block->getTxMerkleRoot().hex(true);
        ret["result"]["receiptsRoot"] = Hash().hex(true); // No receiptsRoot.
        ret["result"]["logsBloom"] = Hash().hex(true); // No logsBloom.
        ret["result"]["difficulty"] = "0x1";
        ret["result"]["number"] = Hex::fromBytes(Utils::uintToBytes(block->getNHeight()),true).forRPC();
        ret["result"]["gasLimit"] = Hex::fromBytes(Utils::uintToBytes(std::numeric_limits<uint64_t>::max()),true).forRPC();
        ret["result"]["gasUsed"] = Hex::fromBytes(Utils::uintToBytes(uint64_t(1000000000)),true).forRPC(); // Arbitrary number
        ret["result"]["timestamp"] = Hex::fromBytes(Utils::uintToBytes(block->getTimestamp()),true).forRPC();
        ret["result"]["extraData"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
        ret["result"]["mixHash"] = Hash().hex(true); // No mixHash.
        ret["result"]["nonce"] = "0x0000000000000000";
        ret["result"]["totalDifficulty"] = "0x1";
        ret["result"]["baseFeePerGas"] = "0x9502f900";
        ret["result"]["withdrawRoot"] = Hash().hex(true); // No withdrawRoot.
        // TODO: to get a block you have to serialize it entirely, this can be expensive.
        ret["result"]["size"] = Hex::fromBytes(Utils::uintToBytes(block->serializeBlock().size()),true).forRPC();
        ret["result"]["transactions"] = json::array();
        for (const auto& tx : block->getTxs()) {
          if (!includeTransactions) { // Only include the transaction hashes.
            ret["result"]["transactions"].push_back(tx.hash().hex(true));
          } else { // Include the transactions as a whole.
            json txJson = json::object();
            txJson["type"] = "0x0"; // Legacy Transactions ONLY. TODO: change this to 0x2 when we support EIP-1559
            txJson["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()),true).forRPC(); // TODO: get the nonce from the transaction.
            txJson["to"] = tx.getTo().hex(true);
            txJson["gas"] = Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()),true).forRPC();
            txJson["value"] = Hex::fromBytes(Utils::uintToBytes(tx.getValue()),true).forRPC();
            txJson["input"] = Hex::fromBytes(tx.getData(),true).forRPC();
            txJson["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC();
            txJson["chainId"] = Hex::fromBytes(Utils::uintToBytes(tx.getChainId()),true).forRPC();
            txJson["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()),true).forRPC();
            txJson["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()),true).forRPC();
            txJson["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()),true).forRPC();
            ret["result"]["transactions"].emplace_back(std::move(txJson));
          }
        }
        ret["result"]["withdrawls"] = json::array();
        ret["result"]["uncles"] = json::array();
      } catch (std::exception& e) {
        json error;
        error["jsonrpc"] = 2.0;
        error["error"]["code"] = -32603;
        error["error"]["message"] = "Internal error: " + std::string(e.what());
        return error;
      }
      return ret;
    }

    json web3_clientVersion(const std::unique_ptr<Options>& options) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = options->getWeb3ClientVersion();
      return ret;
    }

    json web3_sha3(const BytesArrView data) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = Utils::sha3(data).hex(true);
      return ret;
    }

    json net_version(const std::unique_ptr<Options>& options) {
      json ret;
      ret["jsonrpc"] = 2.0;
      ret["result"] = std::to_string(options->getVersion());
      return ret;
    }

    json net_listening() {
      json ret;
      ret["jsonrpc"] = 2.0;
      ret["result"] = true;
      return ret;
    }

    json net_peerCount(const std::unique_ptr<P2P::ManagerNormal>& manager) {
      json ret;
      ret["jsonrpc"] = 2.0;
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(manager->getPeerCount()), true).forRPC();
      return ret;
    }

    json eth_protocolVersion(const std::unique_ptr<Options>& options) {
      json ret;
      ret["jsonrpc"] = 2.0;
      ret["result"] = options->getSDKVersion();
      return ret;
    }

    json eth_getBlockByHash(const std::pair<Hash,bool>& blockInfo, const std::unique_ptr<Storage>& storage) {
      auto const& [blockHash, includeTransactions] = blockInfo;
      auto block = storage->getBlock(blockHash);
      return getBlockJson(block, includeTransactions);
    }

    json eth_getBlockByNumber(const std::pair<uint64_t,bool>& blockInfo, const std::unique_ptr<Storage>& storage) {
      auto const& [blockNumber, includeTransactions] = blockInfo;
      auto block = storage->getBlock(blockNumber);
      return getBlockJson(block, includeTransactions);
    }

    json eth_getBlockTransactionCountByHash(const Hash& blockHash, const std::unique_ptr<Storage>& storage) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto block = storage->getBlock(blockHash);
      if (block == nullptr) ret["result"] = json::value_t::null;
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(block->getTxs().size()), true).forRPC();
      return ret;
    }

    json eth_getBlockTransactionCountByNumber(const uint64_t& blockNumber, const std::unique_ptr<Storage>& storage) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto block = storage->getBlock(blockNumber);
      if (block == nullptr) ret["result"] = json::value_t::null;
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(block->getTxs().size()), true).forRPC();
      return ret;
    }

    json eth_chainId(const std::unique_ptr<Options>& options) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(options->getChainID()), true).forRPC();
      return ret;
    }

    json eth_syncing() {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = false;
      return ret;
    }

    json eth_coinbase(const std::unique_ptr<Options>& options) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = options->getCoinbase().hex(true);
      return ret;
    }

    json eth_blockNumber(const std::unique_ptr<Storage>& storage) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto latestBlock = storage->latest();
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(latestBlock->getNHeight()), true).forRPC();
      return ret;
    }

    json eth_call(const ethCallInfoAllocated& callInfo, const std::unique_ptr<State>& state) {
      json ret;
      ret["jsonrpc"] = "2.0";
      try {
        std::cout << "Calling State...: " << std::endl;
        std::cout << "callInfo functor: " << std::get<5>(callInfo).hex() << std::endl;
        auto result = Hex::fromBytes(state->ethCall(callInfo), true);
        ret["result"] = result;
      } catch (std::exception& e) {
        ret["error"]["code"] = -32000;
        ret["error"]["message"] = "Internal error: " + std::string(e.what());
      }
      return ret;
    }

    json eth_estimateGas(const ethCallInfoAllocated& callInfo, const std::unique_ptr<State>& state) {
      json ret;
      ret["jsonrpc"] = "2.0";
      try {
        state->estimateGas(callInfo);
        ret["result"] = "0x5208"; // Fixed to 21000 for now.
      } catch (std::exception& e) {
        ret["error"]["code"] = -32000;
        ret["error"]["message"] = "Internal error: " + std::string(e.what());
      }
      Utils::safePrint("Estimate gas encoding returning!");
      return ret;
    }

    json eth_gasPrice() {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = "0x9502f900"; // Fixed to 2.5 GWei
      return ret;
    }

    json eth_getBalance(const Address& address, const std::unique_ptr<State>& state) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(state->getNativeBalance(address)), true).forRPC();
      return ret;
    }

    json eth_getTransactionCount(const Address& address, const std::unique_ptr<State>& state) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(state->getNativeNonce(address)), true).forRPC();
      return ret;
    }

    json eth_getCode(const Address& address) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = "0x";
      return ret;
    }

    json eth_sendRawTransaction(const TxBlock& tx, const std::unique_ptr<State>& state, const std::unique_ptr<P2P::ManagerNormal>& p2p) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto txHash = tx.hash();
      // We can't move as we need to broadcast the tx (see below)
      auto TxInvalid = state->addTx(TxBlock(tx));
      if (!TxInvalid) {
        ret["result"] = txHash.hex(true);
        // TODO: Make this use threadpool instead of blocking
        // TODO: Make tx broadcasting better, the current solution is not good.
        p2p->broadcastTxBlock(tx);
      } else {
        ret["error"]["code"] = -32000;
        switch (TxInvalid) {
          case TxInvalid::InvalidNonce:
            ret["error"]["message"] = "Invalid nonce";
            break;
          case TxInvalid::InvalidBalance:
            ret["error"]["message"] = "Invalid balance";
            break;
        }
      }
      return ret;
    }

    json eth_getTransactionByHash(const Hash& txHash, const std::unique_ptr<Storage>& storage, const std::unique_ptr<State>& state) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto txOnMempool = state->getTxFromMempool(txHash);
      if (txOnMempool != nullptr) {
        ret["result"]["blockHash"] = json::value_t::null;
        ret["result"]["blockIndex"] = json::value_t::null;
        ret["result"]["from"] = txOnMempool->getFrom().hex(true);
        ret["result"]["gas"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getGasLimit()), true).forRPC();
        ret["result"]["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getMaxFeePerGas()), true).forRPC();
        ret["result"]["hash"] = txOnMempool->hash().hex(true);
        ret["result"]["input"] = Hex::fromBytes(txOnMempool->getData(), true).forRPC();
        ret["result"]["nonce"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getNonce()), true).forRPC();
        ret["result"]["to"] = txOnMempool->getTo().hex(true);
        ret["result"]["transactionIndex"] = json::value_t::null;
        ret["result"]["value"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getValue()), true).forRPC();
        ret["result"]["v"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getV()), true).forRPC();
        ret["result"]["r"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getR()), true).forRPC();
        ret["result"]["s"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getS()), true).forRPC();
        return ret;
      }

      auto txOnChain = storage->getTx(txHash);
      const auto& [tx, blockHash, blockIndex, blockHeight] = txOnChain;
      if (tx != nullptr) {
        ret["result"]["blockHash"] = blockHash.hex(true);
        ret["result"]["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(blockHeight), true).forRPC();
        ret["result"]["from"] = tx->getFrom().hex(true);
        ret["result"]["gas"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
        ret["result"]["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()), true).forRPC();
        ret["result"]["hash"] = tx->hash().hex(true);
        ret["result"]["input"] = Hex::fromBytes(tx->getData(), true).forRPC();
        ret["result"]["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx->getNonce()), true).forRPC();
        ret["result"]["to"] = tx->getTo().hex(true);
        ret["result"]["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(blockIndex), true).forRPC();
        ret["result"]["value"] = Hex::fromBytes(Utils::uintToBytes(tx->getValue()), true).forRPC();
        ret["result"]["v"] = Hex::fromBytes(Utils::uintToBytes(tx->getV()), true).forRPC();
        ret["result"]["r"] = Hex::fromBytes(Utils::uintToBytes(tx->getR()), true).forRPC();
        ret["result"]["s"] = Hex::fromBytes(Utils::uintToBytes(tx->getS()), true).forRPC();
        return ret;
      }
      ret["result"] = json::value_t::null;
      return ret;
    }

    json eth_getTransactionByBlockHashAndIndex(const std::pair<Hash,uint64_t>& requestInfo, const std::unique_ptr<Storage>& storage) {
      json ret;
      ret["jsonrpc"] = "2.0";
      const auto& [blockHash, blockIndex] = requestInfo;
      auto txInfo = storage->getTxByBlockHashAndIndex(blockHash, blockIndex);
      const auto& [tx, txBlockHash, txBlockIndex, txBlockHeight] = txInfo;
      if (tx != nullptr) {
        ret["result"]["blockHash"] = txBlockHash.hex(true);
        ret["result"]["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(txBlockHeight), true).forRPC();
        ret["result"]["from"] = tx->getFrom().hex(true);
        ret["result"]["gas"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
        ret["result"]["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()), true).forRPC();
        ret["result"]["hash"] = tx->hash().hex(true);
        ret["result"]["input"] = Hex::fromBytes(tx->getData(), true).forRPC();
        ret["result"]["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx->getNonce()), true).forRPC();
        ret["result"]["to"] = tx->getTo().hex(true);
        ret["result"]["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txBlockIndex), true).forRPC();
        ret["result"]["value"] = Hex::fromBytes(Utils::uintToBytes(tx->getValue()), true).forRPC();
        ret["result"]["v"] = Hex::fromBytes(Utils::uintToBytes(tx->getV()), true).forRPC();
        ret["result"]["r"] = Hex::fromBytes(Utils::uintToBytes(tx->getR()), true).forRPC();
        ret["result"]["s"] = Hex::fromBytes(Utils::uintToBytes(tx->getS()), true).forRPC();
        return ret;
      }
      ret["result"] = json::value_t::null;
      return ret;
    }

    json eth_getTransactionByBlockNumberAndIndex(const std::pair<uint64_t,uint64_t>& requestInfo, const std::unique_ptr<Storage>& storage) {
      json ret;
      ret["jsonrpc"] = "2.0";
      const auto& [blockNumber, blockIndex] = requestInfo;
      auto txInfo = storage->getTxByBlockNumberAndIndex(blockNumber, blockIndex);
      const auto& [tx, txBlockHash, txBlockIndex, txBlockHeight] = txInfo;
      if (tx != nullptr) {
        ret["result"]["blockHash"] = txBlockHash.hex(true);
        ret["result"]["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(txBlockHeight), true).forRPC();
        ret["result"]["from"] = tx->getFrom().hex(true);
        ret["result"]["gas"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
        ret["result"]["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()), true).forRPC();
        ret["result"]["hash"] = tx->hash().hex(true);
        ret["result"]["input"] = Hex::fromBytes(tx->getData(), true).forRPC();
        ret["result"]["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx->getNonce()), true).forRPC();
        ret["result"]["to"] = tx->getTo().hex(true);
        ret["result"]["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txBlockIndex), true).forRPC();
        ret["result"]["value"] = Hex::fromBytes(Utils::uintToBytes(tx->getValue()), true).forRPC();
        ret["result"]["v"] = Hex::fromBytes(Utils::uintToBytes(tx->getV()), true).forRPC();
        ret["result"]["r"] = Hex::fromBytes(Utils::uintToBytes(tx->getR()), true).forRPC();
        ret["result"]["s"] = Hex::fromBytes(Utils::uintToBytes(tx->getS()), true).forRPC();
        return ret;
      }
      ret["result"] = json::value_t::null;
      return ret;
    }

    json eth_getTransactionReceipt(const Hash& txHash, const std::unique_ptr<Storage>& storage) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto txInfo = storage->getTx(txHash);
      const auto& [tx, blockHash, blockIndex, blockHeight] = txInfo;
      if (tx != nullptr) {
        ret["result"]["transactionHash"] = tx->hash().hex(true);
        ret["result"]["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(blockIndex), true).forRPC();
        ret["result"]["blockHash"] = blockHash.hex(true);
        ret["result"]["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(blockHeight), true).forRPC();
        ret["result"]["from"] = tx->getFrom().hex(true);
        ret["result"]["to"] = tx->getTo().hex(true);
        ret["result"]["cumulativeGasUsed"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
        ret["result"]["effectiveGasUsed"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
        ret["result"]["effectiveGasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()),true).forRPC();
        ret["result"]["gasUsed"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
        ret["result"]["contractAddress"] = json::value_t::null; // TODO: CHANGE THIS WHEN CREATING CONTRACTS!
        ret["result"]["logs"] = json::array();
        ret["result"]["logsBloom"] = Hash().hex(true);
        ret["result"]["type"] = "0x00";
        ret["result"]["root"] = Hash().hex(true);
        ret["result"]["status"] = "0x1"; // TODO: change this when contracts are ready
        return ret;
      }
      ret["result"] = json::value_t::null;
      return ret;
    }
  }
}

