#include "encoding.h"

namespace P2P {
  RequestID::RequestID(const uint64_t& value) { this->data_ = Utils::uint64ToBytes(value); }

  uint64_t RequestID::toUint64() const { return Utils::bytesToUint64(this->data_); }

  RequestID RequestID::random() { return RequestID(Utils::randBytes(8)); }

  CommandType getCommandType(const BytesArrView message) {
    if (message.size() != 2) { throw std::runtime_error("Invalid Command Type size." + std::to_string(message.size())); }
    uint16_t commandType = Utils::bytesToUint16(message);
    if (commandType > commandPrefixes.size()) { throw std::runtime_error("Invalid command type."); }
    return static_cast<CommandType>(commandType);
  }

  const Bytes& getCommandPrefix(const CommandType& commType) { return commandPrefixes[commType]; }

  RequestType getRequestType(const BytesArrView message) {
    if (message.size() != 1) { throw std::runtime_error("Invalid Request Type size. " + std::to_string(message.size())); }
    uint8_t requestType = Utils::bytesToUint8(message);
    if (requestType > typePrefixes.size()) { throw std::runtime_error("Invalid request type."); }
    return static_cast<RequestType>(requestType);
  }

  const Bytes& getRequestTypePrefix(const RequestType& type) { return typePrefixes[type]; }

  Message RequestEncoder::ping() {
    Bytes message = getRequestTypePrefix(Requesting);
    message.reserve(message.size() + 8 + 2);
    Utils::appendBytes(message, Utils::randBytes(8));
    Utils::appendBytes(message, getCommandPrefix(Ping));
    return Message(std::move(message));
  }

  Message RequestEncoder::info(const std::shared_ptr<const Block>& latestBlock, const std::unique_ptr<Options> &options) {
    Bytes message = getRequestTypePrefix(Requesting);
    message.reserve(message.size() + 8 + 2 + 8 + 8 + 8 + 32);
    Utils::appendBytes(message, Utils::randBytes(8));
    Utils::appendBytes(message, getCommandPrefix(Info));
    Utils::appendBytes(message, Utils::uint64ToBytes(options->getVersion()));
    uint64_t currentEpoch = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    Utils::appendBytes(message, Utils::uint64ToBytes(currentEpoch));
    Utils::appendBytes(message, Utils::uint64ToBytes(latestBlock->getNHeight()));
    Utils::appendBytes(message, latestBlock->hash());
    return Message(std::move(message));
  }

  Message RequestEncoder::requestNodes() {
    Bytes message = getRequestTypePrefix(Requesting);
    message.reserve(message.size() + 8 + 2);
    Utils::appendBytes(message, Utils::randBytes(8));
    Utils::appendBytes(message, getCommandPrefix(RequestNodes));
    return Message(std::move(message));
  }

