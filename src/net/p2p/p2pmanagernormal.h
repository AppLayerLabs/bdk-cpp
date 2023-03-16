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

namespace P2P {
  class ManagerNormal : public ManagerBase {
    protected:
      // Handlers for client and server requests.
      // Handle message (called from sessions) is public.
      void handleRequest(std::shared_ptr<BaseSession>& session, const Message& message) override;
      void handleAnswer(std::shared_ptr<BaseSession>& session, const Message& message) override;
      void handleBroadcast(std::shared_ptr<BaseSession>& session, const Message& message) override;

    private:
      const std::unique_ptr<rdPoS>& rdpos;

      // Handlers for command requests
      void handlePingRequest(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleRequestNodesRequest(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleTxValidatorRequest(std::shared_ptr<BaseSession>& session, const Message& message);

      // Handlers for command answers
      void handlePingAnswer(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleRequestNodesAnswer(std::shared_ptr<BaseSession>& session, const Message& message);
      void handleTxValidatorAnswer(std::shared_ptr<BaseSession>& session, const Message& message);

      // Handlers for command broadcasts
      void handleTxValidatorBroadcast(std::shared_ptr<BaseSession>& session, const Message& message);

    public:
      ManagerNormal(const boost::asio::ip::address& hostIp, 
                    unsigned short hostPort, 
                    const std::unique_ptr<rdPoS>& rdpos) : 
                    ManagerBase(hostIp, hostPort, NodeType::NORMAL_NODE, 50), rdpos(rdpos) {};
      
      void handleMessage(std::shared_ptr<BaseSession> session, const Message message) override;

      std::vector<TxValidator> requestValidatorTxs(const Hash& nodeId);

  };
};

#endif
