/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef P2P_MANAGER_BASE
#define P2P_MANAGER_BASE

#include "discovery.h"
#include "session.h" // encoding.h -> utils/options.h

class DiscoveryWorker; // Forward declaration

namespace P2P {
  /**
   * Base manager class meant to be inherited by the respective managers for
   * both node types (Normal and Discovery).
   */
  class ManagerBase : public Log::LogicalLocationProvider {
    protected:

      /// Helper class that encapsulates our ASIO-based networking engine
      class Net : public Log::LogicalLocationProvider, public std::enable_shared_from_this<Net> {
      private:
        ManagerBase& manager_;
        net::io_context io_context_; ///< io_context for the P2P engine
        net::executor_work_guard<net::io_context::executor_type> work_guard_; ///< Work guard for the io_context
        int netThreads_; ///< Size of thread pool
        boost::asio::thread_pool threadPool_; ///< thread pool that runs the P2P engine
        net::strand<net::io_context::executor_type> connectorStrand_; ///< strand for outbound connections
        net::strand<net::io_context::executor_type> acceptorStrand_; ///< strand for inbound connections
        net::ip::tcp::acceptor acceptor_; ///< listen socket
        std::atomic<bool> stopped_ = false; ///< Set to true as soon as stop() starts
        void handleOutbound(const boost::asio::ip::address &address, const unsigned short &port); ///< Complete TCP connection
        void handleInbound(boost::system::error_code ec, net::ip::tcp::socket socket); ///< Complete TCP connection
        void doAccept(); ///< Wait for the next inbound TCP connection request
      public:
        std::string getLogicalLocation() const override { return manager_.getLogicalLocation(); }
        Net(ManagerBase& manager, int netThreads); ///< Start net engine with netThreads threads, can throw DynamicException
        virtual ~Net(); ///< Stop net engine
        void start(); ///< Initialize the engine
        void stop(); ///< Stop the engine
        void connect(const boost::asio::ip::address& address, uint16_t port); ///< Request connection to a peer
      };

      static std::atomic<int> instanceIdGen_; ///< Instance ID generator.
      static std::atomic<int> netThreads_; ///< Size of the IO thread pool (this is read and used in start()).

      std::shared_ptr<Net> net_; ///< Core P2P networking components instantiated when the P2P Manager is running (started)
      const net::ip::address serverLocalAddress_; ///< The manager's local IP address.
      const unsigned short serverPort_; ///< The manager's port.
      const NodeType nodeType_; ///< The manager's node type.
      const Options& options_; /// Reference to the options singleton.
      const unsigned int minConnections_; ///< Minimum number of simultaneous connections. @see DiscoveryWorker
      const unsigned int maxConnections_; ///< Maximum number of simultaneous connections.
      std::atomic<bool> started_ = false; ///< Check if manager is in the start() state (stop() not called yet).
      mutable std::shared_mutex stateMutex_; ///< Mutex for serializing start(), stop(), and threadPool_.
      mutable std::shared_mutex sessionsMutex_; ///< Mutex for managing read/write access to the sessions list.
      mutable std::shared_mutex requestsMutex_; ///< Mutex for managing read/write access to the requests list.
      DiscoveryWorker discoveryWorker_; ///< DiscoveryWorker object.
      const std::string instanceIdStr_; ///< Instance ID for LOGxxx().
      const NodeID nodeId_; ///< This ManagerBase's own NodeID.

      /// List of currently active sessions.
      boost::unordered_flat_map<NodeID, std::shared_ptr<Session>, SafeHash> sessions_;

      // TODO: Somehow find a way to clean up requests_ after a certain time/being used.
      /// List of currently active requests.
      boost::unordered_flat_map<RequestID, std::shared_ptr<Request>, SafeHash> requests_;

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

      /**
       * Called by a Session to notify handshake completion (successful peer connection).
       * @param session The session that has completed the handshake.
       * @return `true` if registration is successful (relevant for INBOUND connections only), `false` on registration error.
       */
      bool sessionHandshaked(const std::shared_ptr<Session>& callerSession);

      /**
       * Called by a Session object when it closes itself.
       * @param callerSession The Session object that is calling this to notifying the ManagerBase of its closing.
       * @param wasHandshaked Whether the closed session was already handshaked, that is, "Peer Connected" was already raised for it.
       */
      void sessionClosed(const Session& callerSession, bool wasHandshaked);

      /**
       * Called by a Session object when it receives a Message.
       * @param session The session to send an answer to.
       * @param message The message to handle.
       */
      void incomingMessage(const Session& callerSession, const std::shared_ptr<const Message> message);

      /**
       * Called by ManagerBase::Net to try to create, register and run() a new OUTBOUND Session.
       * @param socket TCP socket for the new Session to manage.
       * @param address Remote peer's address.
       * @param port Remote peer's port.
       */
      void trySpawnOutboundSession(tcp::socket&& socket, const boost::asio::ip::address &address, const unsigned short &port);

      /**
       * Called by ManagerBase::Net to try to create and run() a new INBOUND Session.
       * @param socket TCP socket for the new Session to manage.
       */
      void trySpawnInboundSession(tcp::socket&& socket);

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

      std::string getLogicalLocation() const override { return this->instanceIdStr_; }

      /// Ensures the logging ID is not zero, which would generate instanceIdStr_ == "" (for production logs)
      static void setTesting();

      static void setNetThreads(int netThreads);

      const Options& getOptions() const { return this->options_; } ///< Get a reference to the Options object given to the P2P engine.

      virtual void start(); ///< Start P2P::Server and P2P::ClientFactory.
      virtual void stop(); ///< Stop the P2P::Server and P2P::ClientFactory.

      bool isActive() const;

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
      uint64_t getPeerCount() const;

      /// Check if the P2P server is running.
      bool isServerRunning() const { return started_; }

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
       * Handle a message from a Session (or from a unit test that is injecting messages).
       * @param nodeId The message sender.
       * @param message The message to handle.
       */
      virtual void handleMessage(const NodeID &nodeId, const std::shared_ptr<const Message> message) {
        // Do nothing by default, child classes are meant to override this
        LOGERROR("Unexpected error: ManagerBase::handleMessage() called; dropping Message.");
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
      boost::unordered_flat_map<NodeID, NodeType, SafeHash> requestNodes(const NodeID& nodeId);

      friend class DiscoveryWorker;
      friend class Session;
  };
}

#endif // P2P_MANAGER_BASE