  Message RequestEncoder::requestValidatorTxs() {
    Bytes message = getRequestTypePrefix(Requesting);
    Utils::appendBytes(message, Utils::randBytes(8));
    Utils::appendBytes(message, getCommandPrefix(RequestValidatorTxs));
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
    uint64_t nodeVersion = Utils::bytesToUint64(message.message().subspan(0, 8));
    uint64_t nodeEpoch = Utils::bytesToUint64(message.message().subspan(8, 8));
    uint64_t nodeHeight = Utils::bytesToUint64(message.message().subspan(16, 8));
    Hash nodeHash(message.message().subspan(24, 32));
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
    Bytes message = getRequestTypePrefix(Answering);
    message.reserve(message.size() + 8 + 2);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(Ping));
    return Message(std::move(message));
  }

  Message AnswerEncoder::info(const Message& request,
    const std::shared_ptr<const Block>& latestBlock,
    const std::unique_ptr<Options> &options
  ) {
    Bytes message = getRequestTypePrefix(Answering);
    message.reserve(message.size() + 8 + 2 + 8 + 8 + 8 + 32);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(Info));
    Utils::appendBytes(message, Utils::uint64ToBytes(options->getVersion()));
    uint64_t currentEpoch = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    Utils::appendBytes(message, Utils::uint64ToBytes(currentEpoch));
    Utils::appendBytes(message, Utils::uint64ToBytes(latestBlock->getNHeight()));
    Utils::appendBytes(message, latestBlock->hash());
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestNodes(const Message& request,
    const std::unordered_map<NodeID, NodeType, SafeHash>& nodes
  ) {
    Bytes message = getRequestTypePrefix(Answering);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(RequestNodes));
    for (const auto& [nodeId, nodeType] : nodes) {
      const auto& [address, port] = nodeId;
      Utils::appendBytes(message, Utils::uint8ToBytes(nodeType)); // Node type
      Utils::appendBytes(message, Utils::uint8ToBytes(address.is_v4() ? 0 : 1));
      if (address.is_v4()) {
        auto addressBytes = address.to_v4().to_bytes();
        Utils::appendBytes(message, addressBytes);
      } else {
        auto addressBytes = address.to_v6().to_bytes();
        Utils::appendBytes(message, addressBytes);
      }
      Utils::appendBytes(message, Utils::uint16ToBytes(uint16_t(port)));
    }
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestValidatorTxs(const Message& request,
    const std::unordered_map<Hash, TxValidator, SafeHash>& txs
  ) {
    Bytes message = getRequestTypePrefix(Answering);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(RequestValidatorTxs));
    for (const auto& validatorTx : txs) {
      Bytes rlp = validatorTx.second.rlpSerialize();
      Utils::appendBytes(message, Utils::uint32ToBytes(rlp.size()));
      message.insert(message.end(), rlp.begin(), rlp.end());
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
    uint64_t nodeVersion = Utils::bytesToUint64(message.message().subspan(0, 8));
    uint64_t nodeEpoch = Utils::bytesToUint64(message.message().subspan(8, 8));
    uint64_t nodeHeight = Utils::bytesToUint64(message.message().subspan(16, 8));
    Hash nodeHash(message.message().subspan(24, 32));
    return NodeInfo(nodeVersion, nodeEpoch, nodeHeight, nodeHash);
  }

  std::unordered_map<
      NodeID, NodeType, SafeHash
  > AnswerDecoder::requestNodes(const Message& message) {
    if (message.type() != Answering) { throw std::runtime_error("Invalid message type."); }
    if (message.command() != RequestNodes) { throw std::runtime_error("Invalid command."); }
    std::unordered_map<NodeID, NodeType, SafeHash> nodes;

    BytesArrView data = message.message();
    size_t index = 0;
    while (index < data.size()) {
      boost::asio::ip::address address;
      if (data.size() < 8) { throw std::runtime_error("Invalid data size."); }
      NodeType nodeType = NodeType(Utils::bytesToUint8(data.subspan(index, 1)));
      index += 1;
      uint8_t ipVersion = Utils::bytesToUint8(data.subspan(index, 1));
      index += 1; // Move index to IP address
      if (ipVersion == 0) { // V4
        BytesArr<4> ipBytes;
        std::copy(data.begin() + index, data.begin() + index + 4, ipBytes.begin());
        address = boost::asio::ip::address_v4(ipBytes);
        index += 4;
      } else if (ipVersion == 1) { // V6
        BytesArr<16> ipBytes;
        std::copy(data.begin() + index, data.begin() + index + 16, ipBytes.begin());
        address = boost::asio::ip::address_v6(ipBytes);
        index += 16;
      } else {
        throw std::runtime_error("Invalid ip version.");
      }
      auto port = Utils::bytesToUint16(data.subspan(index, 2));
      nodes.insert({NodeID(address, port), nodeType});
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
    BytesArrView data = message.message();
    size_t index = 0;
    while (index < data.size()) {
      if (data.size() < 4) { throw std::runtime_error("Invalid data size."); }
      uint32_t txSize = Utils::bytesToUint32(data.subspan(index, 4));
      index += 4;
      if (data.size() < txSize) { throw std::runtime_error("Invalid data size."); }
      BytesArrView txData = data.subspan(index, txSize);
      index += txSize;
      txs.emplace_back(txData, requiredChainId);
    }
    return txs;
  }

  Message BroadcastEncoder::broadcastValidatorTx(const TxValidator& tx) {
    Bytes message = getRequestTypePrefix(Broadcasting);
    // We need to use std::hash instead of SafeHash
    // Because hashing with SafeHash will always be different between nodes
    Utils::appendBytes(message, Utils::uint64ToBytes(FNVHash()(tx.rlpSerialize())));
    Utils::appendBytes(message, getCommandPrefix(BroadcastValidatorTx));
    Utils::appendBytes(message, tx.rlpSerialize());
    return Message(std::move(message));
  }

  Message BroadcastEncoder::broadcastTx(const TxBlock& tx) {
    Bytes message = getRequestTypePrefix(Broadcasting);
    // We need to use std::hash instead of SafeHash
    // Because hashing with SafeHash will always be different between nodes
    Utils::appendBytes(message, Utils::uint64ToBytes(FNVHash()(tx.rlpSerialize())));
    Utils::appendBytes(message, getCommandPrefix(BroadcastTx));
    Utils::appendBytes(message, tx.rlpSerialize());
    return Message(std::move(message));
  }

  Message BroadcastEncoder::broadcastBlock(const std::shared_ptr<const Block>& block) {
    Bytes message = getRequestTypePrefix(Broadcasting);
    // We need to use std::hash instead of SafeHash
    // Because hashing with SafeHash will always be different between nodes
    Bytes serializedBlock = block->serializeBlock();
    Utils::appendBytes(message, Utils::uint64ToBytes(FNVHash()(serializedBlock)));
    Utils::appendBytes(message, getCommandPrefix(BroadcastBlock));
    message.insert(message.end(), serializedBlock.begin(), serializedBlock.end());
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

