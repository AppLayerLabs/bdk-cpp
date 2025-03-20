#ifndef BTVSERVER_MANAGER_H
#define BTVSERVER_MANAGER_H


#include "websocketserver.h"
#include "../../../contract/templates/btvcommon.h"
#include "websocketsession.h"
#include "httpclient.h"
#include <boost/unordered/concurrent_flat_map.hpp>

namespace BTVServer {
  class Manager {
    private:
      /**
        * World class
        * - 1024x1024 area => 64x64 chunks
        * - Each chunk is 16x64x16
        * - chunk coords in range [-32..31]
        */
      BTVUtils::World world_;
      net::io_context ioc_;
      WebsocketServer server_;
      HTTPSemiSyncClient httpClient_;
      Address btvContractAddress_ = Address(Hex::toBytes("0x30C37F6B1d6321C4398238525046c604C7b26150"));
      std::shared_mutex worldMutex_;
      boost::concurrent_flat_map<uint64_t, std::weak_ptr<WebsocketSession>> players_;
      uint64_t lastProcessedBlock = 0;


    public:
      Manager();
      ~Manager();

      void handleHTTPResponse(const std::string& reqBody);
      void registerPlayer(const uint64_t& playerId, std::weak_ptr<WebsocketSession> session);
      void removePlayer(const uint64_t& playerId);
      void handlePlayerRequest(std::weak_ptr<WebsocketSession> session, const std::string& msg);
      void start();
      void loadWorld();
      void broadcastTooAllPlayers(const json& msg);
      json buildGetChunkRequest(const int32_t& x, const int32_t& y, const uint64_t& id = 1);
  };


}




#endif // BTVSERVER_MANAGER_H