#include "methods.h"

#include "blocktag.h"
#include "variadicparser.h"
#include "../../../core/storage.h"
#include "../../../core/state.h"

#include <ranges>

static inline constexpr std::string_view FIXED_BASE_FEE_PER_GAS = "0x9502f900"; // Fixed to 2.5 GWei

namespace jsonrpc {

static std::optional<uint64_t> getBlockNumber(const Storage& storage, const Hash& hash) {
  const auto block = storage.getBlock(hash);

  if (block)
    return block->getNHeight();
  
  return std::nullopt;
}

template<typename T, std::ranges::input_range R>
  requires std::convertible_to<std::ranges::range_value_t<R>, T>
static std::vector<T> makeVector(R&& range) {
  std::vector<T> res(std::ranges::size(range));
  std::ranges::copy(std::forward<R>(range), res.begin());
  return res;
}

static inline void forbidParams(const json& request) {
  if (request.contains("params") && !request["params"].empty())
    throw DynamicException("\"params\" are not required for method");
}

static json getBlockJson(const FinalizedBlock *block, bool includeTransactions) {
  json ret;
  if (block == nullptr) { ret = json::value_t::null; return ret; }
  ret["hash"] = block->getHash().hex(true);
  ret["parentHash"] = block->getPrevBlockHash().hex(true);
  ret["sha3Uncles"] = Hash().hex(true); // Uncles do not exist.
  ret["miner"] = Secp256k1::toAddress(block->getValidatorPubKey()).hex(true);
  ret["stateRoot"] = Hash().hex(true); // No State root.
  ret["transactionsRoot"] = block->getTxMerkleRoot().hex(true);
  ret["receiptsRoot"] = Hash().hex(true); // No receiptsRoot.
  ret["logsBloom"] = Hash().hex(true); // No logsBloom.
  ret["difficulty"] = "0x1";
  ret["number"] = Hex::fromBytes(Utils::uintToBytes(block->getNHeight()),true).forRPC();
  ret["gasLimit"] = Hex::fromBytes(Utils::uintToBytes(std::numeric_limits<uint64_t>::max()),true).forRPC();
  ret["gasUsed"] = Hex::fromBytes(Utils::uintToBytes(uint64_t(1000000000)),true).forRPC(); // Arbitrary number
  ret["timestamp"] = Hex::fromBytes(Utils::uintToBytes(block->getTimestamp()),true).forRPC();
  ret["extraData"] = "0x0000000000000000000000000000000000000000000000000000000000000000";
  ret["mixHash"] = Hash().hex(true); // No mixHash.
  ret["nonce"] = "0x0000000000000000";
  ret["totalDifficulty"] = "0x1";
  ret["baseFeePerGas"] = FIXED_BASE_FEE_PER_GAS;
  ret["withdrawRoot"] = Hash().hex(true); // No withdrawRoot.
  // TODO: to get a block you have to serialize it entirely, this can be expensive.
  ret["size"] = Hex::fromBytes(Utils::uintToBytes(block->serializeBlock().size()),true).forRPC();
  ret["transactions"] = json::array();
  for (const auto& tx : block->getTxs()) {
    if (!includeTransactions) { // Only include the transaction hashes.
      ret["transactions"].push_back(tx.hash().hex(true));
    } else { // Include the transactions as a whole.
      json txJson = json::object();
      txJson["blockHash"] = block->getHash().hex(true);
      txJson["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(block->getNHeight()),true).forRPC();
      txJson["type"] = "0x2"; // Legacy Transactions ONLY.
      txJson["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()),true).forRPC();
      txJson["hash"] = tx.hash().hex(true);
      txJson["to"] = tx.getTo().hex(true);
      txJson["from"] = tx.getFrom().hex(true);
      txJson["gas"] = Hex::fromBytes(Utils::uintToBytes(tx.getGasLimit()),true).forRPC();
      txJson["value"] = Hex::fromBytes(Utils::uintToBytes(tx.getValue()),true).forRPC();
      txJson["input"] = Hex::fromBytes(tx.getData(),true).forRPC();
      txJson["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC();
      txJson["chainId"] = Hex::fromBytes(Utils::uintToBytes(tx.getChainId()),true).forRPC();
      txJson["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()),true).forRPC();
      txJson["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()),true).forRPC();
      txJson["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()),true).forRPC();
      ret["transactions"].emplace_back(std::move(txJson));
    }
  }
  ret["withdrawls"] = json::array();
  ret["uncles"] = json::array();
  return ret;
}

static std::pair<Bytes, evmc_message> parseEvmcMessage(const json& request, const Storage& storage, bool recipientRequired) {
  std::pair<Bytes, evmc_message> res{};

  Bytes& buffer = res.first;
  evmc_message& msg = res.second;

  const auto [txJson, optionalBlockNumber] = parseAllParams<json, std::optional<BlockTagOrNumber>>(request);

  if (optionalBlockNumber.has_value() && !optionalBlockNumber->isLatest(storage))
    throw Error(-32601, "Only latest block is supported");

  msg.sender = parseIfExists<Address>(txJson, "from")
    .transform([] (const Address& addr) { return addr.toEvmcAddress(); })
    .value_or(evmc::address{});

  if (recipientRequired)
    msg.recipient = parse<Address>(txJson.at("to")).toEvmcAddress();
  else
    msg.recipient = parseIfExists<Address>(txJson, "to")
      .transform([] (const Address& addr) { return addr.toEvmcAddress(); })
      .value_or(evmc::address{});

  msg.gas = parseIfExists<uint64_t>(txJson, "gas").value_or(10000000);
  parseIfExists<uint64_t>(txJson, "gasPrice"); // gas price ignored as chain is fixed at 1 GWEI

  msg.value = parseIfExists<uint64_t>(txJson, "value")
    .transform([] (uint64_t val) { return Utils::uint256ToEvmcUint256(uint256_t(val)); })
    .value_or(evmc::uint256be{});

  buffer = parseIfExists<Bytes>(txJson, "data").value_or(Bytes{});

  msg.input_size = buffer.size();
  msg.input_data = buffer.empty() ? nullptr : buffer.data();

  return res;
}

json web3_clientVersion(const json& request, const Options& options) {
  forbidParams(request);
  return options.getWeb3ClientVersion();
}

json web3_sha3(const json& request) {
  const auto [data] = parseAllParams<Bytes>(request);
  return Utils::sha3(data).hex(true);
}

json net_version(const json& request, const Options& options) {
  forbidParams(request);
  return std::to_string(options.getChainID());
}

json net_listening(const json& request) {
  forbidParams(request);
  return true;
}

json eth_protocolVersion(const json& request, const Options& options) {
  forbidParams(request);
  json ret;
  return options.getSDKVersion();
}

json net_peerCount(const json& request, const P2P::ManagerNormal& manager) {
  forbidParams(request);
  return Hex::fromBytes(Utils::uintToBytes(manager.getPeerCount()), true).forRPC();
}

json eth_getBlockByHash(const json& request, const Storage& storage) {
  const auto [blockHash, optionalIncludeTxs] = parseAllParams<Hash, std::optional<bool>>(request);
  const bool includeTxs = optionalIncludeTxs.value_or(false);
  return getBlockJson(storage.getBlock(blockHash).get(), includeTxs);
}

json eth_getBlockByNumber(const json& request, const Storage& storage) {
  const auto [blockNumberOrTag, optionalIncludeTxs] = parseAllParams<BlockTagOrNumber, std::optional<bool>>(request);
  const uint64_t blockNumber = blockNumberOrTag.number(storage);
  const bool includeTxs = optionalIncludeTxs.value_or(false);
  return getBlockJson(storage.getBlock(blockNumber).get(), includeTxs);
}

json eth_getBlockTransactionCountByHash(const json& request, const Storage& storage) {
  const auto [blockHash] = parseAllParams<Hash>(request);
  const auto block = storage.getBlock(blockHash);

  if (block)
    return Hex::fromBytes(Utils::uintToBytes(block->getTxs().size()), true).forRPC();
  else
    return json::value_t::null;
}

json eth_getBlockTransactionCountByNumber(const json& request, const Storage& storage) {
  const auto [blockTagOrNumber] = parseAllParams<BlockTagOrNumber>(request);
  const uint64_t blockNumber = blockTagOrNumber.number(storage);
  const auto block = storage.getBlock(blockNumber);

  if (block)
    return Hex::fromBytes(Utils::uintToBytes(block->getTxs().size()), true).forRPC();
  else
    return json::value_t::null;
}

json eth_chainId(const json& request, const Options& options) {
  forbidParams(request);
  return Hex::fromBytes(Utils::uintToBytes(options.getChainID()), true).forRPC();
}

json eth_syncing(const json& request) {
  forbidParams(request);
  return false;
}

json eth_coinbase(const json& request, const Options& options) {
  forbidParams(request);
  return options.getCoinbase().hex(true);
}

json eth_blockNumber(const json& request, const Storage& storage) {
  forbidParams(request);
  return Hex::fromBytes(Utils::uintToBytes(storage.latest()->getNHeight()), true).forRPC();
}

json eth_call(const json& request, const Storage& storage, State& state) {
  auto [buffer, callInfo] = parseEvmcMessage(request, storage, true);
  callInfo.kind = EVMC_CALL;
  callInfo.flags = 0;
  callInfo.depth = 0;
  return Hex::fromBytes(state.ethCall(callInfo), true);
}

json eth_estimateGas(const json& request, const Storage& storage, State& state) {
  auto [buffer, callInfo] = parseEvmcMessage(request, storage, false);
  callInfo.flags = 0;
  callInfo.depth = 0;

  // TODO: "kind" is uninitialized if recipient is not zeroes
  if (evmc::is_zero(callInfo.recipient))
    callInfo.kind = EVMC_CREATE;

  const auto usedGas = state.estimateGas(callInfo);

  return Hex::fromBytes(Utils::uintToBytes(static_cast<uint64_t>(usedGas)), true).forRPC();
}

json eth_gasPrice(const json& request) {
  forbidParams(request);
  return FIXED_BASE_FEE_PER_GAS;
}

json eth_feeHistory(const json& request, const Storage& storage) {
  json ret;
  auto [blockCount, newestBlock, optionalRewardPercentiles] = parseAllParams<
    uint64_t, BlockTagOrNumber, std::optional<std::vector<float>>>(request);

  uint64_t blockNumber = newestBlock.number(storage);
  const std::vector<float> percentiles = std::move(optionalRewardPercentiles).value_or(std::vector<float>{});

  // no more than 1024 block can be requested
  blockCount = std::min(blockCount, static_cast<uint64_t>(1024));

  ret["baseFeePerGas"] = json::array();
  ret["gasUsedRatio"] = json::array();

  // The feeHistory output includes the next block after the newest too
  std::shared_ptr<const FinalizedBlock> oneAfterLastBlock = storage.getBlock(blockNumber + 1);
  if (oneAfterLastBlock)
    ret["baseFeePerGas"].push_back(FIXED_BASE_FEE_PER_GAS);
  
  uint64_t oldestBlock;
  while (blockCount--) {
    std::shared_ptr<const FinalizedBlock> block = storage.getBlock(blockNumber);

    if (!block)
      break;

    ret["baseFeePerGas"].push_back(FIXED_BASE_FEE_PER_GAS); // TODO: fill with proper value once available
    ret["gasUsedRatio"].push_back(1.0f); // TODO: calculate as gasUsed / gasLimit

    oldestBlock = blockNumber--;
  }

  if (ret["baseFeePerGas"].empty())
    throw Error::executionError("Requested block not found");

  ret["oldestBlock"] = Hex::fromBytes(Utils::uintToBytes(oldestBlock), true).forRPC();

  return ret;
}

json eth_getLogs(const json& request, const Storage& storage, const State& state) {
  const auto [logsObj] = parseAllParams<json>(request);
  const auto getBlockByHash = [&storage] (const Hash& hash) { return getBlockNumber(storage, hash); };

  const std::optional<Hash> blockHash = parseIfExists<Hash>(logsObj, "blockHash");

  const uint64_t fromBlock = parseIfExists<BlockTagOrNumber>(logsObj, "fromBlock")
    .transform([&] (const BlockTagOrNumber& b) -> uint64_t { return b.number(storage); })
    .or_else([&] { return blockHash.and_then(getBlockByHash); })
    .value_or(ContractGlobals::getBlockHeight());

  const uint64_t toBlock = parseIfExists<BlockTagOrNumber>(logsObj, "toBlock")
    .transform([&] (const BlockTagOrNumber& b) -> uint64_t { return b.number(storage); })
    .or_else([&] { return blockHash.and_then(getBlockByHash); })
    .value_or(ContractGlobals::getBlockHeight());

  const std::optional<Address> address = parseIfExists<Address>(logsObj, "address");

  const std::vector<Hash> topics = parseArrayIfExists<Hash>(logsObj, "topics")
    .transform([] (auto&& arr) { return makeVector<Hash>(std::forward<decltype(arr)>(arr)); })
    .value_or(std::vector<Hash>{});

  json result = json::array();

  for (const auto& event : state.getEvents(fromBlock, toBlock, address.value_or(Address{}), topics))
    result.push_back(event.serializeForRPC());

  return result;
}

json eth_getBalance(const json& request, const Storage& storage, const State& state) {
  const auto [address, block] = parseAllParams<Address, BlockTagOrNumber>(request);

  if (!block.isLatest(storage))
    throw DynamicException("Only the latest block is supported");

  return Hex::fromBytes(Utils::uintToBytes(state.getNativeBalance(address)), true).forRPC();
}

json eth_getTransactionCount(const json& request, const Storage& storage, const State& state) {
  const auto [address, block] = parseAllParams<Address, BlockTagOrNumber>(request);

  if (!block.isLatest(storage))
    throw DynamicException("Only the latest block is supported");

  return Hex::fromBytes(Utils::uintToBytes(state.getNativeNonce(address)), true).forRPC();
}

json eth_getCode(const json& request, const Storage& storage, const State& state) {
  const auto [address, block] = parseAllParams<Address, BlockTagOrNumber>(request);

  if (!block.isLatest(storage))
    throw DynamicException("Only the latest block is supported");

  return Hex::fromBytes(state.getContractCode(address), true).forRPC();
}

json eth_sendRawTransaction(const json& request, uint64_t chainId, State& state, P2P::ManagerNormal& p2p) {
  const auto [bytes] = parseAllParams<Bytes>(request);
  const TxBlock tx(bytes, chainId);

  json ret;
  const auto& txHash = tx.hash();
  auto txStatus = state.addTx(TxBlock(tx));
  if (isTxStatusValid(txStatus)) {
    ret = txHash.hex(true);
    // TODO: Make this use threadpool instead of blocking
    // TODO: Make tx broadcasting better, the current solution is **not good**.
    p2p.getBroadcaster().broadcastTxBlock(tx);
  } else {
    std::string message = "Unknown";
    switch (txStatus) {
      case TxStatus::InvalidNonce:
        message = "Invalid nonce";
        break;
      case TxStatus::InvalidBalance:
        message = "Invalid balance";
        break;
    }
    throw Error(-32000, std::move(message));
  }
  return ret;
}

json eth_getTransactionByHash(const json& request, const Storage& storage, const State& state) {
  const auto [txHash] = parseAllParams<Hash>(request);

  json ret;
  auto txOnMempool = state.getTxFromMempool(txHash);
  if (txOnMempool != nullptr) {
    ret["blockHash"] = json::value_t::null;
    ret["blockIndex"] = json::value_t::null;
    ret["from"] = txOnMempool->getFrom().hex(true);
    ret["gas"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getGasLimit()), true).forRPC();
    ret["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getMaxFeePerGas()), true).forRPC();
    ret["hash"] = txOnMempool->hash().hex(true);
    ret["input"] = Hex::fromBytes(txOnMempool->getData(), true).forRPC();
    ret["nonce"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getNonce()), true).forRPC();
    ret["to"] = txOnMempool->getTo().hex(true);
    ret["transactionIndex"] = json::value_t::null;
    ret["value"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getValue()), true).forRPC();
    ret["v"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getV()), true).forRPC();
    ret["r"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getR()), true).forRPC();
    ret["s"] = Hex::fromBytes(Utils::uintToBytes(txOnMempool->getS()), true).forRPC();
    return ret;
  }

  auto txOnChain = storage.getTx(txHash);
  const auto& [tx, blockHash, blockIndex, blockHeight] = txOnChain;
  if (tx != nullptr) {
    ret["blockHash"] = blockHash.hex(true);
    ret["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(blockHeight), true).forRPC();
    ret["from"] = tx->getFrom().hex(true);
    ret["gas"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
    ret["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()), true).forRPC();
    ret["hash"] = tx->hash().hex(true);
    ret["input"] = Hex::fromBytes(tx->getData(), true).forRPC();
    ret["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx->getNonce()), true).forRPC();
    ret["to"] = tx->getTo().hex(true);
    ret["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(blockIndex), true).forRPC();
    ret["value"] = Hex::fromBytes(Utils::uintToBytes(tx->getValue()), true).forRPC();
    ret["v"] = Hex::fromBytes(Utils::uintToBytes(tx->getV()), true).forRPC();
    ret["r"] = Hex::fromBytes(Utils::uintToBytes(tx->getR()), true).forRPC();
    ret["s"] = Hex::fromBytes(Utils::uintToBytes(tx->getS()), true).forRPC();
    return ret;
  }

  return json::value_t::null;
}

json eth_getTransactionByBlockHashAndIndex(const json& request, const Storage& storage) {
  const auto [blockHash, blockIndex] = parseAllParams<Hash, uint64_t>(request);
  auto txInfo = storage.getTxByBlockHashAndIndex(blockHash, blockIndex);
  const auto& [tx, txBlockHash, txBlockIndex, txBlockHeight] = txInfo;

  if (tx != nullptr) {
    json ret;
    ret["blockHash"] = txBlockHash.hex(true);
    ret["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(txBlockHeight), true).forRPC();
    ret["from"] = tx->getFrom().hex(true);
    ret["gas"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
    ret["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()), true).forRPC();
    ret["hash"] = tx->hash().hex(true);
    ret["input"] = Hex::fromBytes(tx->getData(), true).forRPC();
    ret["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx->getNonce()), true).forRPC();
    ret["to"] = tx->getTo().hex(true);
    ret["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txBlockIndex), true).forRPC();
    ret["value"] = Hex::fromBytes(Utils::uintToBytes(tx->getValue()), true).forRPC();
    ret["v"] = Hex::fromBytes(Utils::uintToBytes(tx->getV()), true).forRPC();
    ret["r"] = Hex::fromBytes(Utils::uintToBytes(tx->getR()), true).forRPC();
    ret["s"] = Hex::fromBytes(Utils::uintToBytes(tx->getS()), true).forRPC();
    return ret;
  }

  return json::value_t::null;
}

json eth_getTransactionByBlockNumberAndIndex(const json& request, const Storage& storage) {
  const auto [blockNumber, blockIndex] = parseAllParams<uint64_t, uint64_t>(request);
  auto txInfo = storage.getTxByBlockNumberAndIndex(blockNumber, blockIndex);
  const auto& [tx, txBlockHash, txBlockIndex, txBlockHeight] = txInfo;

  if (tx != nullptr) {
    json ret;
    ret["blockHash"] = txBlockHash.hex(true);
    ret["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(txBlockHeight), true).forRPC();
    ret["from"] = tx->getFrom().hex(true);
    ret["gas"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
    ret["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()), true).forRPC();
    ret["hash"] = tx->hash().hex(true);
    ret["input"] = Hex::fromBytes(tx->getData(), true).forRPC();
    ret["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx->getNonce()), true).forRPC();
    ret["to"] = tx->getTo().hex(true);
    ret["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txBlockIndex), true).forRPC();
    ret["value"] = Hex::fromBytes(Utils::uintToBytes(tx->getValue()), true).forRPC();
    ret["v"] = Hex::fromBytes(Utils::uintToBytes(tx->getV()), true).forRPC();
    ret["r"] = Hex::fromBytes(Utils::uintToBytes(tx->getR()), true).forRPC();
    ret["s"] = Hex::fromBytes(Utils::uintToBytes(tx->getS()), true).forRPC();
    return ret;
  }
  return json::value_t::null;
}

json eth_getTransactionReceipt(const json& request, const Storage& storage, const State& state) {
  const auto [txHash] = parseAllParams<Hash>(request);
  auto txInfo = storage.getTx(txHash);
  const auto& [tx, blockHash, txIndex, blockHeight] = txInfo;

  if (tx != nullptr) {
    json ret;
    std::optional<const TxAdditionalData> addTxData = storage.getTxAdditionalData(tx->hash());
    const uint256_t gasUsed = addTxData.transform([] (const auto& txData) -> uint256_t { return txData.gasUsed; }).value_or(tx->getGasLimit());
    const Address contractAddress = addTxData.transform([] (const auto& txData) { return txData.contractAddress; }).value_or(Address());
    const bool status = addTxData.transform([] (const auto& txData) { return txData.succeeded; }).value_or(true);

    ret["transactionHash"] = tx->hash().hex(true);
    ret["transactionIndex"] = Hex::fromBytes(Utils::uintToBytes(txIndex), true).forRPC();
    ret["blockHash"] = blockHash.hex(true);
    ret["blockNumber"] = Hex::fromBytes(Utils::uintToBytes(blockHeight), true).forRPC();
    ret["from"] = tx->getFrom().hex(true);
    ret["to"] = tx->getTo().hex(true);
    ret["cumulativeGasUsed"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
    ret["effectiveGasUsed"] = Hex::fromBytes(Utils::uintToBytes(tx->getGasLimit()), true).forRPC();
    ret["effectiveGasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx->getMaxFeePerGas()),true).forRPC();
    ret["gasUsed"] =  Hex::fromBytes(Utils::uintToBytes(gasUsed), true).forRPC();
    ret["contractAddress"] = bool(contractAddress) ? json(contractAddress.hex(true)) : json(json::value_t::null);
    ret["logs"] = json::array();
    ret["logsBloom"] = Hash().hex(true);
    ret["type"] = "0x00";
    ret["root"] = Hash().hex(true);
    ret["status"] = status ? "0x1" : "0x0";
    for (const Event& e : state.getEvents(txHash, blockHeight, txIndex)) {
      ret["logs"].push_back(e.serializeForRPC());
    }
    return ret;
  }
  return json::value_t::null;
}

json eth_getUncleByBlockHashAndIndex() {
  return json::value_t::null;
}

json txpool_content(const json& request, const Storage& storage, const State& state) {
  forbidParams(request);
  json result;
  result["queued"] = json::array();
  json& pending = result["pending"];

  pending = json::array();

  for (const auto& hashTxPair : state.getPendingTxs()) {
    const auto& tx = hashTxPair.second;
    json& txJson = pending[tx.getFrom().hex(true)][tx.getNonce().str()];

    txJson["blockHash"] = json::value_t::null;
    txJson["blockNumber"] = json::value_t::null;
    txJson["from"] = tx.getFrom().hex(true);
    txJson["to"] = tx.getTo().hex(true);
    txJson["gasUsed"] = json::value_t::null;
    txJson["gasPrice"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC();
    txJson["getMaxFeePerGas"] = Hex::fromBytes(Utils::uintToBytes(tx.getMaxFeePerGas()),true).forRPC();
    txJson["chainId"] = Hex::fromBytes(Utils::uintToBytes(tx.getChainId()),true).forRPC(); 
    txJson["input"] = Hex::fromBytes(tx.getData(), true).forRPC();
    txJson["nonce"] = Hex::fromBytes(Utils::uintToBytes(tx.getNonce()), true).forRPC();
    txJson["transactionIndex"] = json::value_t::null;
    txJson["type"] = "0x0"; // Legacy Transactions ONLY. TODO: change this to 0x2 when we support EIP-1559
    txJson["v"] = Hex::fromBytes(Utils::uintToBytes(tx.getV()), true).forRPC();
    txJson["r"] = Hex::fromBytes(Utils::uintToBytes(tx.getR()), true).forRPC();
    txJson["s"] = Hex::fromBytes(Utils::uintToBytes(tx.getS()), true).forRPC();
  }

  return result;
}

json debug_traceBlockByNumber(const json& request, const Storage& storage) {
  json res = json::array();
  auto [blockNumber, traceJson] = parseAllParams<uint64_t, json>(request);

  if (!traceJson.contains("tracer"))
    throw Error(-32000, "trace type missing");

  if (traceJson["tracer"] != "callTracer")
    throw Error(-32000, std::string("trace mode \"") + traceJson["tracer"].get<std::string>() + "\" not supported");

  const auto block = storage.getBlock(blockNumber);

  if (!block)
    throw Error(-32000, std::string("block ") + std::to_string(blockNumber) + " not found");

  for (const auto& tx : block->getTxs()) {
    json txTrace;

    auto callTrace = storage.getCallTrace(tx.hash());

    if (!callTrace)
      continue;

    txTrace["txHash"] = tx.hash().hex(true);
    txTrace["result"] = callTrace->toJson();

    res.push_back(std::move(txTrace));
  }

  return res;
}

json debug_traceTransaction(const json& request, const Storage& storage) {
  json res;
  auto [txHash, traceJson] = parseAllParams<Hash, json>(request);

  if (!traceJson.contains("tracer"))
    throw Error(-32000, "trace mode missing");

  if (traceJson["tracer"] != "callTracer")
    throw Error(-32000, std::string("trace mode \"") + traceJson["tracer"].get<std::string>() + "\" not supported");

  std::optional<trace::Call> callTrace = storage.getCallTrace(txHash);

  if (!callTrace)
    return json::value_t::null;

  res = callTrace->toJson();

  return res;
}

} // namespace jsonrpc
