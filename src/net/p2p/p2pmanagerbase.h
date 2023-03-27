#ifndef P2PMANAGERBASE_H
#define P2PMANAGERBASE_H

#include <iostream>
#include <shared_mutex>
#include <atomic>
#include <unordered_set>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../../libs/BS_thread_pool_light.hpp"
#include "../../utils/utils.h"
#include "../../utils/safehash.h"

#include "p2pdiscoveryworker.h"
#include "p2pbase.h"
#include "p2pclient.h"
#include "p2pencoding.h"
#include "p2pserver.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

/**
 * Base Manager for Normal node and Discovery Node.
 * Discovery Node has access to only few functions of P2PEncoding, such as
 * "Ping", "Info" and "RequestNodes".
 */

namespace P2P {
  class DiscoveryWorker;

  class ManagerBase {
    protected:
      const Hash nodeId_; // Unique node ID, randomly generated at P2PManager constructor, 32 bytes in size, byte encoded.
      // Used in the session unordered_map to identify a session to a given node.
      // Do not allow multiple sessions to the same node.
      boost::asio::ip::address hostIp_;
      unsigned short hostPort_;
      const std::shared_ptr<Server> p2pserver_;
      const NodeType nodeType_;
      const unsigned int maxConnections_;
      std::unordered_map<Hash, std::shared_ptr<BaseSession>, SafeHash> sessions_;
      std::unordered_map<RequestID, std::shared_ptr<Request>, SafeHash> requests_;

      std::shared_mutex sessionsMutex; // Mutex for protecting sessions
      std::shared_mutex requestsMutex; // Mutex for protecting requests

      // Sends a message to a given node. returns pointer to the request object, null if doesn't exists.
      std::shared_ptr<Request> sendMessageTo(const Hash &nodeId, const Message &message);

      void answerSession(std::shared_ptr<BaseSession> &session, const Message &message);

      // Handlers for client and server requests.
      // Handle message (called from sessions) is public.
      // Overriden by inherited object
      // TODO: There is a bug with handleRequest that throws std::system_error.
      // I believe that this is related with the std::shared_ptr<BaseSession> getting deleted or
      // the session itself being disconnected.
      virtual void handleRequest(std::shared_ptr<BaseSession> &session, const Message &message) {};

      virtual void handleAnswer(std::shared_ptr<BaseSession> &session, const Message &message) {};

      virtual void handleBroadcast(std::shared_ptr<BaseSession> &session, const Message &message) {};

      // For Discovery thread.
      const std::unique_ptr<DiscoveryWorker> discoveryWorker;

      // Thread pool.
      const std::unique_ptr<BS::thread_pool_light> threadPool;

    public:
      ManagerBase(
        const boost::asio::ip::address& hostIp, unsigned short hostPort,
        NodeType nodeType, unsigned int maxConnections
      );

      ~ManagerBase() { stop(); }

      // Registers a session
      bool registerSession(std::shared_ptr<BaseSession> session);

      // Unregisters a session
      bool unregisterSession(std::shared_ptr<BaseSession> session);

      // Disconnect from a given Session (client or server) based on nodeId.
      bool disconnectSession(const Hash &nodeId);

      // Starts WebSocket server.
      void startServer();

      // Starts the discovery thread.
      void startDiscovery();

      // Stop the discovery thread.
      void stopDiscovery();

      // Gets a copy of keys of the sessions map
      std::vector<Hash> getSessionsIDs();

      // Connects to a given WebSocket server.
      void connectToServer(const std::string &host, const unsigned short &port);

      // Cleans all current connections and stop the server.
      void stop();

      // Handles a message from a session.
      // the shared_ptr here is not a reference because handleMessage is called from another thread, requiring a copy of the pointer
      // The other handler functions are called from the same thread (from handleMessage) and therefore can use a reference.
      // Overriden by inherited object
      virtual void handleMessage(std::shared_ptr<BaseSession> session, const Message message) {};

      // Public request functions
      void ping(const Hash &nodeId);

      std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash>
      requestNodes(const Hash &nodeId);


      const Hash &nodeId() const { return nodeId_; }

      const NodeType &nodeType() const { return nodeType_; }

      const unsigned int serverPort() const { return hostPort_; }

      const bool isServerRunning() const { return this->p2pserver_->isRunning(); }

      const unsigned int maxConnections() const { return maxConnections_; }

      friend class DiscoveryWorker;
  };
};

#endif  // P2PMANAGERBASE_H
