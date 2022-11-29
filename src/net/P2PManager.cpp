#include "P2PManager.h"


void P2PManager::startServer() {
  server = std::make_shared<P2PServer>(this->server_address.to_string(), this->server_port, 2, shared_from_this());
  std::thread serverThread(&P2PServer::start, server);
  serverThread.detach();
  return;
}

void P2PManager::addClient(const boost::asio::ip::address& address, std::shared_ptr<ServerSession> session) {
  clients_mutex.lock();
  connectedClients[address] = session;
  clients_mutex.unlock();
}

void P2PManager::removeClient(const boost::asio::ip::address& address) {
  clients_mutex.lock();
  connectedClients.erase(address);
  clients_mutex.unlock();
}

void P2PManager::connectToServer(const boost::asio::ip::address& address, const unsigned short& port) {
  Utils::LogPrint(Log::P2PManager, __func__, std::string("Trying to connect to: ") + address.to_string() + ":" + std::to_string(port));
  std::thread clientThread([&, this] {
    net::io_context ioc;
    this->servers_mutex.lock();
    this->connectedServers[address] = std::make_shared<P2PClient>(ioc, address.to_string(), port, shared_from_this());
    this->servers_mutex.unlock();
    this->connectedServers[address]->run();
    ioc.run();
  });
  clientThread.detach();
  return;
}