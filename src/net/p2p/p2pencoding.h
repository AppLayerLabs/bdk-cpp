#ifndef P2PENCODING_H
#define P2PENCODING_H

#include <future>

#include "../../utils/utils.h"
#include "../../utils/safehash.h"
#include "../../utils/tx.h"
#include "../../utils/block.h"
#include "../../utils/options.h"

namespace P2P {
  // Forward declarations.
  class Manager;
  class Message;

  /// Enum for identifying which type of connection is being made.
  enum ConnectionType { SERVER, CLIENT };

  /**
   * Enum for identifying from which type is a given node.
   *
   * "Normal" P2P nodes follow all protocol rules, can answer any request,
   * and will broadcast requests to other nodes if the broadcast flag is used.
   *
   * "Discovery" P2P nodes only answer requests related to connection/discovery,
   * and will not broadcast requests to other nodes.
   */
  enum NodeType { NORMAL_NODE, DISCOVERY_NODE };

  /// Enum for identifying the type of a request.
  enum RequestType { Requesting, Answering, Broadcasting };

  /// Enum for identifying the type of a command.
  enum CommandType {
    Ping,
    Info,
    RequestNodes,
    RequestValidatorTxs,
    BroadcastValidatorTx,
    BroadcastTx,
    BroadcastBlock
  };

  /**
   * List of type prefixes (as per RequestType) for easy conversion.
   * Reference is as follows:
   * - "00" = %Request
   * - "01" = Answer
   * - "02" = Broadcast
   */
  inline extern const std::vector<Bytes> typePrefixes {
    Bytes(1, 0x00), // Request
    Bytes(1, 0x01), // Answer
    Bytes(1, 0x02)  // Broadcast
  };

  /**
   * List of command prefixes (as per CommandType) for easy conversion.
   * Reference is as follows:
   * - "0000" = Ping
   * - "0001" = Info
   * - "0002" = RequestNodes
   * - "0003" = RequestValidatorTxs
   * - "0004" = BroadcastValidatorTx
   * - "0005" = BroadcastTx
   * - "0006" = BroadcastBlock
   */
  inline extern const std::vector<Bytes> commandPrefixes {
    Bytes{0x00, 0x00}, // Ping
    Bytes{0x00, 0x01}, // Info
    Bytes{0x00, 0x02}, // RequestNodes
    Bytes{0x00, 0x03}, // RequestValidatorTxs
    Bytes{0x00, 0x04}, // BroadcastValidatorTx
    Bytes{0x00, 0x05}, // BroadcastTx
    Bytes{0x00, 0x06}  // BroadcastBlock
  };

  /**
   * Get the type of a request within a message.
   * @param message The message to parse.
   * @return The request type.
   */
  RequestType getRequestType(const BytesArrView message);

  /**
   * Get the 1-byte prefix of a given request inside typePrefixes.
   * @param type The request type to parse.
   * @return The prefix string.
   */
  const Bytes& getRequestTypePrefix(const RequestType& type);

  /**
   * Get the type of a command within a message.
   * @param message The message to parse.
   * @return The command type.
   */
  CommandType getCommandType(const BytesArrView message);

  /**
   * Get the 2-byte prefix of a given command inside commandPrefixes.
   * @param commType The command type to parse.
   * @return The prefix string.
   */
  const Bytes& getCommandPrefix(const CommandType& commType);

  /// Abstraction of an 8-byte/64-bit hash that represents a unique ID for a request. Inherits `FixedBytes<8>`.
  class RequestID : public FixedBytes<8> {
    public:
      // Using parent constructor and operator=.
      using FixedBytes<8>::FixedBytes;
      using FixedBytes<8>::operator=;

      /**
       * Constructor.
       * @param value The unsigned number to convert into a hash string.
       */
      RequestID(const uint64_t& value);

      /// Convert the hash string back to an unsigned number.
      uint64_t toUint64() const;

      /// Generate a random hash.
      static RequestID random();
  };

  /// Struct with information about a given node.
  struct NodeInfo {
    /// Node version.
    uint64_t nodeVersion = 0;

    /// Current epoch timestamp, in microseconds.
    uint64_t currentTimestamp = 0;

    /// Height of the latest block the node is at.
    uint64_t latestBlockHeight = 0;

    /// %Hash of the latest block the node is at.
    Hash latestBlockHash = Hash();

