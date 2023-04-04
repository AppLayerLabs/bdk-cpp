#ifndef P2PENCODING_H
#define P2PENCODING_H

#include "../../utils/utils.h"
#include "../../utils/safehash.h"
#include "../../utils/tx.h"
#include "../../utils/block.h"
#include "../../utils/options.h"
#include <future>

namespace P2P {

  // Foward declaration.
  class Manager;
  class Message;

  enum ConnectionType { SERVER, CLIENT };

  // Normal P2P node follows all rules of protocol and can answer for
  // any request, will broadcast requests to other nodes if broadcast
  // flag is used.
  // Discovery P2P node will only answer requests related to
  // connection/discovery and will not broadcast requests to other nodes.
  enum NodeType { NORMAL_NODE, DISCOVERY_NODE };

  /// Abstraction of a 8-byte hash. Inherits `FixedStr<32>`.
  class RequestID : public FixedStr<8> {
    public:
      using FixedStr<8>::FixedStr; ///< Using parent constructor.
      using FixedStr<8>::operator=; ///< Using parent operator=.

      /**
       * Constructor.
       * @param data The unsigned 256-bit number to convert into a hash string.
       */
      RequestID(const uint64_t& value);

      /// Convert the hash string back to an unsigned 256-bit number.
      uint64_t toUint64() const;

      /// Generate a random 32-byte/256-bit hash.
      static RequestID random();
  };
  
  enum RequestType {
    Requesting,
    Answering,
    Broadcasting
  };
  
  enum CommandType {
    Ping,
    Info,
    RequestNodes,
    RequestValidatorTxs,
    BroadcastValidatorTx,
    BroadcastTx,
    BroadcastBlock
  };
  
  CommandType getCommandType(const std::string_view& message);
  std::string getCommandPrefix(const CommandType& commType);
  
  // Vector for easy conversion to command prefixes.
  inline extern const std::vector<std::string> commandPrefixes {
    std::string("\x00\x00", 2),       // Ping
    std::string("\x00\x01", 2),       // Info
    std::string("\x00\x02", 2),       // requestNodes
    std::string("\x00\x03", 2),       // RequestValidatorTxs
    std::string("\x00\x04", 2),       // BroadcastValidatorTx
    std::string("\x00\x05", 2),       // BroadcastTx
    std::string("\x00\x06", 2)        // BroadcastBlock
  };
  
  RequestType getRequestType(const std::string_view& message);
  std::string getRequestTypePrefix(const RequestType& type);
  
  // Vector for easy conversion to type prefixes.
  inline extern const std::vector<std::string> typePrefixes {
    std::string("\x00", 1),       // Request
    std::string("\x01", 1),       // Answer
    std::string("\x02", 1)        // Broadcast
  };
  
  struct NodeInfo {
    uint64_t nodeVersion = 0;
    uint64_t currentTimestamp = 0;
    uint64_t latestBlockHeight = 0;
    Hash latestBlockHash = Hash();

    bool operator==(const NodeInfo& other) const {
      return nodeVersion == other.nodeVersion &&
             currentTimestamp == other.currentTimestamp &&
             latestBlockHeight == other.latestBlockHeight &&
             latestBlockHash == other.latestBlockHash;
    }

    NodeInfo& operator=(const NodeInfo& other) {
      if (this != &other) {
        nodeVersion = other.nodeVersion;
        currentTimestamp = other.currentTimestamp;
        latestBlockHeight = other.latestBlockHeight;
        latestBlockHash = other.latestBlockHash;
      }
      return *this;
    }

  };
  
  // Used when creating a request
  class RequestEncoder {
    public:
      static Message ping();
      static Message info(const std::shared_ptr<const Block>& latestBlock, const std::unique_ptr<Options> &options);
      static Message requestNodes();
      static Message requestValidatorTxs();
  };
  
  // Used to decode a request.
  class RequestDecoder {
    public:
      static bool ping(const Message& message);
      static NodeInfo info(const Message& message);
      static bool requestNodes(const Message& message);
      static bool requestValidatorTxs(const Message& message);
  };
  
  // Used to encode a answer to request.
  class AnswerEncoder {
    public:
      static Message ping(const Message& request);
      static Message info(const Message& request, const std::shared_ptr<const Block>& latestBlock, const std::unique_ptr<Options> &options);
      static Message requestNodes(const Message& request, const std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash>& nodes);
      static Message requestValidatorTxs(const Message& request, const std::unordered_map<Hash, TxValidator, SafeHash>& txs); 
  };
  
  // Used to decode a answer to request.
  class AnswerDecoder {
    public:
      static bool ping(const Message& message);
      static NodeInfo info(const Message& message);
      static std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> requestNodes(const Message& message);
      static std::vector<TxValidator> requestValidatorTxs(const Message& message, const uint64_t& requiredChainId);
  };

  class BroadcastEncoder {
    public:
      static Message broadcastValidatorTx(const TxValidator& tx);
      static Message broadcastTx(const TxBlock& tx);
      static Message broadcastBlock(const std::shared_ptr<const Block>& block);
  };

  class BroadcastDecoder {
    public:
      static TxValidator broadcastValidatorTx(const Message& message, const uint64_t& requiredChainId);
      static TxBlock broadcastTx(const Message& message, const uint64_t& requiredChainId);
      static Block broadcastBlock(const Message& message, const uint64_t& requiredChainId);
  };

  class Message {
    private:
      /**
       * The internal message data to be read/written, stored as bytes (1 byte = 2 chars).
       * The structure of a message is as follows:
       *
       * 0x          00             0000000000000000     0000    00000000000000000000000000000000...
       * What:  Request Flag            Random ID       Cmd ID               Data...
       * Chars:      2                     16             4                     X
       * Bytes:      1                      8             2                    X/2
       *
       */
    
      std::string _rawMessage;

      Message(std::string&& raw) : _rawMessage(std::move(raw)) { if (_rawMessage.size() < 11) throw std::runtime_error("Invalid message size."); };

      Message& operator=(const Message& message) {
        this->_rawMessage = message._rawMessage; return *this;
      };

    public:
      Message(const Message& message) {
        this->_rawMessage = message._rawMessage;
      };
      
      Message(Message&& message) {
        this->_rawMessage = std::move(message._rawMessage);
      };
      const RequestType type() const { return getRequestType(_rawMessage.substr(0,1)); };
      const RequestID id() const { return RequestID(_rawMessage.substr(1, 8)); };
      const CommandType command() const { return getCommandType(_rawMessage.substr(9,2)); };
      const std::string_view message() const { return std::string_view(_rawMessage).substr(11); };
      const std::string_view raw() const { return _rawMessage; };
      const size_t size() const { return _rawMessage.size(); };

      friend class RequestEncoder;
      friend class AnswerEncoder;
      friend class BroadcastEncoder; 
      friend class ClientSession;
      friend class ServerSession;
      friend class Request;
  };

  class Request {
    private:
      CommandType _command; // The command type.
      RequestID _id;      // The request id.
      Hash _nodeId;  // The host node id.
      std::promise<Message> _answer;      // The answer to the request.
      bool _isAnswered = false;

    public:
      Request(const CommandType& command, const RequestID& id, const Hash& nodeId) : _command(command), _id(id), _nodeId(nodeId) {};
      const CommandType& command() const { return _command; };
      const RequestID& id() const { return _id; };
      const Hash& nodeId() const { return _nodeId; };
      std::future<Message> answerFuture() { return _answer.get_future(); };
      const bool isAnswered() const { return _isAnswered; };
      void setAnswer(const Message& answer) { _answer.set_value(answer); _isAnswered = true; };
  };
};

#endif 
