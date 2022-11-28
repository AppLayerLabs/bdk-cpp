#ifndef P2PMANAGER_H
#define P2PMANAGER_H

#include "P2PClient.h"
#include "P2PServer.h"
#include <memory>
#include <unordered_map>
#include <condition_variable>
#include "../utils/utils.h"

class P2PManager : public std::enable_shared_from_this<P2PManager> {
    std::shared_ptr<P2PServer> server;
    std::shared_ptr<P2PClient> client;
    std::unordered_map<boost::asio::ip::address, std::shared_ptr<P2PClient>, SafeHash> connectedServers;
    std::mutex servers_mutex; // Connected servers.
    std::unordered_map<boost::asio::ip::address, std::shared_ptr<ServerSession>, SafeHash> connectedClients;
    std::mutex clients_mutex; // Connected clients

    const boost::asio::ip::address server_address;
    const unsigned short server_port;
    const unsigned int server_threads;

  public:
    P2PManager(const boost::asio::ip::address &address, const unsigned short &server_port, const unsigned int &server_threads) : server_address(address), server_port(server_port), server_threads(server_threads) {};

    void startServer();
    void parseServerMessage(const std::string& message);
    
    // Insert a new client into the connected Clients.
    void addClient(const boost::asio::ip::address& address, std::shared_ptr<ServerSession> session);
    // Remove the said client from the connected Clients.
    void removeClient(const boost::asio::ip::address& address);

    // Create a new thread running the client and connect to the server.
    void connectToServer(const boost::asio::ip::address& address, const unsigned short& port);
    // Disconect a given client from a server.
    void disconnectFromServer(const boost::asio::ip::address& address);
    
    // TODO: Requesters and parsers.

};

#endif