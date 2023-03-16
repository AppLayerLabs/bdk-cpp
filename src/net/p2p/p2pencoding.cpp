#include "p2pencoding.h"

namespace P2P {
  RequestID::RequestID(const uint64_t& value) {
    this->data = Utils::uint64ToBytes(value);
  }

  uint64_t RequestID::toUint64() const {
    return Utils::bytesToUint64(data);
  }

  RequestID RequestID::random() {
    return RequestID(Utils::randBytes(8));
  }


  CommandType getCommandType(const std::string_view& message) {
    if (message.size() != 2) { throw std::runtime_error("Invalid Command Type size." + std::to_string(message.size())); }
    uint16_t commandType = Utils::bytesToUint16(message);
    if (commandType > commandPrefixes.size()) { throw std::runtime_error("Invalid command type."); }
    return static_cast<CommandType>(commandType);
  }
  
  std::string getCommandPrefix(const CommandType& commType) {
    return commandPrefixes[commType];
  }

  RequestType getRequestType(const std::string_view& message) {
    if (message.size() != 1) { throw std::runtime_error("Invalid Request Type size. " + std::to_string(message.size())); }
    uint8_t requestType = Utils::bytesToUint8(message);
    if (requestType > typePrefixes.size()) { throw std::runtime_error("Invalid request type."); }
    return static_cast<RequestType>(requestType);
  }

  std::string getRequestTypePrefix(const RequestType& type) {
    return typePrefixes[type];
  }

  Message RequestEncoder::ping() {
    std::string message;
    message += getRequestTypePrefix(Requesting);
    message += Utils::randBytes(8);
    message += getCommandPrefix(Ping);
    return Message(std::move(message));
  }

  Message RequestEncoder::requestNodes() {
    std::string message;
    message += getRequestTypePrefix(Requesting);
    message += Utils::randBytes(8);
    message += getCommandPrefix(RequestNodes);
    return Message(std::move(message));
  }

  Message RequestEncoder::requestValidatorTxs() {
    std::string message;
    message += getRequestTypePrefix(Requesting);
    message += Utils::randBytes(8);
    message += getCommandPrefix(RequestValidatorTxs);
    return Message(std::move(message));
  }

  bool RequestDecoder::ping(const Message& message) {
    if (message.size() != 11) { return false; }
    if (message.command() != Ping) { return false; }
    return true;
  }

  bool RequestDecoder::requestNodes(const Message& message) {
    if (message.size() != 11) { return false; }
    if (message.command() != RequestNodes) { return false; }
    return true;
  }

  bool RequestDecoder::requestValidatorTxs(const Message& message) {
    if (message.size() != 11) { return false; }
    if (message.command() != RequestValidatorTxs) { return false; }
    return true;
  }

