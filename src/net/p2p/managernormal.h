/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef P2P_MANAGER_NORMAL_H
#define P2P_MANAGER_NORMAL_H

#include "managerbase.h"

// Forward declaration.
class rdPoS;
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
      void handleRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message) override;

      /**
       * Handle an answer from a server.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message) override;

      /**
       * Handle a broadcast from a node.
       * @param session The session that sent the broadcast.
       * @param message The broadcast message to handle.
       */
      void handleBroadcast(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

    private:
      /// Reference to the rdPoS object.
      rdPoS& rdpos_;

      /// Reference to the blockchain's storage.
      const Storage& storage_;

      /// Reference to the blockchain's state.
      State& state_;

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
       * Handle a `Ping` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handlePingRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `Info` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleInfoRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestNodes` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleRequestNodesRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestValidatorTxs` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleTxValidatorRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestTxs` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleTxRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `Ping` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handlePingAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `Info` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleInfoAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestNodes` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleRequestNodesAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestValidatorTxs` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleTxValidatorAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestTxs` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleTxAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a Validator transaction broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleTxValidatorBroadcast(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a block transaction broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleTxBroadcast(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a block broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleBlockBroadcast(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

    public:
      /**
       * Constructor.
       * @param hostIp The manager's host IP/address.
       * @param rdpos Pointer to the rdPoS object.
       * @param options Pointer to the options singleton.
       * @param storage Pointer to the blockchain's storage.
       * @param state Pointer to the blockchain's state.
       */
      ManagerNormal(
        const boost::asio::ip::address& hostIp, rdPoS& rdpos,
        const Options& options, const Storage& storage,
        State& state
      ) : ManagerBase(hostIp, NodeType::NORMAL_NODE, 50, options),
      rdpos_(rdpos), storage_(storage), state_(state)
      {};

      /// Destructor. Automatically stops the manager.
      ~ManagerNormal() { this->stop(); }

      /**
       * Handle a message from a session. Entry point for all the other handlers.
       * @param session The session that sent the message.
       * @param message The message to handle.
       */
      void handleMessage(std::weak_ptr<Session> session, const std::shared_ptr<const Message> message) override;

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
  };
};

#endif  // P2PMANAGERNORMAL_H
