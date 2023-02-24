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

  Message AnswerEncoder::ping(const Message& request) {
    std::string message;
    message += getRequestTypePrefix(Answering);
    message += request.id().get();
    message += getCommandPrefix(Ping);
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestNodes(const Message& request, const std::vector<std::tuple<NodeType, Hash, boost::asio::ip::address, unsigned short>>& nodes) {
    std::string message;
    message += getRequestTypePrefix(Answering);
    message += request.id().get();
    message += getCommandPrefix(RequestNodes);
    for (const auto& node : nodes) {
      // NodeType
      message += Utils::uint8ToBytes(std::get<0>(node));
      // NodeID
      message += std::get<1>(node).get();

      message += Utils::uint8ToBytes(std::get<2>(node).is_v4() ? 0 : 1);
      if (std::get<2>(node).is_v4()) {
        auto address = std::get<2>(node).to_v4().to_bytes();
        message += std::string(address.begin(), address.end());
      } else {
        auto address = std::get<2>(node).to_v6().to_bytes();
        message += std::string(address.begin(), address.end());
      }
      message += Utils::uint16ToBytes(uint16_t(std::get<3>(node)));
    }
    return Message(std::move(message));
  }

  bool AnswerDecoder::ping(const Message& message) {
    if (message.size() != 11) { return false; }
    if (message.type() != Answering) { return false; }
    if (message.command() != Ping) { return false; }
    return true;
  }

  std::vector<std::tuple<NodeType, Hash, boost::asio::ip::address, unsigned short>> AnswerDecoder::requestNodes(const Message& message) {
    if (message.type() != Answering) { throw std::runtime_error("Invalid message type."); }
    if (message.command() != RequestNodes) { throw std::runtime_error("Invalid command."); }
    std::vector<std::tuple<NodeType, Hash, boost::asio::ip::address, unsigned short>> nodes;
    std::string_view data = message.message();
    size_t index = 0;
    while (index < data.size()) {
      if (data.size() < 40) { throw std::runtime_error("Invalid data size."); }
      std::tuple<NodeType, Hash, boost::asio::ip::address, unsigned short> node;
      std::get<0>(node) = static_cast<NodeType>(Utils::bytesToUint8(data.substr(index, 1)));
      index += 1;
      std::get<1>(node) = Hash(std::string(data.substr(index, 32)));
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
        std::get<2>(node) = boost::asio::ip::address::from_string(ip);
        index += 4;
      } else if (ipVersion == 1) { // V6
        std::string ip;
        auto ipBytes = data.substr(index, 16);
        for (const char& byte : ipBytes) {
          ip += std::to_string(uint8_t(byte));
          ip += ':';
        }
        ip.pop_back();
        std::get<2>(node) = boost::asio::ip::address::from_string(ip);
        index += 16;
      } else {
        throw std::runtime_error("Invalid ip version.");
      }
      std::get<3>(node) = Utils::bytesToUint16(data.substr(index, 2));
      nodes.push_back(node);
      index += 2;
    }
    return nodes;
  }
}