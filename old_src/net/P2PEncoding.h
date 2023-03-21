#ifndef P2PENCODER_H
#define P2PENCODER_H

#include "../utils/utils.h"
#include "../utils/transaction.h"
#include "../core/block.h"

// All requests are made client -> server
enum CommandType {
  Info,
  SendTransaction,
  SendBulkTransactions,
  RequestBlockByNumber,
  RequestBlockByHash,
  RequestBlockRange,
  NewBestBlock,
  SendValidatorTransaction,
  SendBulkValidatorTransactions,
  RequestValidatorTransactions,
  GetConnectedNodes
};

CommandType getCommandType(const std::string_view& message);
std::string getCommandPrefix(const CommandType& commType);

// Vector for easy conversion to command prefixes.
inline extern const std::vector<std::string> commandPrefixes {
  Utils::hexToBytes("0x0000"), // Info
  Utils::hexToBytes("0x0001"), // SendTransaction
  Utils::hexToBytes("0x0002"), // SendBulkTransactions
  Utils::hexToBytes("0x0003"), // RequestBlockByNumber
  Utils::hexToBytes("0x0004"), // RequestBlockByHash
  Utils::hexToBytes("0x0005"), // RequestBlockRange
  Utils::hexToBytes("0x0006"), // NewBestBlock
  Utils::hexToBytes("0x0007"), // SendValidatorTransaction
  Utils::hexToBytes("0x0008"), // SendBulkValidatorTransactions
  Utils::hexToBytes("0x0009"), // RequestValidatorTransactions
  Utils::hexToBytes("0x000a")  // GetConnectedNodes
};

// Forward declaration
class P2PMessage;
class ChainHead;
class P2PClient;
class ServerSession;
struct ConnectionInfo;

class P2PRequestEncoder {
  public:
    static P2PMessage info(const std::shared_ptr<const ChainHead> chainHead, const uint64_t &nNodes);
    static P2PMessage sendTransaction(const Tx::Base& transaction);
    static P2PMessage sendBulkTransactions(const std::vector<Tx::Base>& transactions);
    static P2PMessage requestBlockByNumber(const uint64_t& blockNumber);
    static P2PMessage requestBlockByHash(const Hash& block);
    static P2PMessage requestBlockRange(const uint64_t& startBlockNumber, const uint64_t& endBlockNumber);
    static P2PMessage newBestBlock(const Block& blockHash);
    static P2PMessage sendValidatorTransaction(const Tx::Validator& transaction);
    static P2PMessage sendBulkValidatorTransactions(const std::vector<Tx::Validator>& transactions);
    static P2PMessage requestValidatorTransactions();
    static P2PMessage getConnectedNodes();
};

// TODO: remaining Encoder/Decoder of the P2P Commands
class P2PRequestDecoder {
  public:
    static ConnectionInfo info(const P2PMessage& message);
    static Tx::Base sendTransaction(const P2PMessage& message);
    static Tx::Validator sendValidatorTransaction(const P2PMessage& message);
};

class P2PAnswerEncoder {
  public:
    static P2PMessage requestValidatorTransactions(const std::unordered_map<Hash, Tx::Validator, SafeHash>& transactions);
    static P2PMessage info(const std::shared_ptr<const ChainHead> chainHead, const uint64_t &nNodes, const std::string& id);
};

class P2PAnswerDecoder {
  public:
    static std::vector<Tx::Validator> requestValidatorTransactions(const P2PMessage& message);
    static ConnectionInfo info(const P2PMessage& message);
};


// Class for packing up messages to be sent/received.
// Message = 8 Bytes (Request ID) + 2 Bytes (Command Type) + Payload.
class P2PMessage {
  private:
    std::string _rawMessage;
    P2PMessage(std::string&& raw) : _rawMessage(std::move(raw)) {};
  public:
    
    const std::string_view message() const { return std::string_view(_rawMessage).substr(10); };
    const CommandType command() const { return getCommandType(_rawMessage.substr(8,2)); };
    const std::string id() const { return _rawMessage.substr(0, 8); };
    const std::string& raw() const { return _rawMessage; };

    friend class P2PRequestEncoder;
    friend class P2PAnswerEncoder;
    friend class P2PClient;
    friend class ServerSession;
};
#endif // P2PENCODING_H