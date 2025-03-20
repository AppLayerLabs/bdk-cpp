#include "manager.h"

#include "bins/network-sim/src/httpclient.h"
#include "contract/abi.h"
#include "net/http/jsonrpc/methods.h"

namespace BTVServer {
  Manager::Manager() : world_(), ioc_(8), server_(*this, ioc_, tcp::endpoint(tcp::v4(), 29345)), httpClient_("149.112.84.202", "8095", ioc_, *this) {}
  Manager::~Manager() {
    this->server_.close();
    this->ioc_.stop();
    Utils::safePrint("Manager destroyed");
    this->httpClient_.close();

  }
  void Manager::handleHTTPResponse(const std::string& reqBody) {
    static auto lastTimeResponded = std::chrono::system_clock::now();
    // ONLY USED TO GET LOGS FROM THE SERVER!
    // {"jsonrpc":"2.0","id":1,"result":[{"address":"0x30c37f6b1d6321c4398238525046c604c7b26150","blockHash":"0x02bb902e386b9b8baf792294f158897f0677a0d114105f886b9dd73ce8cec7c9","blockNumber":"0x0000000000002621","data":"0x0000000000000000000000000000000000000000000000000000000000000026000000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000070000000000000000000000000000000000000000000000000000000000000004","logIndex":"0x0000000000000000","removed":false,"topics":["0x88c1435105c4190f4c8be13e5dbc689ebf4dbec75a17e63a51697ce761b5a1d2"],"transactionHash":"0x84cb3ac1ac98c1f481e3fa861230e100696896a04275d7af850dc90ff9930618","transactionIndex":"0x0000000000000000"}]}
    json response = json::parse(reqBody);
    /*
    void PlayerMoved(const EventParam<uint64_t, false>& playerId, const EventParam<int32_t, false>& x, const EventParam<int32_t, false>& y, const EventParam<int32_t, false>& z) {
      this->emitEvent("PlayerMoved", std::make_tuple(playerId, x, y, z));
    }
    void PlayerLogin(const EventParam<uint64_t, false>& playerId, const EventParam<int32_t, false>& x, const EventParam<int32_t, false>& y, const EventParam<int32_t, false>& z) {
      this->emitEvent("PlayerLogin", std::make_tuple(playerId, x, y, z));
    }
    void PlayerLogout(const EventParam<uint64_t, false>& playerId) {
      this->emitEvent("PlayerLogout", std::make_tuple(playerId));
    }
    void BlockChanged(const EventParam<uint64_t, false>& playerId, const EventParam<int32_t, false>& x, const EventParam<int32_t, false>& y, const EventParam<int32_t, false>& z, const EventParam<uint8_t, false>& blockType, const EventParam<uint64_t, false>& timestamp) {
      this->emitEvent("BlockChanged", std::make_tuple(playerId, x, y, z, blockType, timestamp));
    }
    void ClaimedEnergy(const EventParam<uint64_t, false>& playerId, const EventParam<uint256_t, false>& value) {
      this->emitEvent("ClaimedEnergy", std::make_tuple(playerId, value));
    }
    void PlayerDead(const EventParam<uint64_t, false>& playerId) {
      this->emitEvent("PlayerDead", std::make_tuple(playerId));
    }
    */
    static Hash PlayerMovedTopic = ABI::EventEncoder::encodeSignature<uint64_t, int32_t, int32_t, int32_t>("PlayerMoved");
    static Hash PlayerLoginTopic = ABI::EventEncoder::encodeSignature<uint64_t, int32_t, int32_t, int32_t>("PlayerLogin");
    static Hash PlayerLogoutTopic = ABI::EventEncoder::encodeSignature<uint64_t>("PlayerLogout");
    static Hash BlockChangedTopic = ABI::EventEncoder::encodeSignature<uint64_t, int32_t, int32_t, int32_t, uint8_t, uint64_t>("BlockChanged");
    static Hash ClaimedEnergyTopic = ABI::EventEncoder::encodeSignature<uint64_t, uint256_t>("ClaimedEnergy");
    static Hash PlayerDeadTopic = ABI::EventEncoder::encodeSignature<uint64_t>("PlayerDead");
    json jsonRpcResponse;
    jsonRpcResponse["id"] = response.at("id");
    jsonRpcResponse["jsonrpc"] = "2.0";
    jsonRpcResponse["result"] = json::array();
    try {
      // For every object within the logs, we need to construct its respective JSON object and insert it into the result array
      for (const auto& log : response.at("result")) {
        // We need to replace the lastProcessedBlock based on the latest provided by the logs we requested
        uint64_t blockNumber = Utils::fromBigEndian<uint64_t>(Hex::toBytes(log.at("blockNumber").get<std::string>()));
        if (blockNumber > this->lastProcessedBlock) {
          this->lastProcessedBlock = blockNumber;
        }
        json eventUpdate;
        // Now we just need to separate the logs by the topics previously defined
        // We dont need to double check anything because we are interested in ALL logs
        // and our requested is correctly built to only get logs from the BTV contract
        Hash logTopic = Hash(Hex::toBytes(log.at("topics").at(0).get<std::string>()));
        if (logTopic == PlayerMovedTopic) {
          auto data = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t>(Hex::toBytes(log.at("data").get<std::string>()));
          const auto& [playerId, x, y, z] = data;
          eventUpdate = {
            {"method", "PlayerMoved"},
            {"playerId", playerId},
            {"x", x},
            {"y", y},
            {"z", z}
          };
        }
        if (logTopic == PlayerLoginTopic) {
          auto data = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t>(Hex::toBytes(log.at("data").get<std::string>()));
          const auto& [playerId, x, y, z] = data;
          eventUpdate = {
            {"method", "PlayerLogin"},
            {"playerId", playerId},
            {"x", x},
            {"y", y},
            {"z", z}
          };
        }
        if (logTopic == PlayerLogoutTopic) {
          auto data = ABI::Decoder::decodeData<uint64_t>(Hex::toBytes(log.at("data").get<std::string>()));
          const auto& [playerId] = data;
          eventUpdate = {
            {"method", "PlayerLogout"},
            {"playerId", playerId}
          };
        }
        if (logTopic == BlockChangedTopic) {
          auto data = ABI::Decoder::decodeData<uint64_t, int32_t, int32_t, int32_t, uint8_t, uint64_t>(Hex::toBytes(log.at("data").get<std::string>()));
          const auto& [playerId, x, y, z, blockType, timestamp] = data;
          eventUpdate = {
            {"method", "BlockChanged"},
            {"playerId", playerId},
            {"x", x},
            {"y", y},
            {"z", z},
            {"blockType", blockType},
            {"timestamp", timestamp}
          };
          // We also need to update the world!
          this->worldMutex_.lock();
          auto block = this->world_.getBlock(BTVUtils::WorldBlockPos{x, y, z});
          block->setBlockType(static_cast<BTVUtils::BlockType>(blockType));
          block->setPlacer(playerId);
          block->setModificationTimestamp(timestamp);
        }
        if (logTopic == ClaimedEnergyTopic) {
          auto data = ABI::Decoder::decodeData<uint64_t, uint256_t>(Hex::toBytes(log.at("data").get<std::string>()));
          const auto& [playerId, value] = data;
          eventUpdate = {
            {"method", "ClaimedEnergy"},
            {"playerId", playerId},
            {"value", value.str()} // VALUE IS A STRING BECAUSE JSON CANNOT SUPPORT UINT256_T!!! PAY ATTENTION TO THIS
          };
        }
        if (logTopic == PlayerDeadTopic) {
          auto data = ABI::Decoder::decodeData<uint64_t>(Hex::toBytes(log.at("data").get<std::string>()));
          const auto& [playerId] = data;
          eventUpdate = {
            {"method", "PlayerDead"},
            {"playerId", playerId}
          };
        }
      }
    } catch (std::exception &e) {
      Printer::safePrint("Error while processing response: " + std::string(e.what()) + " with message " + reqBody);
    }
    // Now we need to broadcast the update object to all players!
    this->broadcastTooAllPlayers(jsonRpcResponse);
    // Lmao lets request the logs again!
    // We wait at least 100ms
    std::this_thread::sleep_until(lastTimeResponded + std::chrono::milliseconds(100));
    lastTimeResponded = std::chrono::system_clock::now();
    this->httpClient_.makeHTTPRequest(makeRequestMethod("eth_getLogs", json::array({
      {
        {"address", btvContractAddress_.hex(true)},
        {"fromBlock", Hex::fromBytes(Utils::uintToBytes(this->lastProcessedBlock), true).forRPC()},
        {"toBlock", "latest"}
      }
    })).dump());
  }
  void Manager::registerPlayer(const uint64_t& id, std::weak_ptr<WebsocketSession> session) {
    this->players_.insert({id, session});
  }
  void Manager::removePlayer(const uint64_t& id) {
    this->players_.erase(id);
  }
  void Manager::handlePlayerRequest(std::weak_ptr<WebsocketSession> session, const std::string& msg) {
    // Post this to the io_context
    this->ioc_.post([this, session, msg]() {
      try {
        json j = json::parse(msg);
        if (!j.contains("method")) {
          throw std::runtime_error("Method not found");
        }
        if (!j.at("method").is_string()) {
          throw std::runtime_error("Method is not a string");
        }
        if (j.at("method").get<std::string>() == "getChunks") {
          if (!j.contains("params")) {
            throw std::runtime_error("Params not found");
          }
          if (!j.at("params").is_array()) {
            throw std::runtime_error("Params is not an array");
          }
          json response = {
            {"jsonrpc", "2.0"},
            {"id", j.at("id")},
            {"result", json::array()}
          };
          for (const auto& param : j.at("params")) {
            if (!param.contains("x") || !param.contains("y")) {
              throw std::runtime_error("Param does not contain x or y");
            }
            if (!param.at("x").is_number_integer() || !param.at("y").is_number_integer()) {
              throw std::runtime_error("Param x or y is not an integer");
            }
            int32_t x = param.at("x").get<int32_t>();
            int32_t y = param.at("y").get<int32_t>();
            if (x < -32 || x > 31 || y < -32 || y > 31) {
              throw std::runtime_error("Invalid x or y");
            }
            this->worldMutex_.lock_shared();
            BTVUtils::Chunk chunk = *this->world_.getChunk({x, y});
            json chunkJson = {
              {"x", x},
              {"y", y},
              {"data", Hex::fromBytes(chunk.serialize(), true).get()}
            };
            response["result"].push_back(chunkJson);
          }
          session.lock()->write(response.dump());
        } else {
          throw std::runtime_error("Method not allowed");
        }
      } catch (std::exception &e) {
        Printer::safePrint("Error while processing player request: " + std::string(e.what()) + " with message " + msg + " with size " + std::to_string(msg.size()) + " disconnecting player");
      }
    });
  }
  void Manager::start() {
    // To properly start, we need to start querying the blockchain for the logs from BTV contract
    // For that, we need to initialize the HTTP client.
    Printer::safePrint("Are you COMPLETELY sure that the blockchain is NOT moving?");
    std::string answer;
    std::cin >> answer;
    this->loadWorld();

    this->httpClient_.connect();
    this->server_.setup();
    // Now we need to initialize the thread vector that will be executing the io_context
    std::vector<std::thread> threads;
    threads.reserve(7); // 7 because the main thread will also be running the io_context
    for (int i = 0; i < 7; i++) {
      threads.emplace_back([this]() {
        Printer::safePrint("Running io_context");
        this->ioc_.run();
        Printer::safePrint("io_context has stopped");
      });
    }
    // Before running the io_context in the main thread, we MUST start requesting for the logs! Otherwise it will be a bad time for us
    Printer::safePrint("Making the first request for the logs");
    this->httpClient_.makeHTTPRequest(makeRequestMethod("eth_getLogs", json::array({
      {
        {"address", btvContractAddress_.hex(true)},
        {"fromBlock", Hex::fromBytes(Utils::uintToBytes(this->lastProcessedBlock), true).forRPC()},
        {"toBlock", "latest"}
      }
    })).dump());
    Printer::safePrint("Request sent");
    // Now we need to run the io_context on the main thread
    this->ioc_.run();
    Printer::safePrint("Joining all other threads");
    for (auto& thread : threads) {
      thread.join();
    }
    Printer::safePrint("Manager is successfully shutting down");
  }

