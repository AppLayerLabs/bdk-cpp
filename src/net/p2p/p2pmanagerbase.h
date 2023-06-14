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
#include "../../utils/options.h"

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

namespace P2P {
  // Forward declarations.
  class DiscoveryWorker;

  /**
   * Base manager class meant to be inherited by the respective managers for
   * both node types (Normal and Discovery).
   */
  class ManagerBase {
    protected:
      /**
       * Unique node ID, randomly generated at the constructor, 32 bytes in size, byte encoded.
       * Used in the session unordered_map to identify a session to a given node,
       * and to not allow multiple sessions in the same node.
       */
      const Hash nodeId_;

      /// The manager's IP/address.
      boost::asio::ip::address hostIp_;

      /// The manager's port.
      unsigned short hostPort_;

      /// The manager's node type.
      const NodeType nodeType_;

      /// Maximum number of simultaneous connections.
      const unsigned int maxConnections_;

      /// Minimum number of simultaneous connections. See DiscoveryWorker for more information.
      const unsigned int minConnections_ = 11;

      /// Indicates whether the manager is closed to new connections.
      std::atomic<bool> closed_ = true;

      /// List of currently active sessions.
      std::unordered_map<Hash, std::shared_ptr<BaseSession>, SafeHash> sessions_;

      // TODO: Somehow find a way to clean up requests_ after a certain time/being used.
      /// List of currently active requests.
      std::unordered_map<RequestID, std::shared_ptr<Request>, SafeHash> requests_;

      /// Pointer to the %P2P server.
      const std::shared_ptr<Server> p2pserver_;

      /// Mutex for managing read/write access to the sessions list.
      mutable std::shared_mutex sessionsMutex;

      /// Mutex for managing read/write access to the requests list.
      mutable std::shared_mutex requestsMutex;

      /// Counter for the current number of client sessions.
      std::atomic<unsigned int> clientSessionsCount = 0;

      /// Pointer to the DiscoveryWorker thread.
      const std::unique_ptr<DiscoveryWorker> discoveryWorker;

      /// Pointer to the thread pool.
      const std::unique_ptr<BS::thread_pool_light> threadPool;

      /// Pointer to the options singleton.
      const std::unique_ptr<Options>& options;

      /**
       * Send a message to a given node.
       * @param nodeId The ID of the node to send the message to.
       * @param message The message to send.
       * @return A pointer to the request object, or null on error.
       */
      std::shared_ptr<Request> sendMessageTo(const Hash &nodeId, const Message &message);

      /**
       * Answer a message to a given session.
       * @param session The session to answer to.
       * @param message The message to answer.
       */
      void answerSession(std::shared_ptr<BaseSession> &session, const Message &message);

      // TODO: There is a bug with handleRequest that throws std::system_error.
      // I believe that this is related with the std::shared_ptr<BaseSession> getting deleted or
      // the session itself being disconnected.
      /**
       * Handle a request from a session (meant to be overriden on child classes).
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      virtual void handleRequest(std::shared_ptr<BaseSession> &session, const Message &message) {};

      /**
       * Handle an answer from a session (meant to be overriden on child classes).
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      virtual void handleAnswer(std::shared_ptr<BaseSession> &session, const Message &message) {};

      /**
       * Handle a broadcast from a session (meant to be overriden on child classes).
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      virtual void handleBroadcast(std::shared_ptr<BaseSession> &session, const Message &message) {};

    public:
      /**
       * Constructor.
       * @param hostIp The manager's host IP.
       * @param nodeType The manager's node type.
       * @param maxConnections The maximum number of simultaneous connections.
       * @param options Pointer to the options singleton.
       */
      ManagerBase(
        const boost::asio::ip::address& hostIp, NodeType nodeType,
        unsigned int maxConnections, const std::unique_ptr<Options>& options
      );

      /// Destructor. Automatically stops the manager.
      ~ManagerBase() { stop(); }

      /// Getter for `nodeId_`.
      const Hash &nodeId() const { return nodeId_; }

      /// Getter for `nodeType_`.
      const NodeType &nodeType() const { return nodeType_; }

      /// Getter for `hostPort_`.
      const unsigned int serverPort() const { return hostPort_; }

      /// Getter for `maxConnections_`.
      const unsigned int maxConnections() const { return maxConnections_; }

      /// Getter for `minConnections_`.
      const unsigned int minConnections() const { return minConnections_; }

      /// Getter for `closed_`.
      const std::atomic<bool>& isClosed() const { return closed_; }

      /// Get the size of the session list.
      const uint64_t getPeerCount() const { std::shared_lock lock(this->sessionsMutex); return sessions_.size(); }

      /// Check if the P2P server is running.
      const bool isServerRunning() const { return this->p2pserver_->isRunning(); }

      /**
       * Register a session into the list.
       * @param session The session to register.
       */
      bool registerSession(std::shared_ptr<BaseSession> session);

      /**
       * Unregister a session from the list.
       * @param session The session to unregister.
       */
      bool unregisterSession(std::shared_ptr<BaseSession> session);

      /**
       * Disconnect from a session.
       * @param nodeId The ID of the session to disconnect from.
       * @return `true` if the session was successfully disconnected from,
       *         `false` if the session does not exist.
       */
      bool disconnectSession(const Hash &nodeId);

      /// Start the websocket server.
      void startServer();

      /// Start the discovery thread.
      void startDiscovery();

      /// Stop the discovery thread.
      void stopDiscovery();

      /// Get the current sessions' IDs from the list.
      std::vector<Hash> getSessionsIDs();

      /**
       * Connect to a given websocket server.
       * @param host The websocket's host.
       * @param port The websocket's port.
       */
      void connectToServer(const std::string &host, const unsigned short &port);

      /// Close all current sessions, stop accepting new ones, and stop the server altogether.
      void stop();

      /**
       * Handle a message from a session.
       * The shared_ptr here is not a reference because handleMessage
       * is called from another thread, requiring a copy of the pointer.
       * Other handler functions are called from the same thread
       * (from handleMessage) and therefore can use a reference.
       * Overriden by inherited object.
       * @param session The session to send an answer to.
       * @param message The message to handle.
       */
      virtual void handleMessage(std::shared_ptr<BaseSession> session, const Message message) {};

      /**
       * Ping a node and wait for it to answer. Throws on failure.
       * @param nodeId The ID of the node to ping.
       */
      void ping(const Hash &nodeId);

      /**
       * Request the list of connected nodes from a given node.
       * @param nodeId The node to ask for.
       * @return The node's list of connected nodes.
       */
      std::unordered_map<
        Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash
      > requestNodes(const Hash &nodeId);

      friend class DiscoveryWorker;
  };
};

#endif  // P2PMANAGERBASE_H
