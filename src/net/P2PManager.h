#ifndef P2PMANAGER_H
#define P2PMANAGER_H

#include "P2PClient.h"
#include "P2PServer.h"
#include "P2PEncoding.h"
#include <memory>
#include <unordered_map>
#include <condition_variable>
#include "../utils/utils.h"

// Forward declaration.
class Subnet;

struct ConnectionInfo {
  // Clock difference between our node and the node we are connected to.
  uint64_t version = 0; 
  // Timestamp.
  uint64_t timestamp = 0;
  // Their best block height.
  uint64_t latestBlockHeight = 0;
  // Their best block hash
  Hash latestBlockHash = Hash();
  // Nodes connnected to it.
  uint64_t nNodes = 0;
  // Last time we checked their info.
  uint64_t latestChecked = 0;
  // Clock difference.
  uint64_t clockDiff = 0; // ** approximate **
  ConnectionInfo operator=(const ConnectionInfo& other) {
    version = other.version;
    timestamp = other.timestamp;
    latestBlockHeight = other.latestBlockHeight;
    latestBlockHash = other.latestBlockHash;
    nNodes = other.nNodes;
    latestChecked = other.latestChecked;
    clockDiff = other.clockDiff;
    return *this;
  }
};

template <typename T>
class Connection {
  private:
    ConnectionInfo connInfo;
  public:
    const std::shared_ptr<T> session;
    const boost::asio::ip::address address;
    const unsigned short port;
    Connection(const boost::asio::ip::address &_address, const unsigned short &_port, const std::shared_ptr<T> &_session) : 
      address(_address), port(_port), session(_session) {}

    const uint64_t& version() const { return connInfo.version; }
    const uint64_t& timestamp() const { return connInfo.timestamp; }
    const uint64_t& latestBlockHeight() const { return connInfo.latestBlockHeight; }
    const Hash& latestBlockHash() const { return connInfo.latestBlockHash; }
    const uint64_t& nNodes() const { return connInfo.nNodes; }
    const uint64_t& latestChecked() const { return connInfo.latestChecked; }
    const uint64_t& clockDiff() const { return connInfo.clockDiff; }
    void updateInfo(const ConnectionInfo& info) { connInfo = info; }

    bool operator==(const Connection<T>& other) const {
      return address == other.address && port == other.port; // Connections should be unique per IP/port.
    }
};


class P2PManager : public std::enable_shared_from_this<P2PManager> {
    std::shared_ptr<P2PServer> server;
    std::vector<Connection<P2PClient>> connectedServersVector;
    std::mutex servers_mutex; // Connected servers.
    std::vector<Connection<ServerSession>> connectedClientsVector;
    std::mutex clients_mutex; // Connected clients
    std::mutex counter_mutex;

    const boost::asio::ip::address server_address;
    const unsigned short server_port;
    const unsigned int server_threads;

    uint64_t connCounter = 0;
  public:
    const std::shared_ptr<const ChainHead> chainHead;
    Subnet &subnet;

    P2PManager(const boost::asio::ip::address &address, const unsigned short &server_port, const unsigned int &server_threads, const std::shared_ptr<const ChainHead> _chainHead, Subnet &_subnet)
       : server_address(address), server_port(server_port), server_threads(server_threads), chainHead(_chainHead), subnet(_subnet) {};

    void startServer();
    void parseServerMessage(const std::string& message);
    
    // Insert a new client into the connected Clients.
    void addClient(const Connection<ServerSession> &connInfo);
    // Remove the said client from the connected Clients.
    void removeClient(const Connection<ServerSession> &connInfo);

    // Create a new thread running the client and connect to the server.
    void connectToServer(const boost::asio::ip::address &address, const unsigned short &port);
    // Disconect a given client from a server.
    void disconnectFromServer(const Connection<P2PClient> &address);

    uint64_t connectionCount() const { return connCounter; }

    const std::vector<Connection<P2PClient>>& connectedServers() const { return connectedServersVector; }
    const std::vector<Connection<ServerSession>>& connectedClients() const { return connectedClientsVector; }

    const void parseClientRequest(const P2PMessage& message, const std::shared_ptr<ServerSession> &connInfo);
    const void parseServerAnswer(const P2PMessage& message, const std::shared_ptr<P2PClient> &connInfo);

    const void broadcastTx(const Tx::Base &tx) const;

};

#endif