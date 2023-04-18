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
  /// Manager focused exclusively at Discovery nodes.
  class ManagerDiscovery : public ManagerBase {
    protected:
      /**
       * Handle a request from a client.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleRequest(std::shared_ptr<BaseSession>& session, const Message& message) override;

      /**
       * Handle an answer from a server.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleAnswer(std::shared_ptr<BaseSession>& session, const Message& message) override;

    private:
      /**
       * Handle a `Ping` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handlePingRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `RequestNodes` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleRequestNodesRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `Ping` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handlePingAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `RequestNodes` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleRequestNodesAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

    public:
      /**
       * Constructor.
       * @param hostIp The manager's host IP/address.
       * @param options Pointer to the options singleton.
       */
      ManagerDiscovery(
        const boost::asio::ip::address& hostIp, const std::unique_ptr<Options>& options
      ) : ManagerBase(hostIp, NodeType::DISCOVERY_NODE, 200, options) {};

      /// Destructor. Automatically stops the manager.
      ~ManagerDiscovery() { this->stop(); }

      /**
       * Handle a message from a session. Entry point for all the other handlers.
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      void handleMessage(std::shared_ptr<BaseSession> session, const Message message) override;
  };
};

#endif  // P2PMANAGERDISCOVERY_H
