#ifndef P2PENCODING_H
#define P2PENCODING_H

#include <unordered_map>

#include "p2pencoding.h"
#include "p2pmanager.h"
#include "../core/storage.h"
#include "../utils/block.h"
#include "../utils/strings.h"
#include "../utils/tx.h"
#include "../utils/utils.h"

// All requests are made client -> server
enum P2PCmdType {
  Info,
  SendTx,
  SendBulkTxs,
  GetBlockByNumber,
  GetBlockByHash,
  GetBlockRange,
  NewBestBlock,
  SendValidatorTx,
  SendBulkValidatorTxs,
  RequestValidatorTxs,
  GetConnectedNodes
};

static const std::unordered_map<P2PCmdType, std::string> p2pcmds = {
  { P2PCmdType::Info,                 Utils::hexToBytes("0x0000") },
  { P2PCmdType::SendTx,               Utils::hexToBytes("0x0001") },
  { P2PCmdType::SendBulkTxs,          Utils::hexToBytes("0x0002") },
  { P2PCmdType::GetBlockByNumber,     Utils::hexToBytes("0x0003") },
  { P2PCmdType::GetBlockByHash,       Utils::hexToBytes("0x0004") },
  { P2PCmdType::GetBlockRange,        Utils::hexToBytes("0x0005") },
  { P2PCmdType::NewBestBlock,         Utils::hexToBytes("0x0006") },
  { P2PCmdType::SendValidatorTx,      Utils::hexToBytes("0x0007") },
  { P2PCmdType::SendBulkValidatorTxs, Utils::hexToBytes("0x0008") },
  { P2PCmdType::RequestValidatorTxs,  Utils::hexToBytes("0x0009") },
  { P2PCmdType::GetConnectedNodes,    Utils::hexToBytes("0x000a") }
};

class P2PMsg {
  private:
    std::string msg;

  public:
    P2PMsg(std::string&& data) : msg(std::move(msg));

    const std::string_view getMsg() { return std::string_view(this->msg).substr(10); }

    const std::pair<P2PCmdType, std::string> getCmd() {
      auto ret = p2pcmds.find(Utils::bytesToUint16(this->msg.substr(8, 2)));
      if (ret == p2pcmds.end()) throw std::runtime_error("Invalid p2p cmd type");
      return std::make_pair(ret->first, ret->second);
    }

    const std::string getId() { return msg.substr(0, 8); }

    const std::string& getRaw() { return msg; }
};

// TODO: remaining commands for the en/decoders below

namespace P2PRequestEncoder {
  P2PMsg info(const std::shared_ptr<Storage>& storage, const uint64_t& nodes);
  P2PMsg sendTx(const TxBlock& tx);
  P2PMsg sendBulkTxs(const std::vector<const TxBlock>& txs);
  P2PMsg getBlockByNumber(const uint64_t& height);
  P2PMsg getBlockByHash(const Hash& hash);
  P2PMsg getBlockRange(const uint64_t& startHeight, const uint64_t& endHeight);
  P2PMsg newBestBlock(const Block& block);
  P2PMsg sendValidatorTx(const TxValidator& tx);
  P2PMsg sendBulkValidatorTxs(const std::vector<const TxValidator>& txs);
  P2PMsg requestValidatorTxs();
  P2PMsg getConnectedNodes();
};

namespace P2PRequestDecoder {
  ConnectionInfo info(const P2PMsg& msg);
  TxBlock sendTx(const P2PMsg& msg);
  TxValidator sendValidatorTx(const P2PMsg& msg);
};

namespace P2PAnswerEncoder {
  P2PMsg info(const std::shared_ptr<Storage>& storage, const uint64_t& nodes, const std::string& id);
  P2PMsg requestValidatorTxs(const std::unordered_map<Hash, TxValidator, SafeHash>& txs);
};

namespace P2PAnswerDecoder {
  ConnectionInfo info(const P2PMsg& msg);
  std::vector<TxValidator> requestValidatorTxs(const P2PMsg& msg);
};

#endif  // P2PENCODING_H
