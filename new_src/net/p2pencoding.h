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

/// Enum for easy reference of supported P2P commands.
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

/**
 * Map with the supported P2P commands and their respective 4-byte hex IDs.
 * All requests are made client -> server.
 * Please refer to the internal Sparq documentation for more details on each command.
 */
static const std::unordered_map<P2PCmdType, std::string> p2pcmds = {
  { P2PCmdType::Info,                 Utils::hexToBytes("0x00000000") },
  { P2PCmdType::SendTx,               Utils::hexToBytes("0x00000001") },
  { P2PCmdType::SendBulkTxs,          Utils::hexToBytes("0x00000002") },
  { P2PCmdType::GetBlockByNumber,     Utils::hexToBytes("0x00000003") },
  { P2PCmdType::GetBlockByHash,       Utils::hexToBytes("0x00000004") },
  { P2PCmdType::GetBlockRange,        Utils::hexToBytes("0x00000005") },
  { P2PCmdType::NewBestBlock,         Utils::hexToBytes("0x00000006") },
  { P2PCmdType::SendValidatorTx,      Utils::hexToBytes("0x00000007") },
  { P2PCmdType::SendBulkValidatorTxs, Utils::hexToBytes("0x00000008") },
  { P2PCmdType::RequestValidatorTxs,  Utils::hexToBytes("0x00000009") },
  { P2PCmdType::GetConnectedNodes,    Utils::hexToBytes("0x0000000a") }
};

/// Helper class that abstracts a P2P message/command.
class P2PMsg {
  private:
    /**
     * The internal message data to be read/written, stored as bytes (1 byte = 2 chars).
     * The structure of a message is as follows:
     *
     * 0x     0000000000000000  00000000  00000000000000000000000000000000...
     * What:     Random ID       Cmd ID               Data...
     * Chars:       16             8                     X
     * Bytes:        8             4                    X/2
     *
     * e.g. "0123456789abcdef0000111122223333" would be:
     * - `msg.substr(0, 8)` = "0123456789abcdef"
     * - `msg.substr(8, 4)` = "0000"
     * - `msg.substr(12)` = "111122223333"
     */
    std::string msg;

  public:
    /**
     * Constructor.
     * @param data The message data.
     */
    P2PMsg(std::string&& data) : msg(std::move(msg));

    /**
     * Get the message data (without both IDs).
     * @return A read-only copy of the data string.
     */
    const std::string_view getMsg() { return std::string_view(this->msg).substr(12); }

    /**
     * Get the command of the message.
     * @return A pair with the P2P command type and its respective 4-byte ID.
     */
    const std::pair<P2PCmdType, std::string> getCmd() {
      auto ret = p2pcmds.find(Utils::bytesToUint16(this->msg.substr(8, 4)));
      if (ret == p2pcmds.end()) throw std::runtime_error("Invalid p2p cmd type");
      return std::make_pair(ret->first, ret->second);
    }

    /**
     * Get the random ID (fingerprint) of the message.
     * @return The 8-byte ID.
     */
    const std::string getId() { return this->msg.substr(0, 8); }

    /**
     * Get the whole message (aka "the real getter for `msg`").
     * @return The entire unchanged data string.
     */
    const std::string& getRaw() { return this->msg; }
};

// TODO: remaining commands for the en/decoders below

/// Namespace for helping P2PClient to encode requests for P2PServer to answer.
namespace P2PRequestEncoder {
  /**
   * Encode a request for `Info`.
   * @param storage Pointer to the blockchain history.
   * @param nodes The number of connected nodes.
   * @return The encoded request.
   */
  P2PMsg info(const std::shared_ptr<Storage>& storage, const uint64_t& nodes);

  /**
   * Encode a request for `SendTx`.
   * @param tx The transaction to send.
   * @return The encoded request.
   */
  P2PMsg sendTx(const TxBlock& tx);

  /**
   * Encode a request for `SendBulkTxs`.
   * @param txs The list of transactions to send.
   * @return The encoded request.
   */
  P2PMsg sendBulkTxs(const std::vector<const TxBlock>& txs);

  /**
   * Encode a request for `GetBlockByNumber`.
   * @param height The block height to get.
   * @return The encoded request.
   */
  P2PMsg getBlockByNumber(const uint64_t& height);

  /**
   * Encode a request for `GetBlockByHash`.
   * @param height The block hash to get.
   * @return The encoded request.
   */
  P2PMsg getBlockByHash(const Hash& hash);

  /**
   * Encode a request for `GetBlockRange`.
   * @param startHeight The block height to start with.
   * @param endHeight The block height to end with.
   * @return The encoded request.
   */
  P2PMsg getBlockRange(const uint64_t& startHeight, const uint64_t& endHeight);

  /**
   * Encode a request for `NewBestBlock`.
   * @param block The block hash to set as the new best.
   * @return The encoded request.
   */
  P2PMsg newBestBlock(const Block& block);

  /**
   * Encode a request for `SendValidatorTx`.
   * @param tx The transaction to send.
   * @return The encoded request.
   */
  P2PMsg sendValidatorTx(const TxValidator& tx);

  /**
   * Encode a request for `SendBulkValidatorTxs`.
   * @param txs The list of transactions to send.
   * @return The encoded request.
   */
  P2PMsg sendBulkValidatorTxs(const std::vector<const TxValidator>& txs);

  /**
   * Encode a request for `RequestValidatorTxs`.
   * @return The encoded request.
   */
  P2PMsg requestValidatorTxs();

  /**
   * Encode a request for `GetConnectedNodes`.
   * @return The encoded request.
   */
  P2PMsg getConnectedNodes();
};

/// Namespace for helping P2PServer to decode requests made by P2PClient.
namespace P2PRequestDecoder {
  /**
   * Decode a request for `Info`.
   * @param msg The message to decode.
   * @return The info about the connection.
   */
  ConnectionInfo info(const P2PMsg& msg);

  /**
   * Decode a request for `SendTx`.
   * @param msg The message to decode.
   * @return The sent transaction.
   */
  TxBlock sendTx(const P2PMsg& msg);

  /**
   * Decode a request for `SendValidatorTx`.
   * @param msg The message to decode.
   * @return The sent transaction.
   */
  TxValidator sendValidatorTx(const P2PMsg& msg);
};

/// Namespace for helping P2PServer to encode answers to requests made by P2PClient.
namespace P2PAnswerEncoder {
  /**
   * Encode an answer to `Info`.
   * @param storage Pointer to the blockchain history.
   * @param nodes The number of connected nodes.
   * @param id The random ID/fingerprint of the message.
   * @return The encoded answer.
   */
  P2PMsg info(const std::shared_ptr<Storage>& storage, const uint64_t& nodes, const std::string& id);

  /**
   * Encode an answer to `RequestValidatorTxs`.
   * @param txs The list of requested transactions.
   * @return The encoded answer.
   */
  P2PMsg requestValidatorTxs(const std::unordered_map<Hash, TxValidator, SafeHash>& txs);
};

/// Namespace for helping P2PClient to decode answers to requests made to P2PServer.
namespace P2PAnswerDecoder {
  /**
   * Decode an answer to `Info`.
   * @param msg The message to decode.
   * @return The info about the connection.
   */
  ConnectionInfo info(const P2PMsg& msg);

  /**
   * Decode an answer to `RequestValidatorTxs`.
   * @param msg The message to decode.
   * @return The list of requested transactions.
   */
  std::vector<TxValidator> requestValidatorTxs(const P2PMsg& msg);
};

#endif  // P2PENCODING_H
