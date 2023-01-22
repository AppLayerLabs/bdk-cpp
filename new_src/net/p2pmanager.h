#ifndef P2PMANAGER_H
#define P2PMANAGER_H

#include <memory>
#include <mutex>
#include <unordered_map>
#include <condition_variable>

#include <boost/asio/ip/address.hpp>

#include "p2pclient.h"
#include "p2pencoding.h"
#include "p2pserver.h"
#include "../utils/utils.h"

struct ConnectionInfo {
  uint64_t version = 0;
  uint64_t timestamp = 0;
  uint64_t latestBlockHeight = 0;
  Hash latestBlockHash = Hash();
  uint64_t nodes = 0;
  uint64_t lastNodeCheck = 0;
  uint64_t clockDiff = 0;
  ConnectionInfo operator=(const ConnectionInfo& other) {
    version = other.version;
    timestamp = other.timestamp;
    latestBlockHeight = other.latestBlockHeight;
    latestBlockHash = other.latestBlockHash;
    nodes = other.nodes;
    lastNodeCheck = other.lastNodeCheck;
    clockDiff = other.clockDiff;
    return *this;
  }
};

template <typename T> class Connection {
  private:
    ConnectionInfo info;
    const boost::asio::ip::address host;
    const unsigned short port;
    const shared_ptr<T> session;

  public:
    Connection(
      const boost::asio::ip::address& host,
      const unsigned short& port,
      const std::shared_ptr<T>& session
    ) : host(host), port(port), session(session);

    ConnectionInfo& getInfo() { return this->info; }
    const boost::asio::ip::address& getHost() { return this->host; }
    const unsigned short& getPort() { return this->port; }
    const std::shared_ptr<T>& getSession() { return this->session; }

    void setInfo(const ConnectionInfo& info) { this->info = info; }

    // Connections should be unique per IP/port combo
    bool operator==(const Connection<T>& other) {
      return (address == other.address && port == other.port)
    }
};

class P2PManager : public std::enable_shared_from_this<P2PManager> {
  private:
    std::vector<Connection<P2PClient>> connServers;
    std::vector<Connection<P2PServerSession> connClients;
    uint64_t connCt = 0;
    std::mutex connServersLock;
    std::mutex connClientsLock;
    std::mutex connCtLock;
    const boost::asio::ip::address serverHost;
    const unsigned short serverPort;
    const unsigned int serverThreads;
    const std::shared_ptr<P2PServer> server;
    const std::shared_ptr<Storage> storage;
    Blockchain& blockchain;

  public;
    P2PManager(
      const boost::asio::ip::address& serverHost, const unsigned short& serverPort,
      const unsigned int& serverThreads, const std::shared_ptr<Storage> storage,
      Blockchain& blockchain
    ) : serverHost(serverHost), serverPort(serverPort), serverThreads(serverThreads),
        storage(storage), blockchain(blockchain)
    {}

    const vector<Connection<P2PClient>>& getConnServers() { return this->connServers; }
    const vector<Connection<P2PServerSession>>& getConnClients() { return this->connClients; }
    const uint64_t& getConnCt() { return this->connCt; }

    void startServer();
    void addClient(const Connection<P2PServerSession> conn);
    void removeClient(const Connection<P2PServerSession> conn);
    void connectToServer(const boost::asio::ip::address& host, const unsigned short& port)
    void disconnectFromServer(const Connection<P2PClient> conn);
    void parseClientMsg(const P2PMsg& msg, const shared_ptr<P2PServerSession>& conn);
    void parseServerMsg(const P2PMsg& msg, const shared_ptr<P2PClient>& conn);
    void broadcastTx(const TxBlock& tx);
    void broadcastValidatorTx(const TxValidator& tx);
    void requestValidatorTxsToAll();
};

#endif  // P2PMANAGER_H