  Message AnswerEncoder::ping(const Message& request) {
    std::string message;
    message += getRequestTypePrefix(Answering);
    message += request.id().get();
    message += getCommandPrefix(Ping);
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestNodes(const Message& request, const std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash>& nodes) {
    std::string message;
    message += getRequestTypePrefix(Answering);
    message += request.id().get();
    message += getCommandPrefix(RequestNodes);
    for (const auto& node : nodes) {
      // NodeType
      message += Utils::uint8ToBytes(std::get<0>(node.second));
      // NodeID
      message += node.first.get();

      message += Utils::uint8ToBytes(std::get<1>(node.second).is_v4() ? 0 : 1);
      if (std::get<1>(node.second).is_v4()) {
        auto address = std::get<1>(node.second).to_v4().to_bytes();
        message += std::string(address.begin(), address.end());
      } else {
        auto address = std::get<1>(node.second).to_v6().to_bytes();
        message += std::string(address.begin(), address.end());
      }
      message += Utils::uint16ToBytes(uint16_t(std::get<2>(node.second)));
    }
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestValidatorTxs(const Message& request, const std::unordered_map<Hash, TxValidator, SafeHash>& txs) {
    std::string message;
    message += getRequestTypePrefix(Answering);
    message += request.id().get();
    message += getCommandPrefix(RequestValidatorTxs);
    for (const auto& validatorTx : txs) {
      std::string rlp = validatorTx.second.rlpSerialize();
      message += Utils::uint32ToBytes(rlp.size());
      message += rlp;
    }
    return Message(std::move(message));
  }

  bool AnswerDecoder::ping(const Message& message) {
    if (message.size() != 11) { return false; }
    if (message.type() != Answering) { return false; }
    if (message.command() != Ping) { return false; }
    return true;
  }

  std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> AnswerDecoder::requestNodes(const Message& message) {
    if (message.type() != Answering) { throw std::runtime_error("Invalid message type."); }
    if (message.command() != RequestNodes) { throw std::runtime_error("Invalid command."); }
    std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> nodes;
    std::string_view data = message.message();
    size_t index = 0;
    while (index < data.size()) {
      if (data.size() < 40) { throw std::runtime_error("Invalid data size."); }
      std::tuple<NodeType, boost::asio::ip::address, unsigned short> node;
      std::get<0>(node) = static_cast<NodeType>(Utils::bytesToUint8(data.substr(index, 1)));
      index += 1;
      Hash nodeId = Hash(std::string(data.substr(index, 32)));
      index += 32;
      uint8_t ipVersion = Utils::bytesToUint8(data.substr(index, 1));
      index += 1; // Move index to IP address
      if (ipVersion == 0) { // V4
        std::string ip;
        auto ipBytes = data.substr(index, 4);
        for (const char& byte : ipBytes) {
          ip += std::to_string(uint8_t(byte));
          ip += '.';
        }
        ip.pop_back();
        std::get<1>(node) = boost::asio::ip::address::from_string(ip);
        index += 4;
      } else if (ipVersion == 1) { // V6
        std::string ip;
        auto ipBytes = data.substr(index, 16);
        for (const char& byte : ipBytes) {
          ip += std::to_string(uint8_t(byte));
          ip += ':';
        }
        ip.pop_back();
        std::get<1>(node) = boost::asio::ip::address::from_string(ip);
        index += 16;
      } else {
        throw std::runtime_error("Invalid ip version.");
      }
      std::get<2>(node) = Utils::bytesToUint16(data.substr(index, 2));
      nodes[nodeId] = node;
      index += 2;
    }
    return nodes;
  }

  std::vector<TxValidator> AnswerDecoder::requestValidatorTxs(const Message& message) {
    if (message.type() != Answering) { throw std::runtime_error("Invalid message type."); }
    if (message.command() != RequestValidatorTxs) { throw std::runtime_error("Invalid command."); }
    std::vector<TxValidator> txs;
    std::string_view data = message.message();
    size_t index = 0;
    while (index < data.size()) {
      if (data.size() < 4) { throw std::runtime_error("Invalid data size."); }
      uint32_t txSize = Utils::bytesToUint32(data.substr(index, 4));
      index += 4;
      if (data.size() < txSize) { throw std::runtime_error("Invalid data size."); }
      std::string_view txData = data.substr(index, txSize);
      index += txSize;
      txs.push_back(txData);
    }
    return txs;
  }

  Message BroadcastEncoder::broadcastValidatorTx(const TxValidator& tx) {
    std::string message;
    message += getRequestTypePrefix(Broadcasting);
    message += getCommandPrefix(BroadcastValidatorTx);
    message += tx.rlpSerialize();
    return Message(std::move(message));
  }

  TxValidator BroadcastDecoder::broadcastValidatorTx(const Message& message) {
    if (message.type() != Broadcasting) { throw std::runtime_error("Invalid message type."); }
    if (message.command() != BroadcastValidatorTx) { throw std::runtime_error("Invalid command."); }
    return TxValidator(message.message());
  }
}