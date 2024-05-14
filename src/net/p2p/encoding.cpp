/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "encoding.h"

namespace P2P {

  // ------------------------------------------------------------------------------------------------------------------
  // Serialization/deserialization helpers.
  // These are shared between messages of various types that share the same encoding and decoding patterns.
  // ------------------------------------------------------------------------------------------------------------------

  std::unordered_map<NodeID, NodeType, SafeHash> nodesFromMessage(const BytesArrView& data) {
    std::unordered_map<NodeID, NodeType, SafeHash> nodes;
    size_t index = 0;
    while (index < data.size()) {
      boost::asio::ip::address address;
      if (data.size() < 8) { throw DynamicException("Invalid data size."); }
      auto nodeType = NodeType(Utils::bytesToUint8(data.subspan(index, 1)));
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
        throw DynamicException("Invalid ip version.");
      }
      auto port = Utils::bytesToUint16(data.subspan(index, 2));
      nodes.insert({NodeID(address, port), nodeType});
      index += 2;
    }
    return nodes;
  }

  void nodesToMessage(Bytes& message, const std::unordered_map<NodeID, NodeType, SafeHash>& nodes) {
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
  }

  NodeInfo nodeInfoFromMessage(const BytesArrView& data) {
    uint64_t nodeVersion = Utils::bytesToUint64(data.subspan(0, 8));
    uint64_t nodeEpoch = Utils::bytesToUint64(data.subspan(8, 8));
    uint64_t nodeHeight = Utils::bytesToUint64(data.subspan(16, 8));
    Hash nodeHash(data.subspan(24, 32));
    uint64_t currentEpoch = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    int64_t diff = currentEpoch - nodeEpoch;
    auto peers = nodesFromMessage(data.subspan(56));
    return NodeInfo(nodeVersion, nodeEpoch, currentEpoch, diff, nodeHeight, nodeHash, peers);
  }

  void nodeInfoToMessage(
    Bytes& message,
    const std::shared_ptr<const FinalizedBlock>& latestBlock,
    const std::unordered_map<NodeID, NodeType, SafeHash>& nodes,
    const Options& options)
  {
    Utils::appendBytes(message, Utils::uint64ToBytes(options.getVersion()));
    uint64_t currentEpoch = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch()
    ).count();
    Utils::appendBytes(message, Utils::uint64ToBytes(currentEpoch));
    Utils::appendBytes(message, Utils::uint64ToBytes(latestBlock->getNHeight()));
    Utils::appendBytes(message, latestBlock->getHash());
    nodesToMessage(message, nodes);
  }

  template<typename TxType>
  std::vector<TxType> txsFromMessage(const BytesArrView& data, const uint64_t& requiredChainId) {
    std::vector<TxType> txs;
    size_t index = 0;
    while (index < data.size()) {
      if (data.size() < 4) { throw DynamicException("Invalid data size."); }
      uint32_t txSize = Utils::bytesToUint32(data.subspan(index, 4));
      index += 4;
      if (data.size() < txSize) { throw DynamicException("Invalid data size."); }
      BytesArrView txData = data.subspan(index, txSize);
      index += txSize;
      // Assuming requiredChainId is declared elsewhere
      txs.emplace_back(txData, requiredChainId);
    }
    return txs;
  }

  // FIXME/TODO: This duplication is pointless; Make it into one template or get rid of it.
  template<typename TxType>
  void txsToMessage(Bytes& message, const std::unordered_map<Hash, TxType, SafeHash>& txs) {
    for (const auto& [txHash, tx] : txs) {
      Bytes rlp = tx.rlpSerialize();
      Utils::appendBytes(message, Utils::uint32ToBytes(rlp.size()));
      message.insert(message.end(), rlp.begin(), rlp.end());
    }
  }
  template<typename TxType>
  void txsToMessage(Bytes& message, const std::vector<TxType>& txs) {
    for (const auto& tx : txs) {
      Bytes rlp = tx.rlpSerialize();
      Utils::appendBytes(message, Utils::uint32ToBytes(rlp.size()));
      message.insert(message.end(), rlp.begin(), rlp.end());
    }
  }

  std::vector<FinalizedBlock> blocksFromMessage(const BytesArrView& data, const uint64_t& requiredChainId) {
    std::vector<FinalizedBlock> blocks;
    size_t index = 0;
    while (index < data.size()) {
      if (data.size() < 8) { throw DynamicException("Invalid data size."); }
      uint64_t blockSize = Utils::bytesToUint64(data.subspan(index, 8));
      index += 8;
      if (data.size() < blockSize) { throw DynamicException("Invalid data size."); }
      BytesArrView blockData = data.subspan(index, blockSize);
      index += blockSize;
      blocks.emplace_back(FinalizedBlock::fromBytes(blockData, requiredChainId));
    }
    return blocks;
  }

  void blocksToMessage(Bytes& message, const std::vector<std::shared_ptr<const FinalizedBlock>>& blocks) {
    for (const auto& block : blocks) {
      Bytes serializedBlock = block->serializeBlock();
      Utils::appendBytes(message, Utils::uint64ToBytes(serializedBlock.size()));
      Utils::appendBytes(message, serializedBlock);
    }
  }

  // ------------------------------------------------------------------------------------------------------------------
  // Implementation of all network messages that are in encoding.h (common code is in the helpers above).
  // ------------------------------------------------------------------------------------------------------------------

  RequestID::RequestID(const uint64_t& value) { this->data_ = Utils::uint64ToBytes(value); }

  uint64_t RequestID::toUint64() const { return Utils::bytesToUint64(this->data_); }

  RequestID RequestID::random() { return RequestID(Utils::randBytes(8)); }

  CommandType getCommandType(const BytesArrView message) {
    if (message.size() != 2) { throw DynamicException("Invalid Command Type size." + std::to_string(message.size())); }
    uint16_t commandType = Utils::bytesToUint16(message);
    if (commandType > commandPrefixes.size()) { throw DynamicException("Invalid command type."); }
    return static_cast<CommandType>(commandType);
  }

  const Bytes& getCommandPrefix(const CommandType& commType) { return commandPrefixes[commType]; }

  RequestType getRequestType(const BytesArrView message) {
    if (message.size() != 1) { throw DynamicException("Invalid Request Type size. " + std::to_string(message.size())); }
    uint8_t requestType = Utils::bytesToUint8(message);
    if (requestType > typePrefixes.size()) { throw DynamicException("Invalid request type."); }
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

  Message RequestEncoder::info(
    const std::shared_ptr<const FinalizedBlock>& latestBlock,
    const std::unordered_map<NodeID, NodeType, SafeHash>& nodes,
    const Options& options)
  {
    Bytes message = getRequestTypePrefix(Requesting);
    Utils::appendBytes(message, Utils::randBytes(8));
    Utils::appendBytes(message, getCommandPrefix(Info));
    nodeInfoToMessage(message, latestBlock, nodes, options);
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

  Message RequestEncoder::requestTxs() {
    Bytes message = getRequestTypePrefix(Requesting);
    Utils::appendBytes(message, Utils::randBytes(8));
    Utils::appendBytes(message, getCommandPrefix(RequestTxs));
    return Message(std::move(message));
  }

  Message RequestEncoder::requestBlock(uint64_t height, uint64_t heightEnd, uint64_t bytesLimit) {
    Bytes message = getRequestTypePrefix(Requesting);
    Utils::appendBytes(message, Utils::randBytes(8));
    Utils::appendBytes(message, getCommandPrefix(RequestBlock));
    Utils::appendBytes(message, Utils::uint64ToBytes(height));
    Utils::appendBytes(message, Utils::uint64ToBytes(heightEnd));
    Utils::appendBytes(message, Utils::uint64ToBytes(bytesLimit));
    return Message(std::move(message));
  }


  bool RequestDecoder::ping(const Message& message) {
    if (message.size() != 11) { return false; }
    if (message.command() != Ping) { return false; }
    return true;
  }

  NodeInfo RequestDecoder::info(const Message& message) {
    if (message.command() != Info) { throw DynamicException("Invalid Info message command."); }
   return nodeInfoFromMessage(message.message());
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

  bool RequestDecoder::requestTxs(const Message& message) {
    if (message.size() != 11) { return false; }
    if (message.command() != RequestTxs) { return false; }
    return true;
  }

  void RequestDecoder::requestBlock(const Message& message, uint64_t& height, uint64_t& heightEnd, uint64_t& bytesLimit) {
    if (message.size() != 35) { throw DynamicException("Invalid RequestBlock message size."); }
    if (message.command() != RequestBlock) { throw DynamicException("Invalid RequestBlock message command."); }
    height = Utils::bytesToUint64(message.message().subspan(0, 8));
    heightEnd = Utils::bytesToUint64(message.message().subspan(8, 8));
    bytesLimit = Utils::bytesToUint64(message.message().subspan(16, 8));
  }

  Message AnswerEncoder::ping(const Message& request) {
    Bytes message = getRequestTypePrefix(Answering);
    message.reserve(message.size() + 8 + 2);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(Ping));
    return Message(std::move(message));
  }

  Message AnswerEncoder::info(const Message& request,
    const std::shared_ptr<const FinalizedBlock>& latestBlock,
    const std::unordered_map<NodeID, NodeType, SafeHash>& nodes,
    const Options& options
  ) {
    Bytes message = getRequestTypePrefix(Answering);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(Info));
    nodeInfoToMessage(message, latestBlock, nodes, options);
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestNodes(const Message& request,
    const std::unordered_map<NodeID, NodeType, SafeHash>& nodes
  ) {
    Bytes message = getRequestTypePrefix(Answering);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(RequestNodes));
    nodesToMessage(message, nodes);
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestValidatorTxs(const Message& request,
    const std::unordered_map<Hash, TxValidator, SafeHash>& txs
  ) {
    Bytes message = getRequestTypePrefix(Answering);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(RequestValidatorTxs));
    txsToMessage<TxValidator>(message, txs);
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestTxs(const Message& request,
    const std::vector<TxBlock>& txs
  ) {
    Bytes message = getRequestTypePrefix(Answering);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(RequestTxs));
    txsToMessage<TxBlock>(message, txs);
    return Message(std::move(message));
  }

  Message AnswerEncoder::requestBlock(const Message& request,
    const std::vector<std::shared_ptr<const FinalizedBlock>>& blocks
  ) {
    Bytes message = getRequestTypePrefix(Answering);
    Utils::appendBytes(message, request.id());
    Utils::appendBytes(message, getCommandPrefix(RequestBlock));
    blocksToMessage(message, blocks);
    return Message(std::move(message));
  }

  bool AnswerDecoder::ping(const Message& message) {
    if (message.size() != 11) { return false; }
    if (message.type() != Answering) { return false; }
    if (message.command() != Ping) { return false; }
    return true;
  }

  NodeInfo AnswerDecoder::info(const Message& message) {
    if (message.type() != Answering) { throw DynamicException("Invalid message type."); }
    if (message.command() != Info) { throw DynamicException("Invalid command."); }
    return nodeInfoFromMessage(message.message());
  }

  std::unordered_map<NodeID, NodeType, SafeHash> AnswerDecoder::requestNodes(const Message& message) {
    if (message.type() != Answering) { throw DynamicException("Invalid message type."); }
    if (message.command() != RequestNodes) { throw DynamicException("Invalid command."); }
    return nodesFromMessage(message.message());
  }

  std::vector<TxValidator> AnswerDecoder::requestValidatorTxs(
    const Message& message, const uint64_t& requiredChainId
  ) {
    if (message.type() != Answering) { throw DynamicException("Invalid message type."); }
    if (message.command() != RequestValidatorTxs) { throw DynamicException("Invalid command."); }
    return txsFromMessage<TxValidator>(message.message(), requiredChainId);
  }

  std::vector<TxBlock> AnswerDecoder::requestTxs(
    const Message& message, const uint64_t& requiredChainId
  ) {
    if (message.type() != Answering) { throw DynamicException("Invalid message type."); }
    if (message.command() != RequestTxs) { throw DynamicException("Invalid command."); }
    return txsFromMessage<TxBlock>(message.message(), requiredChainId);
  }

  std::vector<FinalizedBlock> AnswerDecoder::requestBlock(
    const Message& message, const uint64_t& requiredChainId
  ) {
    if (message.type() != Answering) { throw DynamicException("Invalid message type."); }
    if (message.command() != RequestBlock) { throw DynamicException("Invalid command."); }
    return blocksFromMessage(message.message(), requiredChainId);
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

  Message BroadcastEncoder::broadcastBlock(const std::shared_ptr<const FinalizedBlock>& block) {
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
    if (message.type() != Broadcasting) { throw DynamicException("Invalid message type."); }
    if (message.id().toUint64() != FNVHash()(message.message())) { throw DynamicException("Invalid message id."); }
    if (message.command() != BroadcastValidatorTx) { throw DynamicException("Invalid command."); }
    return TxValidator(message.message(), requiredChainId);
  }

  TxBlock BroadcastDecoder::broadcastTx(const P2P::Message &message, const uint64_t &requiredChainId) {
    if (message.type() != Broadcasting) { throw DynamicException("Invalid message type."); }
    if (message.id().toUint64() != FNVHash()(message.message())) { throw DynamicException("Invalid message id."); }
    if (message.command() != BroadcastTx) { throw DynamicException("Invalid command."); }
    return TxBlock(message.message(), requiredChainId);
  }

  FinalizedBlock BroadcastDecoder::broadcastBlock(const P2P::Message &message, const uint64_t &requiredChainId) {
    if (message.type() != Broadcasting) { throw DynamicException("Invalid message type."); }
    if (message.id().toUint64() != FNVHash()(message.message())) { throw DynamicException("Invalid message id. "); }
    if (message.command() != BroadcastBlock) { throw DynamicException("Invalid command."); }
    FinalizedBlock block = FinalizedBlock::fromBytes(message.message(), requiredChainId);
    return block;
  }

  Message NotificationEncoder::notifyInfo(
    const std::shared_ptr<const FinalizedBlock>& latestBlock,
    const std::unordered_map<NodeID, NodeType, SafeHash>& nodes,
    const Options& options)
  {
    Bytes message = getRequestTypePrefix(Notifying);
    Utils::appendBytes(message, Utils::randBytes(8));
    Utils::appendBytes(message, getCommandPrefix(NotifyInfo));
    nodeInfoToMessage(message, latestBlock, nodes, options);
    return Message(std::move(message));
  }

  NodeInfo NotificationDecoder::notifyInfo(const Message& message) {
    if (message.type() != Notifying) { throw DynamicException("Invalid message type."); }
    if (message.command() != NotifyInfo) { throw DynamicException("Invalid command."); }
    return nodeInfoFromMessage(message.message());
  }

}

