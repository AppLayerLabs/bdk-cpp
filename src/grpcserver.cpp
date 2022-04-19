#include "httpserver.h"
#include "validation.h"
#include "grpcserver.h"

Status VMServiceImplementation::Initialize(ServerContext* context, const vm::InitializeRequest* request,
                  vm::InitializeResponse* reply) {
    Utils::logToFile("Initialized Called");
    std::string jsonRequest;
    google::protobuf::util::JsonOptions options;
    google::protobuf::util::MessageToJsonString(*request, &jsonRequest, options);
    Utils::logToFile(jsonRequest);
    // Open database with the nodeID as folder name.
    json req = json::parse(jsonRequest);
    std::string nodeID = req["nodeId"];
    validation = std::make_shared<Validation>(nodeID);
    initialized = true;
    validation->initialize();


    auto bestBlock = validation->initialize();
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

    commClient = std::make_shared<VMCommClient>(grpc::CreateChannel(request->server_addr(), grpc::InsecureChannelCredentials()));
    try {
      boost::thread p(&startServer, shared_from_this());
      p.detach();
    } catch (std::exception &e) {
      Utils::logToFile(std::string("HTTP: ") + e.what());
    }

    Utils::logToFile("server started");
    return Status::OK;
  }