  void Manager::loadWorld() {
    Printer::safePrint("Connecting to the blockchain...");
    HTTPSyncClient client("149.112.84.202", "8095");
    client.connect();
    Printer::safePrint("Connected");
    // We need to request ALL the chunks from the blockchain
    // That means a range (x, y) from (-32, -32) to (31, 31)
    auto now = std::chrono::system_clock::now();
/*
    for (int x = -32; x < 32; x++) {
      Printer::safePrint("Requesting chunks for x = " + std::to_string(x) + " total Y: 64");
      json requestArr = json::array();
      for (int y = -32; y < 32; y++) {
        requestArr.push_back(buildGetChunkRequest(x, y, y + 32));
      }
      auto response = client.makeHTTPRequest(requestArr.dump());

      json chunkRequestResponse = json::parse(response);
      // Create a range of -32 to 31 numbers (number -> bool) so we can check if ALL
      // chunks were successfully deserialized
      std::map<int, bool> chunksReceived;
      for (int y = -32; y < 32; y++) {
        chunksReceived[y] = false;
      }
      assert(chunksReceived.size() == 64);
      assert(chunkRequestResponse.is_array());
      assert(chunkRequestResponse.size() == 64);
      for (const auto& chunkResponse : chunkRequestResponse) {
        uint64_t chunkId = chunkResponse["id"].get<uint64_t>();
        *this->world_.getChunk({x, chunkId - 32}) = BTVUtils::Chunk::deserialize(Hex::toBytes(chunkResponse["result"].get<std::string>()));
        chunksReceived.at(chunkId - 32) = true;
      }
      for (const auto& [y, received] : chunksReceived) {
        if (!received) {
          Printer::safePrint("Chunk (" + std::to_string(x) + ", " + std::to_string(y) + ") was not received");
          throw std::runtime_error("Chunk was not received");
        }
      }
    } */
    auto after = std::chrono::system_clock::now();
    Printer::safePrint("Time taken to request all 4096 chunks: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(after - now).count()) + "ms");
    Printer::safePrint("Getting the latest block from the network");
    auto latestBlock = client.makeHTTPRequest(makeRequestMethod("eth_blockNumber", json::array()).dump());
    // Result will have the number value hex encoded
    this->lastProcessedBlock = Utils::fromBigEndian<uint64_t>(Hex::toBytes(json::parse(latestBlock)["result"].get<std::string>()));
    Utils::safePrint("Latest block: " + std::to_string(this->lastProcessedBlock));
    client.close();
  }

  void Manager::broadcastTooAllPlayers(const json &msg) {
    std::string message = msg.dump();
    this->players_.visit_all([message](auto& player) {
      if (auto session = player.second.lock()) {
        Printer::safePrint("Broadcast to: " + std::to_string(player.first));
        session->write(message);
      }
    });
    // Clear up the map from bad objects!
    this->players_.erase_if([](auto& player) {
      return player.second.expired();
    });
  }


  json Manager::buildGetChunkRequest(const int32_t& x, const int32_t& y, const uint64_t& id) {
    Functor getChunksFunctor = ABI::FunctorEncoder::encode<int32_t, int32_t>("getChunk");
    Bytes data;
    Utils::appendBytes(data, UintConv::uint32ToBytes(getChunksFunctor.value));
    Utils::appendBytes(data, ABI::Encoder::encodeData(x, y));

    json req = {
      {"to", btvContractAddress_.hex(true)},
      {"data", Hex::fromBytes(data, true).get()}
    };
    return makeRequestMethod("eth_call",
      json::array( {req}), id
    );
  }
}

