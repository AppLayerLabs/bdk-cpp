#include "P2PManager.h"
#include "../core/subnet.h"

void P2PManager::startServer() {
  server = std::make_shared<P2PServer>(this->server_address.to_string(), this->server_port, 2, shared_from_this());
  std::thread serverThread(&P2PServer::start, server);
  serverThread.detach();
  return;
}

void P2PManager::addClient(const Connection<ServerSession> &session) {
  clients_mutex.lock();
  connectedClientsVector.push_back(session);
  clients_mutex.unlock();
  counter_mutex.lock();
  ++connCounter;
  counter_mutex.unlock();
}

void P2PManager::removeClient(const Connection<ServerSession>& connInfo) {
  clients_mutex.lock();
  // TODO lol
  //connectedClientsVector.erase(connInfo);
  clients_mutex.unlock();
  counter_mutex.lock();
  --connCounter;
  counter_mutex.unlock();
}

void P2PManager::connectToServer(const boost::asio::ip::address &address, const unsigned short &port) {
  Utils::LogPrint(Log::P2PManager, __func__, std::string("Trying to connect to: ") + address.to_string() + ":" + std::to_string(port));
  std::thread clientThread([&, address, port] {
    net::io_context ioc;
    auto client = std::make_shared<P2PClient>(ioc, address.to_string(), port, shared_from_this());
    {
      Connection<P2PClient> connInfo(address, port, client);
      this->servers_mutex.lock();
      this->connectedServersVector.push_back(connInfo);
      this->servers_mutex.unlock();
    }
    client->run();
    counter_mutex.lock();
    ++connCounter;
    counter_mutex.unlock();
    ioc.run();
    counter_mutex.lock();
    --connCounter;
    counter_mutex.unlock();
  });
  clientThread.detach();
  return;
}

const void P2PManager::parseClientRequest(const P2PMessage& message, const std::shared_ptr<ServerSession> &connInfo) {
  Utils::logToFile(std::string("Trying to parse client request: ") + Utils::bytesToHex(message.message()));
  switch (message.command()) {
    case CommandType::Info :
      {
        Utils::LogPrint(Log::P2PManager, __func__, std::string("Received Info from: ") + connInfo->address().to_string() + ":" + std::to_string(connInfo->port()));
        // Parse info and update the connection inside the vector.
        auto newInfo = P2PRequestDecoder::info(message);
        this->clients_mutex.lock();
        for (auto& i : this->connectedClientsVector) {
          if (i.session == connInfo) {
            i.updateInfo(newInfo);
          }
        }
        this->clients_mutex.unlock();
        auto answer = P2PAnswerEncoder::info(this->chainHead, this->connectedClientsVector.size(), message.id());
        connInfo->write(answer);
      }
      break;
    case CommandType::SendTransaction :
      {
        Utils::LogPrint(Log::P2PManager, __func__, std::string("Received Tx from: ") + connInfo->address().to_string() + ":" + std::to_string(connInfo->port()));
        auto tx = P2PRequestDecoder::sendTransaction(message);
        this->subnet.validateTransaction(std::move(tx));
      }
      break;
  } // TODO: Others commands.
}

const void P2PManager::parseServerAnswer(const P2PMessage& message, const std::shared_ptr<P2PClient> &connInfo) {
  Utils::logToFile("Trying to parse servers answer at command: " + std::to_string(message.command()));
  switch (message.command()) {
    case CommandType::Info :
      // Info updates P2PManager itself, no need to route the answer.
      Utils::LogPrint(Log::P2PManager, __func__, std::string("Received Info from: ") + connInfo->host + ":" + std::to_string(connInfo->port));
      auto newInfo = P2PRequestDecoder::info(message);
      this->servers_mutex.lock();
      for (auto& i : this->connectedServersVector) {
        if (i.session == connInfo) {
          i.updateInfo(newInfo);
        }
      }
      this->servers_mutex.unlock();
      break;
  } // TODO: Other Commands
}

const void P2PManager::broadcastTx(const Tx::Base &tx) const {
  P2PMessage message = P2PRequestEncoder::sendTransaction(tx);
  for (auto &s : this->connectedServersVector) {
    Utils::logToFile(std::string("Trying to send to: ") + s.address.to_string() + ":" + std::to_string(s.port) + " tx: " + Utils::bytesToHex(tx.rlpSerialize(true)));
    s.session->write(message);
  }
}