/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef P2P_MANAGER_DISCOVERY_H
#define P2P_MANAGER_DISCOVERY_H

#include "managerbase.h"

#include <algorithm>
#include <iterator>

namespace P2P {
  /// Manager focused exclusively at Discovery nodes.
  class ManagerDiscovery : public ManagerBase {
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

    private:
      /**
       * Handle a `Ping` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handlePingRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestNodes` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleRequestNodesRequest(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `Ping` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handlePingAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

      /**
       * Handle a `RequestNodes` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleRequestNodesAnswer(std::weak_ptr<Session> session, const std::shared_ptr<const Message>& message);

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
      void handleMessage(std::weak_ptr<Session> session, const std::shared_ptr<const Message> message) override;
  };
};

#endif  // P2PMANAGERDISCOVERY_H
