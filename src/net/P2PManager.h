#ifndef P2PMANAGER_H
#define P2PMANAGER_H

#include "P2PClient.h"
#include "P2PServer.h"
#include <memory>
#include <unordered_map>
#include <condition_variable>
#include "../utils/utils.h"

template<typename T>
struct ConnectionInfo {
  const std::shared_ptr<T> session;
  const boost::asio::ip::address address;
  const unsigned short port;
  ConnectionInfo(const boost::asio::ip::address &_address, const unsigned short &_port, const std::shared_ptr<T> &_session) : 
    address(_address), port(_port), session(_session) {}

  // Clock difference between our node and the node we are connected to.
  int64_t clockDiff = 0; 
  // Quantity of nodes that they are connected to.
  uint64_t connectedNodes = 0;
  // Their best block hash.
  Hash bestBlockHash = Hash();
  // Last time we checked their info.
  uint64_t lastTimeUpdates = 0;
};


class P2PManager : public std::enable_shared_from_this<P2PManager> {
    std::shared_ptr<P2PServer> server;
    std::vector<ConnectionInfo<P2PClient>> connectedServersVector;
    std::mutex servers_mutex; // Connected servers.
    std::vector<ConnectionInfo<ServerSession>> connectedClientsVector;
    std::mutex clients_mutex; // Connected clients

    const boost::asio::ip::address server_address;
    const unsigned short server_port;
    const unsigned int server_threads;

  public:
    P2PManager(const boost::asio::ip::address &address, const unsigned short &server_port, const unsigned int &server_threads) : server_address(address), server_port(server_port), server_threads(server_threads) {};

    void startServer();
    void parseServerMessage(const std::string& message);
    
    // Insert a new client into the connected Clients.
    void addClient(const ConnectionInfo<ServerSession> &connInfo);
    // Remove the said client from the connected Clients.
    void removeClient(const ConnectionInfo<ServerSession> &connInfo);

    // Create a new thread running the client and connect to the server.
    void connectToServer(const boost::asio::ip::address &address, const unsigned short &port);
    // Disconect a given client from a server.
    void disconnectFromServer(const ConnectionInfo<P2PClient> &address);


    const std::vector<ConnectionInfo<P2PClient>>& connectedServers() const { return connectedServersVector; }
    const std::vector<ConnectionInfo<ServerSession>>& connectedClients() const { return connectedClientsVector; }
    // TODO: Requesters and parsers.

};

#endif