    /// Equality operator. Checks if all members are the same.
    bool operator==(const NodeInfo& other) const {
      return (
        nodeVersion == other.nodeVersion &&
        currentTimestamp == other.currentTimestamp &&
        latestBlockHeight == other.latestBlockHeight &&
        latestBlockHash == other.latestBlockHash
      );
    }

    /// Assignment operator.
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

  /// Helper class used to create requests.
  class RequestEncoder {
    public:
      /**
       * Create a `Ping` request.
       * @return The formatted request.
       */
      static Message ping();

      /**
       * Create a `Info` request.
       * @param latestBlock Pointer to the node's latest block.
       * @param options Pointer to the node's options singleton.
       * @return The formatted request.
       */
      static Message info(
        const std::shared_ptr<const Block>& latestBlock,
        const std::unique_ptr<Options>& options
      );

      /**
       * Create a `RequestNodes` request.
       * @return The formatted request.
       */
      static Message requestNodes();

      /**
       * Create a `RequestValidatorTxs` request.
       * @return The formatted request.
       */
      static Message requestValidatorTxs();
  };

  /// Helper class used to parse requests.
  class RequestDecoder {
    public:
      /**
       * Parse a `Ping` message.
       * @param message The message to parse.
       * @return `true` if the message is valid, `false` otherwise.
       */
      static bool ping(const Message& message);

      /**
       * Parse a `Info` message.
       * Throws if message is invalid.
       * @param message The message to parse.
       * @return A struct with the node's information.
       */
      static NodeInfo info(const Message& message);

      /**
       * Parse a `RequestNodes` message.
       * @param message The message to parse.
       * @return `true` if the message is valid, `false` otherwise.
       */
      static bool requestNodes(const Message& message);

      /**
       * Parse a `RequestValidatorTxs` message.
       * @param message The message to parse.
       * @return `true` if the message is valid, `false` otherwise.
       */
      static bool requestValidatorTxs(const Message& message);
  };

  /// Helper class used to create answers to requests.
  class AnswerEncoder {
    public:
      /**
       * Create a `Ping` answer.
       * @param request The request message.
       * @return The formatted answer.
       */
      static Message ping(const Message& request);

      /**
       * Create a `Info` answer.
       * @param request The request message.
       * @param latestBlock Pointer to the node's latest block.
       * @param options Pointer to the node's options singleton.
       * @return The formatted answer.
       */
      static Message info(const Message& request,
        const std::shared_ptr<const Block>& latestBlock,
        const std::unique_ptr<Options>& options
      );

      /**
       * Create a `RequestNodes` answer.
       * @param request The request message.
       * @param nodes The list of nodes to use as reference.
       * @return The formatted answer.
       */
      static Message requestNodes(const Message& request,
        const std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash>& nodes
      );

      /**
       * Create a `RequestValidatorTxs` answer.
       * @param request The request message.
       * @param txs The list of transactions to use as reference.
       * @return The formatted answer.
       */
      static Message requestValidatorTxs(const Message& request,
        const std::unordered_map<Hash, TxValidator, SafeHash>& txs
      );
  };

  /// Helper class used to parse answers to requests.
  class AnswerDecoder {
    public:
      /**
       * Parse a `Ping` answer.
       * @param message The answer to parse.
       * @return `true` if the message is valid, `false` otherwise.
       */
      static bool ping(const Message& message);

      /**
       * Parse a `Info` answer.
       * @param message The answer to parse.
       * @return A struct with the node's information.
       */
      static NodeInfo info(const Message& message);

      /**
       * Parse a `RequestNodes` answer.
       * @param message The answer to parse.
       * @return A list of requested nodes.
       */
      static std::unordered_map<
        Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash
      > requestNodes(const Message& message);

      /**
       * Parse a `RequestValidatorTxs` answer.
       * @param message The answer to parse.
       * @param requiredChainId The chain ID to use as reference.
       * @return A list of requested Validator transactions.
       */
      static std::vector<TxValidator> requestValidatorTxs(
        const Message& message, const uint64_t& requiredChainId
      );
  };

  /// Helper class used to create broadcast messages.
  class BroadcastEncoder {
    public:
      /**
       * Create a message to broadcast a Validator transaction.
       * @param tx The transaction to broadcast.
       * @return The formatted message.
       */
      static Message broadcastValidatorTx(const TxValidator& tx);

      /**
       * Create a message to broadcast a block transaction.
       * @param tx The transaction to broadcast.
       * @return The formatted message.
       */
      static Message broadcastTx(const TxBlock& tx);

