#include <iostream>
#include <shared_mutex>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../../utils/utils.h"
#include "../../utils/safehash.h"

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
  class Manager {
    private:
      const Hash nodeId_;        // Unique node ID, randomly generated at P2PManager constructor, 32 bytes in size, byte encoded.
                                 // Used in the session unordered_map to identify a session to a given node.
                                 // Do not allow multiple sessions to the same node.
      boost::asio::ip::address hostIp_;
      unsigned short hostPort_;
      const std::shared_ptr<Server> p2pserver_;
      const NodeType nodeType_;
      std::unordered_map <Hash, std::shared_ptr<BaseSession>, SafeHash> sessions_;
      std::unordered_map <RequestID, std::shared_ptr<Request>, SafeHash> requests_;

      std::shared_mutex sessionsMutex; // Mutex for protecting sessions
      std::shared_mutex requestsMutex; // Mutex for protecting requests

      // Sends a message to a given node. returns pointer to the request object, null if doesn't exists.
      std::shared_ptr<Request>& sendMessageTo(const Hash& nodeId, const Message& message);
      void answerSession(std::shared_ptr<BaseSession>& session, const Message& message);

      // Handlers for client and server requests.
      // Handle message (called from sessions) is public.
      void handleRequest(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

      // Handlers for command requests
      void handlePingRequest(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleRequestNodesRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      // Handlers for command answers
      void handlePingAnswer(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleRequestNodesAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

    public:
      Manager(const boost::asio::ip::address& hostIp, unsigned short hostPort, NodeType nodeType);

      // Registers a session
      bool registerSession(std::shared_ptr<BaseSession> session);

      // Unregisters a session
      bool unregisterSession(std::shared_ptr<BaseSession> session);

      // Disconnect from a given Session (client or server) based on nodeId.
      bool disconnectSession(const Hash& nodeId);

      // Starts WebSocket server.
      void startServer();

      // Gets a copy of keys of the sessions map
      std::vector<Hash> getSessionsIDs();

      // Connects to a given WebSocket server.
      void connectToServer(const std::string &host, const unsigned short &port);

      // Handles a message from a session.
      // the shared_ptr here is not a reference because handleMessage is called from another thread, requiring a copy of the pointer
      // The other handler functions are called from the same thread (from handleMessage) and therefore can use a reference.
      void handleMessage(std::shared_ptr<BaseSession> session, const Message message);

      // Public request functions
      void ping(const Hash& nodeId);

      std::vector<std::tuple<NodeType, Hash, boost::asio::ip::address, unsigned short>> requestNodes(const Hash& nodeId);

      const Hash& nodeId() const { return nodeId_; }

      const NodeType& nodeType() const { return nodeType_; }

      const unsigned int serverPort() const { return hostPort_; }
  };
};
