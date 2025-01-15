/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef P2P_MANAGER_DISCOVERY_H
#define P2P_MANAGER_DISCOVERY_H

#include "managerbase.h" // encoding.h -> utils/utils.h -> libs/json.hpp -> algorithm

/// Namespace for P2P-related functionalities.
namespace P2P {
  /// Manager class focused exclusively at Discovery nodes.
  class ManagerDiscovery : public ManagerBase {
    protected:
      /**
       * Handle a request from a client.
       * @param nodeId The ID of the node that sent the request.
       * @param message The request message to handle.
       */
      void handleRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message) override;

      /**
       * Handle an answer from a server.
       * @param nodeId The ID of the node that sent the answer.
       * @param message The answer message to handle.
       */
      void handleAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message) override;

    private:
      /**
       * Handle a `Ping` request.
       * @param nodeId The ID of the node that sent the request.
       * @param message The request message to handle.
       */
      void handlePingRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestNodes` request.
       * @param nodeId The ID of the node that sent the request.
       * @param message The request message to handle.
       */
      void handleRequestNodesRequest(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `Ping` answer.
       * @param nodeId The ID of the node that sent the answer.
       * @param message The answer message to handle.
       */
      void handlePingAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestNodes` answer.
       * @param nodeId The ID of the node that sent the answer.
       * @param message The answer message to handle.
       */
      void handleRequestNodesAnswer(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

    public:
      /**
       * Constructor.
       * @param hostIp The manager's host IP/address.
       * @param options Pointer to the options singleton.
       */
      ManagerDiscovery(
        const boost::asio::ip::address& hostIp, const Options& options
      ) : ManagerBase(hostIp, NodeType::DISCOVERY_NODE, options, options.getMinDiscoveryConns(), options.getMaxDiscoveryConns())
      {}

      /**
       * Handle a message from a session. Entry point for all the other handlers.
       * @param nodeId The ID of the node that sent the message.
       * @param message The message to handle.
       */
      void handleMessage(const NodeID &nodeId, const std::shared_ptr<const Message> message) override;
  };
};

#endif  // P2PMANAGERDISCOVERY_H
