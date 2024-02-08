/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef P2P_MANAGER_BASE
#define P2P_MANAGER_BASE

#include "session.h"
#include "encoding.h"
#include "server.h"
#include "client.h"
#include "discovery.h"
#include "../../utils/options.h"
#include "../../libs/BS_thread_pool_light.hpp"

namespace P2P {
  /**
   * Base manager class meant to be inherited by the respective managers for
   * both node types (Normal and Discovery).
   */
  class ManagerBase {
    protected:
      /// The manager's port.
      const unsigned short serverPort_;

      /// The manager's node type.
      const NodeType nodeType_;

      /// Maximum number of simultaneous connections.
      const unsigned int maxConnections_;

      /// Minimum number of simultaneous connections. See DiscoveryWorker for more information.
      const unsigned int minConnections_ = 11;

      /// Indicates whether the manager is closed to new connections.
      std::atomic<bool> closed_ = true;

      /// Pointer to the thread pool.
      BS::thread_pool_light threadPool_;

      /// Pointer to the options singleton.
      const Options& options_;

      /// Mutex for managing read/write access to the sessions list.
      mutable std::shared_mutex sessionsMutex_;

      /// Mutex for managing read/write access to the requests list.
      mutable std::shared_mutex requestsMutex_;

      /// List of currently active sessions.
      std::unordered_map<NodeID, std::shared_ptr<Session>, SafeHash> sessions_;

      // TODO: Somehow find a way to clean up requests_ after a certain time/being used.
      /// List of currently active requests.
      std::unordered_map<RequestID, std::shared_ptr<Request>, SafeHash> requests_;

      /// Server Object
      Server server_;

      /// ClientFactory Object
      ClientFactory clientfactory_;

      /// DiscoveryWorker.
      DiscoveryWorker discoveryWorker_;

      /// Internal register function for sessions.
      bool registerSessionInternal(const std::shared_ptr<Session>& session);

      /// Internal unregister function for sessions.
      bool unregisterSessionInternal(const std::shared_ptr<Session>& session);

      /// Internal disconnect function for sessions.
      bool disconnectSessionInternal(const NodeID& session);

      /**
       * Send a Request to a given node.
       * @param nodeId The ID of the node to send the message to.
       * @param message The message to send.
       * @return A pointer to the request object, or null on error.
       */
      std::shared_ptr<Request> sendRequestTo(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Answer a message to a given session.
       * @param session The session to answer to.
       * @param message The message to answer.
       */
      void answerSession(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      // TODO: There is a bug with handleRequest that throws std::system_error.
      // I believe that this is related with the std::shared_ptr<Session> getting deleted or
      // the session itself being disconnected.
      /**
       * Handle a request from a session (meant to be overriden on child classes).
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      virtual void handleRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message) {
        // Do nothing by default, child classes are meant to override this
      }

      /**
       * Handle an answer from a session (meant to be overriden on child classes).
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      virtual void handleAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message) {
        // Do nothing by default, child classes are meant to override this
      }

    public:
      /**
       * Constructor.
       * @param hostIp The manager's host IP.
       * @param nodeType The manager's node type.
       * @param maxConnections The maximum number of simultaneous connections.
       * @param options Pointer to the options singleton.
       */
      ManagerBase(
          const net::ip::address& hostIp, NodeType nodeType,
          unsigned int maxConnections, const Options& options
      ) : serverPort_(options.getP2PPort()), nodeType_(nodeType), maxConnections_(maxConnections), options_(options),
          threadPool_(std::thread::hardware_concurrency() * 4),
          server_(hostIp, options.getP2PPort(), 4, *this, this->threadPool_),
          clientfactory_(*this, 4, this->threadPool_),
          discoveryWorker_(*this) {};

      /// Destructor. Automatically stops the manager.
      ~ManagerBase() {
        this->stopDiscovery();
        this->stop();
      };

      /// Start P2P::Server and P2P::ClientFactory.
      void start();

      /// Stop the P2P::Server and P2P::ClientFactory.
      void stop();

      /// Start the discovery thread.
      void startDiscovery() { this->discoveryWorker_.start(); };

      /// Stop the discovery thread.
      void stopDiscovery() { this->discoveryWorker_.stop(); };

      /// Get the current sessions' IDs from the list.
      std::vector<NodeID> getSessionsIDs() const;

      /// Getter for `nodeType_`.
      const NodeType& nodeType() const { return this->nodeType_; }

      /// Getter for `hostPort_`.
      unsigned int serverPort() const { return this->serverPort_; }

      /// Getter for `maxConnections_`.
      unsigned int maxConnections() const { return this->maxConnections_; }

      /// Getter for `minConnections_`.
      unsigned int minConnections() const { return this->minConnections_; }

      /// Getter for `closed_`.
      const std::atomic<bool>& isClosed() const { return this->closed_; }

      /// Get the size of the session list.
      uint64_t getPeerCount() const { std::shared_lock lock(this->sessionsMutex_); return this->sessions_.size(); }

      /// Check if the P2P server is running.
      bool isServerRunning() const { return this->server_.isRunning(); }

      /**
       * Register a session into the list.
       * @param session The session to register.
       */
      bool registerSession(const std::shared_ptr<Session>& session);

      /**
       * Unregister a session from the list.
       * @param session The session to unregister.
       */
      bool unregisterSession(const std::shared_ptr<Session>& session);

      /**
       * Disconnect from a session.
       * @param nodeId The ID of the session to disconnect from.
       * @return `true` if the session was successfully disconnected from,
       *         `false` if the session does not exist.
       */
      bool disconnectSession(const NodeID &nodeId);

      /**
       * Connect to a given websocket server.
       * @param address The websocket's address.
       * @param port The websocket's port.
       */
      void connectToServer(const boost::asio::ip::address& address, uint16_t port);

      /**
       * Handle a message from a session.
       * The pointer is a weak_ptr because the parser doesn't need to own the session.
       * The session is owned by the manager (if registered) and the session io_context itself.
       * Other handler functions are called from the same thread.
       * (from handleMessage) and therefore can use a reference.
       * @param session The session to send an answer to.
       * @param message The message to handle.
       */
      virtual void handleMessage(std::weak_ptr<Session> session, const std::shared_ptr<const Message> message) {
        // Do nothing by default, child classes are meant to override this
      }

      /**
       * Ping a node and wait for it to answer. Throws on failure.
       * @param nodeId The ID of the node to ping.
       */
      void ping(const NodeID &nodeId);

      /**
       * Request the list of connected nodes from a given node.
       * @param nodeId The node to ask for.
       * @return The node's list of connected nodes.
       */
      std::unordered_map<NodeID, NodeType, SafeHash> requestNodes(const NodeID& nodeId);

      friend class DiscoveryWorker;
  };
}

#endif // P2P_MANAGER_BASE
