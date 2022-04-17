#include "grpcserver.h"

Status VMServiceImplementation::Initialize(ServerContext* context, const vm::InitializeRequest* request,
                  vm::InitializeResponse* reply) {
    Utils::logToFile("Initialized Called");
    initialized = true;
    std::string jsonRequest;
    google::protobuf::util::JsonOptions options;
    google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
    Utils::logToFile(jsonRequest);
    // Open database with the nodeID as folder name.
    json req = json::parse(jsonRequest);
    dbName = req["nodeId"];
    blocksDb.setAndOpenDB(dbName + "-blocks");
    accountsDb.setAndOpenDB(dbName + "-balances");
    confirmedTxs.setAndOpenDB(dbName + "-txs");
    nonceDb.setAndOpenDB(dbName + "-nonces");
    txToBlock.setAndOpenDB(dbName + "-txToBlocks");
    if (blocksDb.isEmpty()) {
      blocksDb.putKeyValue(genesis.blockHash(), genesis.serializeToString());
      blocksDb.putKeyValue(boost::lexical_cast<std::string>(genesis.nHeight()), genesis.serializeToString());
      blocksDb.putKeyValue("latest", genesis.serializeToString());
    }
    if(accountsDb.isEmpty()) {
      accountsDb.putKeyValue("0x9Ef72cF1180C279589ACb06b3bA14700A022e220", "10000000000000000000000");
    }
    Block bestBlock(blocksDb.getKeyValue("latest"));
    Utils::logToFile(blocksDb.getKeyValue("latest"));

    auto hash = bestBlock.blockHash();
    Utils::logToFile(std::string("size: ") + boost::lexical_cast<std::string>(hash.size()));
    Utils::logToFile(bestBlock.blockHash());
    reply->set_last_accepted_id(Utils::hashToBytes(bestBlock.blockHash()));
    reply->set_last_accepted_parent_id(Utils::hashToBytes(bestBlock.prevBlockHash()));
    reply->set_status(1);
    reply->set_height(1);
    // bytes -> last block bytes.
    reply->set_bytes(bestBlock.serializeToString()); 
    reply->set_timestamp(Utils::secondsToGoTimeStamp(boost::lexical_cast<uint64_t>(bestBlock.timestamp())));
    std::string jsonAnswer;
    google::protobuf::util::MessageToJsonString(*reply, &jsonAnswer, options);
    Utils::logToFile(std::string("jsonAnswer:" + jsonAnswer));

    commClient.reset(new VMCommClient(grpc::CreateChannel(request->server_addr(), grpc::InsecureChannelCredentials())));
    try {
      // boost::thread p(&startServer);
      // p.detach();
    } catch (std::exception &e) {
      Utils::logToFile(std::string("HTTP: ") + e.what());
    }

    Utils::logToFile("server started");
    return Status::OK;
  }