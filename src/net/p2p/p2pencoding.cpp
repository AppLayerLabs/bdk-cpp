#include "p2pencoding.h"
#include "p2pbase.h"

namespace P2P {
  RequestID::RequestID(const uint64_t& value) { this->data = Utils::uint64ToBytes(value); }

  uint64_t RequestID::toUint64() const { return Utils::bytesToUint64(data); }

  RequestID RequestID::random() { return RequestID(Utils::randBytes(8)); }

  CommandType getCommandType(const std::string_view& message) {
    if (message.size() != 2) { throw std::runtime_error("Invalid Command Type size." + std::to_string(message.size())); }
    uint16_t commandType = Utils::bytesToUint16(message);
    if (commandType > commandPrefixes.size()) { throw std::runtime_error("Invalid command type."); }
    return static_cast<CommandType>(commandType);
  }

  std::string getCommandPrefix(const CommandType& commType) { return commandPrefixes[commType]; }

  RequestType getRequestType(const std::string_view& message) {
    if (message.size() != 1) { throw std::runtime_error("Invalid Request Type size. " + std::to_string(message.size())); }
    uint8_t requestType = Utils::bytesToUint8(message);
    if (requestType > typePrefixes.size()) { throw std::runtime_error("Invalid request type."); }
    return static_cast<RequestType>(requestType);
  }

  std::string getRequestTypePrefix(const RequestType& type) { return typePrefixes[type]; }

  Message RequestEncoder::ping() {
    std::string message;
    message += getRequestTypePrefix(Requesting);
    message += Utils::randBytes(8);
    message += getCommandPrefix(Ping);
    return Message(std::move(message));
  }