      /**
       * Create a message to broadcast a whole block.
       * @param block The block to broadcast.
       * @return The formatted message.
       */
      static Message broadcastBlock(const std::shared_ptr<const Block>& block);
  };

  /// Helper class used to parse broadcast messages.
  class BroadcastDecoder {
    public:
      /**
       * Parse a broadcasted message for a Validator transaction.
       * @param message The message that was broadcast.
       * @param requiredChainId The chain ID to use as reference.
       * @return The built Validator transaction object.
       */
      static TxValidator broadcastValidatorTx(const Message& message, const uint64_t& requiredChainId);

      /**
       * Parse a broadcasted message for a block transaction.
       * @param message The message that was broadcast.
       * @param requiredChainId The chain ID to use as reference.
       * @return The built block transaction object.
       */
      static TxBlock broadcastTx(const Message& message, const uint64_t& requiredChainId);

      /**
       * Parse a broadcasted message for a whole block.
       * @param message The message that was broadcast.
       * @param requiredChainId The chain ID to use as reference.
       * @return The build block object.
       */
      static Block broadcastBlock(const Message& message, const uint64_t& requiredChainId);
  };

  /**
   * Abstraction of a %P2P message.
   * The structure is a bytes string (1 byte = 2 chars), as follows:
   * ```
   * 0x            00         0000000000000000       0000        00000000000000000000000000000000...
   * What:    Request Flag        Random ID       Command ID                Data...
   * Chars:        2                 16               4                       X
   * Bytes:        1                 8                2                      X/2
   * ```
   */
  class Message {
    private:
      /// The internal message data to be read/written, stored as bytes.
      Bytes _rawMessage;

      /// Raw string move constructor. Throws on invalid size.
      Message(Bytes&& raw) : _rawMessage(std::move(raw)) {
        if (_rawMessage.size() < 11) throw std::runtime_error("Invalid message size.");
      }

      /// Assignment operator.
      Message& operator=(const Message& message) {
        this->_rawMessage = message._rawMessage; return *this;
      }

    public:
      /// Copy constructor.
      Message(const Message& message) { this->_rawMessage = message._rawMessage; }

      /// Move constructor.
      Message(Message&& message) { this->_rawMessage = std::move(message._rawMessage); }

      /// Get the request type of the message.
      const RequestType type() const { return getRequestType(BytesArrView(_rawMessage).subspan(0,1)); }

      /// Get the request ID of the message.
      const RequestID id() const { return RequestID(BytesArrView(_rawMessage).subspan(1, 8)); }

      /// Get the command type of the message.
      const CommandType command() const { return getCommandType(BytesArrView(_rawMessage).subspan(9,2)); }

      /// Get the message data (without the flags and IDs).
      const BytesArrView message() const { return BytesArrView(_rawMessage).subspan(11); }

      /// Get the whole message.
      const BytesArrView raw() const { return _rawMessage; }

      /// Get the message's size.
      const size_t size() const { return _rawMessage.size(); }

      friend class RequestEncoder;
      friend class AnswerEncoder;
      friend class BroadcastEncoder;
      friend class ClientSession;
      friend class ServerSession;
      friend class Request;
  };

  /// Abstraction of a %P2P request, passed through the network.
  class Request {
    private:
      CommandType _command;           ///< Command type.
      RequestID _id;                  ///< Request ID.
      Hash _nodeId;                   ///< Host node ID.
      std::promise<Message> _answer;  ///< Answer to the request.
      bool _isAnswered = false;       ///< Indicates whether the request was answered.

    public:
      /**
       * Constructor.
       * @param command The request's command type.
       * @param id The request's ID.
       * @param nodeId The request's host node ID.
       */
      Request(const CommandType& command, const RequestID& id, const Hash& nodeId) : _command(command), _id(id), _nodeId(nodeId) {};

      /// Getter for `_command`.
      const CommandType& command() const { return _command; };

      /// Getter for `_id`.
      const RequestID& id() const { return _id; };

      /// Getter for `_nodeId`.
      const Hash& nodeId() const { return _nodeId; };

      /// Getter for `_answer`.
      std::future<Message> answerFuture() { return _answer.get_future(); };

      /// Getter for `_isAnswered`.
      const bool isAnswered() const { return _isAnswered; };

      /// Setter for `_answer`. Also sets `_isAnswered` to `true`.
      void setAnswer(const Message& answer) { _answer.set_value(answer); _isAnswered = true; };
  };
};

#endif  // P2PENCODING_H
