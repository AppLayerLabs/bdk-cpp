/*
Copyright (c) [2023-2024] [AppLayer Developers]

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
  class ManagerBase : public Log::LogicalLocationProvider {
    protected:
      static std::atomic<int> instanceIdGen_; ///< Instance ID generator.
      static std::atomic<int> netThreads_; ///< Size of the IO thread pool (this is read and used in start()).

      const unsigned short serverPort_; ///< The manager's port.
      const NodeType nodeType_; ///< The manager's node type.
      const unsigned int minConnections_; ///< Minimum number of simultaneous connections. @see DiscoveryWorker
      const unsigned int maxConnections_; ///< Maximum number of simultaneous connections.
      std::atomic<bool> started_ = false; ///< Check if manager is in the start() state (stop() not called yet).
      std::atomic<bool> stopping_ = false; ///< Indicates whether the manager is in the process of stopping.
      std::unique_ptr<BS::thread_pool_light> threadPool_; ///< Pointer to the thread pool.
      const Options& options_; /// Reference to the options singleton.
      mutable std::shared_mutex stateMutex_; ///< Mutex for serializing start(), stop(), and threadPool_.
      mutable std::shared_mutex sessionsMutex_; ///< Mutex for managing read/write access to the sessions list.
      mutable std::shared_mutex requestsMutex_; ///< Mutex for managing read/write access to the requests list.
      Server server_; ///< Server object.
      ClientFactory clientfactory_; ///< ClientFactory object.
      DiscoveryWorker discoveryWorker_; ///< DiscoveryWorker object.
      const std::string instanceIdStr_; ///< Instance ID for LOGxxx().

      /// List of currently active sessions.
      std::unordered_map<NodeID, std::shared_ptr<Session>, SafeHash> sessions_;

      // TODO: Somehow find a way to clean up requests_ after a certain time/being used.
      /// List of currently active requests.
      std::unordered_map<RequestID, std::shared_ptr<Request>, SafeHash> requests_;

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
      void answerSession(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      // TODO: There is a bug with handleRequest that throws std::system_error.
      // I believe that this is related with the std::shared_ptr<Session> getting deleted or
      // the session itself being disconnected.
      /**
       * Handle a request from a session (meant to be overriden on child classes).
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      virtual void handleRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message) {
        // Do nothing by default, child classes are meant to override this
      }

      /**
       * Handle an answer from a session (meant to be overriden on child classes).
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      virtual void handleAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message) {
        // Do nothing by default, child classes are meant to override this
      }

    public:
      /**
       * Constructor.
       * @param hostIp The manager's host IP.
       * @param nodeType The manager's node type.
       * @param options Reference to the options singleton.
       * @param minConnections The minimum number of simultaneous connections.
       * @param maxConnections The maximum number of simultaneous connections.
       */
      ManagerBase(
        const net::ip::address& hostIp, NodeType nodeType, const Options& options,
        const unsigned int& minConnections, const unsigned int& maxConnections
      );

      /// Destructor. Automatically stops the manager.
      virtual ~ManagerBase() { this->stopDiscovery(); this->stop(); }

      virtual std::string getLogicalLocation() const { return this->instanceIdStr_; }

      static void setNetThreads(int netThreads);

      const Options& getOptions() { return this->options_; } ///< Get a reference to the Options object given to the P2P engine.

      virtual void start(); ///< Start P2P::Server and P2P::ClientFactory.
      virtual void stop(); ///< Stop the P2P::Server and P2P::ClientFactory.

      /// Start the discovery thread.
      void startDiscovery() { this->discoveryWorker_.start(); }

      /// Stop the discovery thread.
      void stopDiscovery() { this->discoveryWorker_.stop(); }

      /// Get the current sessions' IDs from the list.
      std::vector<NodeID> getSessionsIDs() const;

      /// Get the current Session ID's for the given NodeType.
      std::vector<NodeID> getSessionsIDs(const NodeType& nodeType) const;

      ///@{
      /** Getter. */
      unsigned int serverPort() const { return this->serverPort_; }
      const NodeType& nodeType() const { return this->nodeType_; }
      unsigned int maxConnections() const { return this->maxConnections_; }
      unsigned int minConnections() const { return this->minConnections_; }
      ///@}

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
       * Entrust the internal thread pool to call handleMessage() with the supplied arguments.
       * @param session The session to send an answer to.
       * @param message The message to handle.
       */
      void asyncHandleMessage(const NodeID &nodeId, const std::shared_ptr<const Message> message);

      /**
       * Handle a message from a session.
       * The pointer is a weak_ptr because the parser doesn't need to own the session.
       * The session is owned by the manager (if registered) and the session io_context itself.
       * Other handler functions are called from the same thread.
       * (from handleMessage) and therefore can use a reference.
       * @param session The session to send an answer to.
       * @param message The message to handle.
       */
      virtual void handleMessage(const NodeID &nodeId, const std::shared_ptr<const Message> message) {
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