  Message RequestEncoder::info(const std::shared_ptr<const Block>& latestBlock, const std::unique_ptr<Options> &options) {
    std::string message;
    message += getRequestTypePrefix(Requesting);
    message += Utils::randBytes(8);
    message += getCommandPrefix(Info);
    message += Utils::uint64ToBytes(options->getVersion());
    uint64_t currentEpoch = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    message += Utils::uint64ToBytes(currentEpoch);
    message += Utils::uint64ToBytes(latestBlock->getNHeight());
    message += latestBlock->hash().get();
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

  NodeInfo RequestDecoder::info(const Message& message) {
    if (message.size() != 67) { throw std::runtime_error("Invalid Info message size."); }
    if (message.command() != Info) { throw std::runtime_error("Invalid Info message command."); }
    uint64_t nodeVersion = Utils::bytesToUint64(message.message().substr(0, 8));
    uint64_t nodeEpoch = Utils::bytesToUint64(message.message().substr(8, 8));
    uint64_t nodeHeight = Utils::bytesToUint64(message.message().substr(16, 8));
    Hash nodeHash(message.message().substr(24, 32));
    return NodeInfo(nodeVersion, nodeEpoch, nodeHeight, nodeHash);
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

  Message AnswerEncoder::info(const Message& request,
    const std::shared_ptr<const Block>& latestBlock,
    const std::unique_ptr<Options> &options
  ) {
    std::string message;
    message += getRequestTypePrefix(Answering);
    message += request.id().get();
    message += getCommandPrefix(Info);
    message += Utils::uint64ToBytes(options->getVersion());
    uint64_t currentEpoch = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    message += Utils::uint64ToBytes(currentEpoch);
    message += Utils::uint64ToBytes(latestBlock->getNHeight());
    message += latestBlock->hash().get();
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestNodes(const Message& request,
    const std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash>& nodes
  ) {
    std::string message;
    message += getRequestTypePrefix(Answering);
    message += request.id().get();
    message += getCommandPrefix(RequestNodes);
    for (const auto& node : nodes) {
      message += Utils::uint8ToBytes(std::get<0>(node.second)); // Node type
      message += node.first.get();  // Node ID
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

  Message AnswerEncoder::requestValidatorTxs(const Message& request,
    const std::unordered_map<Hash, TxValidator, SafeHash>& txs
  ) {
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

  NodeInfo AnswerDecoder::info(const Message& message) {
    if (message.type() != Answering) { throw std::runtime_error("Invalid message type."); }
    if (message.command() != Info) { throw std::runtime_error("Invalid command."); }
    uint64_t nodeVersion = Utils::bytesToUint64(message.message().substr(0, 8));
    uint64_t nodeEpoch = Utils::bytesToUint64(message.message().substr(8, 8));
    uint64_t nodeHeight = Utils::bytesToUint64(message.message().substr(16, 8));
    Hash nodeHash(message.message().substr(24, 32));
    return NodeInfo(nodeVersion, nodeEpoch, nodeHeight, nodeHash);
  }

  std::unordered_map<
    Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash
  > AnswerDecoder::requestNodes(const Message& message) {
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

  std::vector<TxValidator> AnswerDecoder::requestValidatorTxs(
    const Message& message, const uint64_t& requiredChainId
  ) {
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
      txs.emplace_back(txData, requiredChainId);
    }
    return txs;
  }

  Message BroadcastEncoder::broadcastValidatorTx(const TxValidator& tx) {
    std::string message;
    message += getRequestTypePrefix(Broadcasting);
    // We need to use std::hash instead of SafeHash
    // Because hashing with SafeHash will always be different between nodes
    message += Utils::uint64ToBytes(FNVHash()(tx.rlpSerialize()));
    message += getCommandPrefix(BroadcastValidatorTx);
    message += tx.rlpSerialize();
    return Message(std::move(message));
  }

  Message BroadcastEncoder::broadcastTx(const TxBlock& tx) {
    std::string message;
    message += getRequestTypePrefix(Broadcasting);
    // We need to use std::hash instead of SafeHash
    // Because hashing with SafeHash will always be different between nodes
    message += Utils::uint64ToBytes(FNVHash()(tx.rlpSerialize()));
    message += getCommandPrefix(BroadcastTx);
    message += tx.rlpSerialize();
    return Message(std::move(message));
  }

  Message BroadcastEncoder::broadcastBlock(const std::shared_ptr<const Block>& block) {
    std::string message;
    message += getRequestTypePrefix(Broadcasting);
    // We need to use std::hash instead of SafeHash
    // Because hashing with SafeHash will always be different between nodes
    std::string serializedBlock = block->serializeBlock();
    message += Utils::uint64ToBytes(FNVHash()(serializedBlock));
    message += getCommandPrefix(BroadcastBlock);
    message += serializedBlock;
    return Message(std::move(message));
  }

  TxValidator BroadcastDecoder::broadcastValidatorTx(const Message& message, const uint64_t& requiredChainId) {
    if (message.type() != Broadcasting) { throw std::runtime_error("Invalid message type."); }
    if (message.id().toUint64() != FNVHash()(message.message())) { throw std::runtime_error("Invalid message id."); }
    if (message.command() != BroadcastValidatorTx) { throw std::runtime_error("Invalid command."); }
    return TxValidator(message.message(), requiredChainId);
  }

  TxBlock BroadcastDecoder::broadcastTx(const P2P::Message &message, const uint64_t &requiredChainId) {
    if (message.type() != Broadcasting) { throw std::runtime_error("Invalid message type."); }
    if (message.id().toUint64() != FNVHash()(message.message())) { throw std::runtime_error("Invalid message id."); }
    if (message.command() != BroadcastTx) { throw std::runtime_error("Invalid command."); }
    return TxBlock(message.message(), requiredChainId);
  }

  Block BroadcastDecoder::broadcastBlock(const P2P::Message &message, const uint64_t &requiredChainId) {
    if (message.type() != Broadcasting) { throw std::runtime_error("Invalid message type."); }
    if (message.id().toUint64() != FNVHash()(message.message())) { throw std::runtime_error("Invalid message id. "); }
    if (message.command() != BroadcastBlock) { throw std::runtime_error("Invalid command."); }
    return Block(message.message(), requiredChainId);
  }
}

