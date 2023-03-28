#include "encoding.h"

#include "../../p2p/p2pmanagernormal.h"
#include "../../../core/storage.h"
#include "../../../core/state.h"

namespace JsonRPC {
  namespace Encoding {
    json getBlockJson(const std::shared_ptr<const Block>& block, bool includeTransactions) {
      json ret;
      ret["jsonrpc"] = 2.0;
      try {
        if (block == nullptr) {
          ret["result"] = json::value_t::null;
          return ret;
        }
        ret["result"]["parentHash"] = block->getPrevBlockHash().hex(true);
        ret["result"]["sha3Uncles"] = Hash().hex(true); /// Uncles doesn't exists.
        ret["result"]["miner"] = Secp256k1::toAddress(block->getValidatorPubKey()).hex(true);
        ret["result"]["stateRoot"] = Hash().hex(true); /// No State root.
        ret["result"]["transactionsRoot"] = block->getTxMerkleRoot().hex(true);
        ret["result"]["receiptsRoot"] = Hash().hex(true); /// No receiptsRoot.
        ret["result"]["logsBloom"] = Hash().hex(true); /// No logsBloom.
        ret["result"]["difficulty"] = "0x1";
        ret["result"]["number"] = Hex::fromBytes(Utils::uintToBytes(block->getNHeight()),true);
        ret["result"]["gasLimit"] = Hex::fromBytes(Utils::uintToBytes(std::numeric_limits<uint64_t>::max()),true);
        ret["result"]["gasUsed"] = Hex::fromBytes(Utils::uintToBytes(uint64_t(1000000000)),true); /// Arbitrary number
        ret["result"]["timestamp"] = Hex::fromBytes(Utils::uintToBytes(block->getTimestamp()),true);
        ret["result"]["extraData"] = "0x0";
        ret["result"]["mixHash"] = Hash().hex(true); /// No mixHash.
        ret["result"]["nonce"] = "0x0000000000000000";
        ret["result"]["totalDifficulty"] = "0x1";
        ret["result"]["baseFeePerGas"] = "0x9502f900";
        ret["result"]["withdrawRoot"] = Hash().hex(true); /// No withdrawRoot.
        ret["result"]["size"] = Hex::fromBytes(Utils::uintToBytes(block->serializeBlock().size()),true); /// TODO: to get a block you have to serialize it entirely, this can be expensive.
        if (!includeTransactions) {
          /// Only include the transaction hashes.
          ret["result"]["transactions"] = json::array();
          for (const auto& tx : block->getTxs()) {
            ret["result"]["transactions"].push_back(tx.hash().hex(true));
          }
        } else {
          /// Encode the transaction within the json response.
          ret["result"]["transactions"] = json::array();
          for (const auto& tx : block->getTxs()) {
            json txJson = json::object();
            txJson["type"] = "0x0"; /// Legacy Transactions ONLY. TODO: change this to 0x2 when we support EIP-1559
            txJson["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()),true); /// TODO: get the nonce from the transaction.
            txJson["to"] = tx.getTo().hex(true);
            txJson["gas"] = Hex::fromBytes(Utils::uintToBytes(tx.getGas()),true);
            txJson["value"] = Hex::fromBytes(Utils::uintToBytes(tx.getValue()),true);
            txJson["input"] = Hex::fromBytes(tx.getData(),true);
            txJson["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getGasPrice()),true);
            txJson["chainId"] = Hex::fromBytes(Utils::uintToBytes(tx.getChainId()),true);
            txJson["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()),true);
            txJson["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()),true);
            txJson["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()),true);
            ret["result"]["transactions"].emplace_back(std::move(txJson));
          }
        }
        ret["result"]["withdrawls"] = json::array();
        ret["result"]["uncles"] = json::array();
      } catch (std::exception &e) {
        json error;
        error["jsonrpc"] = 2.0;
        error["error"]["code"] = -32603;
        error["error"]["message"] = "Internal error: " + std::string(e.what());
        return error;
      }
      return ret;
    }

    json web3_clientVersion() {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = "OrbiterSDK/cpp/linux_x86-64/0.0.1";
      return ret;
    }

    json web3_sha3(const std::string& data) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = Utils::sha3(data).hex(true);
      return ret;
    }

    json net_version() {
      json ret;
      ret["jsonrpc"] = 2.0;
      ret["result"] = "1";
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
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(manager->getPeerCount()), true);
      return ret;
    }

    json eth_protocolVersion() {
      json ret;
      ret["jsonrpc"] = 2.0;
      ret["result"] = "1";
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
      if (block == nullptr) {
        ret["result"] = json::value_t::null;
      }
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(block->getTxs().size()), true);
      return ret;
    }

    json eth_getBlockTransactionCountByNumber(const uint64_t& blockNumber, const std::unique_ptr<Storage>& storage) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto block = storage->getBlock(blockNumber);
      if (block == nullptr) {
        ret["result"] = json::value_t::null;
      }
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(block->getTxs().size()), true);
      return ret;
    }

    json eth_chainId() {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = "0x1f90";
      return ret;
    }

    json eth_syncing() {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = false;
      return ret;
    }

    json eth_coinbase() {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = json::value_t::null;
      return ret;
    }

    json eth_blockNumber(const std::unique_ptr<Storage>& storage) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto latestBlock = storage->latest();
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(latestBlock->getNHeight()), true);
      return ret;
    }

    json eth_estimateGas(const std::tuple<Address,Address,uint64_t, uint256_t, uint256_t, std::string>& callInfo) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = "0x5208"; // Fixed to 21000 for now.
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
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(state->getNativeBalance(address)), true);
      return ret;
    }

    json eth_getTransactionCount(const Address& address, const std::unique_ptr<State>& state) {
      json ret;
      ret["jsonrpc"] = "2.0";
      ret["result"] = Hex::fromBytes(Utils::uintToBytes(state->getNativeNonce(address)), true);
      return ret;
    }

    json eth_sendRawTransaction(TxBlock&& tx, const std::unique_ptr<State>& state) {
      json ret;
      ret["jsonrpc"] = "2.0";
      auto txHash = tx.hash();
      auto TxInvalid = state->addTx(std::move(tx));
      if (!TxInvalid) {
        ret["result"] = txHash.hex(true);
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
  }
}