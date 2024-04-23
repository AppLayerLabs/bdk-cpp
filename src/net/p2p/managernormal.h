/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef P2P_MANAGER_NORMAL_H
#define P2P_MANAGER_NORMAL_H

#include "managerbase.h"
#include "nodeconns.h"

#include <optional>

// Forward declaration.
class Storage;
class State;

namespace P2P {

  /// Manager focused exclusively at Normal nodes.
  class ManagerNormal : public ManagerBase {
    protected:
      /**
       * Handle a request from a client.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message) override;

      /**
       * Handle an answer from a server.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message) override;

      /**
       * Handle a broadcast from a node.
       * @param session The session that sent the broadcast.
       * @param message The broadcast message to handle.
       */
      void handleBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a notification from a node.
       * @param session The session that sent the notification.
       * @param message The notification message to handle.
       */
      void handleNotification(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

    private:
      P2P::NodeConns nodeConns_; ///< P2P engine's logical peer connection tracking & keepalive component.

      const Storage& storage_; ///< Reference to the blockchain's storage.
      State& state_; ///< Reference to the blockchain's state.

      /**
       * Map with broadcasted messages and a counter of how many times they were broadcast.
       * Used to avoid broadcasting the same message multiple times.
       */
      std::unordered_map <uint64_t, unsigned int, SafeHash> broadcastedMessages_;

      /// Mutex for managing read/write access to broadcasted messages.
      std::shared_mutex broadcastMutex_;

      /// Mutex for managing read/write access to block broadcasts.
      std::mutex blockBroadcastMutex_;

      /**
       * Broadcast a message to all connected nodes.
       * @param message The message to broadcast.
       */
      void broadcastMessage(const std::shared_ptr<const Message> message);

      /**
       * Send a notification message to all connected nodes.
       * @param message The message to notify all connected nodes.
       */
      void notifyAllMessage(const std::shared_ptr<const Message> message);

      /**
       * Handle a `Ping` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handlePingRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `Info` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleInfoRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestNodes` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleRequestNodesRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestValidatorTxs` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleTxValidatorRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestTxs` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleTxRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestBlock` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleRequestBlockRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `Ping` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handlePingAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `Info` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleInfoAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestNodes` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleRequestNodesAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestValidatorTxs` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleTxValidatorAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestTxs` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleTxAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestBlock` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleRequestBlockAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a Validator transaction broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleTxValidatorBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a block transaction broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleTxBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a block broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleBlockBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a info broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleInfoBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a info notification message.
       * @param session The node that sent the notification.
       * @param message The notification message to handle.
       */
      void handleInfoNotification(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

    public:
      /**
       * Constructor.
       * @param hostIp The manager's host IP/address.
       * @param options Pointer to the options singleton.
       * @param storage Pointer to the blockchain's storage.
       * @param state Pointer to the blockchain's state.
       */
      ManagerNormal(
        const boost::asio::ip::address& hostIp, const Options& options, const Storage& storage, State& state
      ) : ManagerBase(hostIp, NodeType::NORMAL_NODE, options, options.getMinNormalConns(), options.getMaxNormalConns()),
        storage_(storage), state_(state), nodeConns_(*this)
      {}

      /// Destructor. Automatically stops the manager.
      ~ManagerNormal() { this->stop(); }

      /// Get a reference to the NodeConns component.
      P2P::NodeConns& getNodeConns() { return this->nodeConns_; }

      /// Start the P2P engine
      virtual void start() { ManagerBase::start(); nodeConns_.start(); }

      /// Stop the P2P engine
      virtual void stop() { nodeConns_.stop(); ManagerBase::stop(); }

      /**
       * Handle a message from a session. Entry point for all the other handlers.
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      void handleMessage(const NodeID &nodeId, const std::shared_ptr<const Message> message) override;

      /**
       * Request Validator transactions from a given node.
       * @param nodeId The ID of the node to request.
       * @return A list of the node's Validator transactions.
       */
      std::vector<TxValidator> requestValidatorTxs(const NodeID& nodeId);

      /**
       * Request Validator transactions from a given node.
       * @param nodeId The ID of the node to request.
       * @return A list of the node's Validator transactions.
       */
      std::vector<TxBlock> requestTxs(const NodeID& nodeId);

      /**
       * Request info about a given node.
       * @param nodeId The ID of the node to request.
       * @return A struct with the node's info.
       */
      NodeInfo requestNodeInfo(const NodeID& nodeId);

      /**
       * Request a block to a peer.
       * @param nodeId The ID of the node to request.
       * @param height The block height to request.
       * @return The requested block, or an empty optional on error.
       */
      std::optional<Block> requestBlock(const NodeID& nodeId, const uint64_t& height);

      /**
       * Broadcast a Validator transaction to all connected nodes.
       * @param tx The transaction to broadcast.
       */
      void broadcastTxValidator(const TxValidator& tx);

      /**
       * Broadcast a block transaction to all connected nodes.
       * @param txBlock The transaction to broadcast.
       */
      void broadcastTxBlock(const TxBlock& txBlock);

      /**
       * Broadcast a block to all connected nodes.
       * @param block The block to broadcast.
       */
      void broadcastBlock(const std::shared_ptr<const Block> block);

      /**
       * Broadcast current node info
       */
      void broadcastInfo();

      /**
       * Notify all connected peers of our current node info
       */
      void notifyAllInfo();
  };
};

#endif  // P2PMANAGERNORMAL_H
