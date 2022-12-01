#include "P2PManager.h"


void P2PManager::startServer() {
  server = std::make_shared<P2PServer>(this->server_address.to_string(), this->server_port, 2, shared_from_this());
  std::thread serverThread(&P2PServer::start, server);
  serverThread.detach();
  return;
}

void P2PManager::addClient(const ConnectionInfo<ServerSession> &session) {
  clients_mutex.lock();
  connectedClientsVector.push_back(session);
  clients_mutex.unlock();
}

void P2PManager::removeClient(const ConnectionInfo<ServerSession>& connInfo) {
  clients_mutex.lock();
  // TODO lol
  //connectedClientsVector.erase(connInfo);
  clients_mutex.unlock();
}

void P2PManager::connectToServer(const boost::asio::ip::address &address, const unsigned short &port) {
  Utils::LogPrint(Log::P2PManager, __func__, std::string("Trying to connect to: ") + address.to_string() + ":" + std::to_string(port));
  std::thread clientThread([&, address, port] {
    net::io_context ioc;
    auto client = std::make_shared<P2PClient>(ioc, address.to_string(), port, shared_from_this());
    {
      ConnectionInfo<P2PClient> connInfo(address, port, client);
      this->servers_mutex.lock();
      this->connectedServersVector.push_back(connInfo);
      this->servers_mutex.unlock();
    }
    client->run();
    ioc.run();
  });
  clientThread.detach();
  return;
}