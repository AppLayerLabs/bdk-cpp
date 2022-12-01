#include "P2PEncoding.h"
#include "P2PManager.h"
#include "../core/chainHead.h"

CommandType getCommandType(const std::string_view& message) {
  if (message.size() != 2) { throw std::runtime_error("Invalid messagetype size."); }
  uint16_t commandType = Utils::bytesToUint16(message);
  if (commandType > commandPrefixes.size()) { throw std::runtime_error("Invalid command type."); }
  return static_cast<CommandType>(commandType);
}

std::string getCommandPrefix(const CommandType& commType) {
  return commandPrefixes[commType];
}

P2PMessage P2PRequestEncoder::info(const std::shared_ptr<const ChainHead> chainHead, const uint64_t &nNodes) {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::Info);
  message += Utils::uint64ToBytes(1); // Version
  message += Utils::uint64ToBytes(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()); // Timestamp
  message += Utils::uint64ToBytes(chainHead->latest()->nHeight()); // Latest block number
  message += chainHead->latest()->getBlockHash().get(); // Latest block hash
  message += Utils::uint64ToBytes(nNodes); // Number of connected nodes
  return P2PMessage(std::move(message));
};

P2PMessage P2PRequestEncoder::sendTransaction(const Tx::Base& transaction) {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::SendTransaction);
  message += transaction.rlpSerialize(true);
  return P2PMessage(std::move(message));
}

P2PMessage P2PRequestEncoder::sendBulkTransactions(const std::vector<Tx::Base>& transactions) {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::SendBulkTransactions);
  message += Utils::uint64ToBytes(transactions.size());
  for (const auto& transaction : transactions) {
    auto txRLP = transaction.rlpSerialize(true);
    message += Utils::uint64ToBytes(txRLP.size());
    message += txRLP;
  }
  return P2PMessage(std::move(message));
}

P2PMessage P2PRequestEncoder::requestBlockByNumber(const uint64_t& blockNumber) {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::RequestBlockByNumber);
  message += Utils::uint64ToBytes(blockNumber);
  return P2PMessage(std::move(message));
}

P2PMessage P2PRequestEncoder::requestBlockByHash(const Hash& blockHash) {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::RequestBlockByHash);
  message += blockHash.get();
  return P2PMessage(std::move(message));
};

P2PMessage P2PRequestEncoder::requestBlockRange(const uint64_t& startBlockNumber, const uint64_t& endBlockNumber) { 
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::RequestBlockRange);
  message += Utils::uint64ToBytes(startBlockNumber);
  message += Utils::uint64ToBytes(endBlockNumber);
  return P2PMessage(std::move(message));
}
P2PMessage P2PRequestEncoder::newBestBlock(const Block& block) {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::NewBestBlock);
  message += block.serializeToBytes(false);
  return P2PMessage(std::move(message));
}

P2PMessage P2PRequestEncoder::sendValidatorTransaction(const Tx::Base& transaction) {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::SendValidatorTransaction);
  message += transaction.rlpSerialize(true);
  return P2PMessage(std::move(message));
};

P2PMessage P2PRequestEncoder::sendBulkValidatorTransactions(const std::vector<Tx::Base>& transactions) {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::SendBulkValidatorTransactions);
  message += Utils::uint64ToBytes(transactions.size());
  for (const auto& transaction : transactions) {
    auto txRLP = transaction.rlpSerialize(true);
    message += Utils::uint64ToBytes(txRLP.size());
    message += txRLP;
  }
  return P2PMessage(std::move(message));
}

P2PMessage P2PRequestEncoder::requestValidatorTransactions() {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::RequestValidatorTransactions);
  return P2PMessage(std::move(message));
}

P2PMessage P2PRequestEncoder::getConnectedNodes() {
  std::string message = Utils::randomBytes(8);
  message += getCommandPrefix(CommandType::GetConnectedNodes);
  return P2PMessage(std::move(message));
}

ConnectionInfo P2PRequestDecoder::info(const P2PMessage& message) {
  // Version + Epoch + nHeight + nBestHash + nNodes
  ConnectionInfo ret;
  ret.version = Utils::bytesToUint64(message.message().substr(0,8));
  ret.timestamp = Utils::bytesToUint64(message.message().substr(8,8));
  ret.clockDiff = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - ret.timestamp;
  ret.latestBlockHeight = Utils::bytesToUint64(message.message().substr(16,8));
  ret.latestBlockHash = Hash(message.message().substr(24,32));  
  ret.nNodes = Utils::bytesToUint64(message.message().substr(56,8));
  ret.latestChecked = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  return ret;
}

P2PMessage P2PAnswerEncoder::info(const std::shared_ptr<const ChainHead> chainHead, const uint64_t &nNodes, const std::string& id) {
  std::string message = id;
  message += getCommandPrefix(CommandType::Info);
  message += Utils::uint64ToBytes(1); // Version
  message += Utils::uint64ToBytes(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count()); // Timestamp
  message += Utils::uint64ToBytes(chainHead->latest()->nHeight()); // Latest block number
  message += chainHead->latest()->getBlockHash().get(); // Latest block hash
  message += Utils::uint64ToBytes(nNodes); // Number of connected nodes
  return P2PMessage(std::move(message));
}

ConnectionInfo P2PAnswerDecoder::info(const P2PMessage& message) {
  ConnectionInfo ret;
  ret.version = Utils::bytesToUint64(message.message().substr(10,8));
  ret.timestamp = Utils::bytesToUint64(message.message().substr(18,8));
  ret.clockDiff = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - ret.timestamp;
  ret.latestBlockHeight = Utils::bytesToUint64(message.message().substr(26,8));
  ret.latestBlockHash = Hash(message.message().substr(34,32));
  ret.nNodes = Utils::bytesToUint64(message.message().substr(66,8));
  ret.latestChecked = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  return ret;
}