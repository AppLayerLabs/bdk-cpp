#ifndef P2PMANAGERDISCOVERY_H
#define P2PMANAGERDISCOVERY_H

#include <iostream>
#include <shared_mutex>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../../utils/utils.h"
#include "../../utils/safehash.h"

#include "p2pmanagerbase.h"
#include "p2pbase.h"
#include "p2pclient.h"
#include "p2pencoding.h"
#include "p2pserver.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace P2P {
  class ManagerDiscovery : public ManagerBase {
    protected:
      // Handlers for client and server requests.
      // Handle message (called from sessions) is public.
      void handleRequest(std::shared_ptr<BaseSession>& session, const Message& message) override;
      void handleAnswer(std::shared_ptr<BaseSession>& session, const Message& message) override;

    private:
      // Handlers for command requests
      void handlePingRequest(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleRequestNodesRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      // Handlers for command answers
      void handlePingAnswer(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleRequestNodesAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

    public:
      ManagerDiscovery(const boost::asio::ip::address& hostIp, unsigned short hostPort) : ManagerBase(hostIp, hostPort, NodeType::DISCOVERY_NODE, 200) {};
      
      void handleMessage(std::shared_ptr<BaseSession> session, const Message message) override;

  };
};

#endif