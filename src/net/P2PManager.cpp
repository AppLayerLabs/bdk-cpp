#include "P2PManager.h"


void P2PManager::startServer() {
  server = std::make_shared<P2PServer>(this->server_address.to_string(), this->server_port, 2, shared_from_this());
  std::thread serverThread(&P2PServer::start, server);
  serverThread.detach();
  return;
}

void P2PManager::addClient(const ConnectionInfo &connInfo, std::shared_ptr<ServerSession> session) {
  clients_mutex.lock();
  connectedClientsMap[connInfo] = session;
  clients_mutex.unlock();
}

void P2PManager::removeClient(const ConnectionInfo& connInfo) {
  clients_mutex.lock();
  connectedClientsMap.erase(connInfo);
  clients_mutex.unlock();
}

void P2PManager::connectToServer(const ConnectionInfo connInfo) {
  Utils::LogPrint(Log::P2PManager, __func__, std::string("Trying to connect to: ") + connInfo.address.to_string() + ":" + std::to_string(connInfo.port));
  std::thread clientThread([&, this, connInfo] {
    net::io_context ioc;
    auto client = std::make_shared<P2PClient>(ioc, connInfo.address.to_string(), connInfo.port, shared_from_this());
    {
      this->servers_mutex.lock();
      this->connectedServersMap[connInfo] = client;
      this->servers_mutex.unlock();
    }
    client->run();
    ioc.run();
  });
  clientThread.detach();
  return;
}