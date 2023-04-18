#ifndef P2PMANAGERNORMAL_H
#define P2PMANAGERNORMAL_H

#include <iostream>
#include <shared_mutex>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "../../utils/utils.h"
#include "../../utils/safehash.h"

#include "p2pmanagerbase.h"
#include "p2pbase.h"
#include "p2pclient.h"
#include "p2pencoding.h"
#include "p2pserver.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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
      void handleRequest(std::shared_ptr<BaseSession>& session, const Message& message) override;

      /**
       * Handle an answer from a server.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleAnswer(std::shared_ptr<BaseSession>& session, const Message& message) override;

      /**
       * Handle a broadcast from a node.
       * @param session The session that sent the broadcast.
       * @param message The broadcast message to handle.
       */
      void handleBroadcast(std::shared_ptr<BaseSession>& session, const Message& message) override;

    private:
      /// Pointer to the rdPoS object.
      const std::unique_ptr<rdPoS>& rdpos_;

      /// Pointer to the blockchain's storage.
      const std::unique_ptr<Storage>& storage_;

      /// Pointer to the blockchain's state.
      const std::unique_ptr<State>& state_;

      /**
       * Map with broadcasted messages and a counter of how many times they were broadcast.
       * Used to avoid broadcasting the same message multiple times.
       */
      std::unordered_map <uint64_t, unsigned int, SafeHash> broadcastedMessages_;

      /// Mutex for managing read/write access to broadcasted messages.
      std::shared_mutex broadcastMutex;

      /// Mutex for managing read/write access to block broadcasts.
      std::mutex blockBroadcastMutex;

      /**
       * Broadcast a message to all connected nodes.
       * @param message The message to broadcast.
       */
      void broadcastMessage(const Message& message);

      /**
       * Handle a `Ping` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handlePingRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `Info` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleInfoRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `RequestNodes` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleRequestNodesRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `RequestValidatorTxs` request.
       * @param session The session that sent the request.
       * @param message The request message to handle.
       */
      void handleTxValidatorRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `Ping` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handlePingAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `Info` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleInfoAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `RequestNodes` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleRequestNodesAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a `RequestValidatorTxs` answer.
       * @param session The session that sent the answer.
       * @param message The answer message to handle.
       */
      void handleTxValidatorAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a Validator transaction broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleTxValidatorBroadcast(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a block transaction broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleTxBroadcast(std::shared_ptr<BaseSession>& session, const Message& message);

      /**
       * Handle a block broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleBlockBroadcast(std::shared_ptr<BaseSession>& session, const Message& message);

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
        const boost::asio::ip::address& hostIp, const std::unique_ptr<rdPoS>& rdpos,
        const std::unique_ptr<Options>& options, const std::unique_ptr<Storage>& storage,
        const std::unique_ptr<State>& state
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
      void handleMessage(std::shared_ptr<BaseSession> session, Message message) override;

      /**
       * Request Validator transactions from a given node.
       * @param nodeId The ID of the node to request.
       * @return A list of the node's Validator transactions.
       */
      std::vector<TxValidator> requestValidatorTxs(const Hash& nodeId);

      /**
       * Request info about a given node.
       * @param nodeId The ID of the node to request.
       * @return A struct with the node's info.
       */
      NodeInfo requestNodeInfo(const Hash& nodeId);

      /**
       * Broadcast a Validator transaction to all connected nodes.
       * @param tx The transaction to broadcast.
       */
      void broadcastTxValidator(const TxValidator& tx);

      /**
       * Broadcast a block transaction to all connected nodes.
       * @param tx The transaction to broadcast.
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
