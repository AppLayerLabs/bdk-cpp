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

/// Info about a given connected node.
struct ConnectionInfo {
  /// Version of the endpoint's blockchain.
  uint64_t version = 0;

  /// Epoch timestamp of the endpoint.
  uint64_t timestamp = 0;

  /// Most recent block height of the endpoint.
  uint64_t latestBlockHeight = 0;

  /// Most recent block hash of the endpoint.
  Hash latestBlockHash = Hash();

  /// Number of nodes that the endpoint is connected to.
  uint64_t nodes = 0;

  /// Last timestamp when info was queried about the endpoint.
  uint64_t lastNodeCheck = 0;

  /// Clock difference between the node and the endpoint (approximate).
  uint64_t clockDiff = 0;

  /// Copy assignment operator.
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

/**
 * Helper class for abstracting connections from both client and server nodes.
 * The logic here is *to whom the connection is being made*, not who's making it.
 * Ideally this would be:
 * - Connection<P2PServerSession> for *client* connections
 * - Connection<P2PClient> for *server* connections
 */
template <typename T> class Connection {
  private:
    /// Struct with info about the connection.
    ConnectionInfo info;

    /// The address of the connection's endpoint.
    const boost::asio::ip::address host;

    /// The port of the connection's endpoint.
    const unsigned short port;

    /// Pointer to the connection's endpoint object.
    const shared_ptr<T> session;

  public:
    /**
     * Constructor.
     * @param host The endpoint address.
     * @param port The endpoint port.
     * @param session Pointer to the endpoint object.
     */
    Connection(
      const boost::asio::ip::address& host,
      const unsigned short& port,
      const std::shared_ptr<T>& session
    ) : host(host), port(port), session(session);

    /// Getter for `info`.
    const ConnectionInfo& getInfo() { return this->info; }

    /// Getter for `host`.
    const boost::asio::ip::address& getHost() { return this->host; }

    /// Getter for `port`.
    const unsigned short& getPort() { return this->port; }

    /// Getter for `session`.
    const std::shared_ptr<T>& getSession() { return this->session; }

    /// Setter for `info`.
    void setInfo(const ConnectionInfo& info) { this->info = info; }

    /**
     * Equality operator.
     * Checks both IPs and ports, as connections should be unique per IP/port combo.
     */
    bool operator==(const Connection<T>& other) {
      return (address == other.address && port == other.port)
    }
};

/**
 * Class that holds all P2P logic (client and server).
 * Responsible for managing the connections to and from the node.
 */
class P2PManager : public std::enable_shared_from_this<P2PManager> {
  private:
    /// List of connections from servers to clients.
    std::vector<Connection<P2PClient>> connServers;

    /// List of connections from clients to servers.
    std::vector<Connection<P2PServerSession> connClients;

    /// Number of current connections.
    uint64_t connCt = 0;

    /// Mutex for managing read/write access to the connected servers list.
    std::mutex connServersLock;

    /// Mutex for managing read/write access to the connected clients list.
    std::mutex connClientsLock;

    /// Mutex for managing read/write access to the connections counter.
    std::mutex connCtLock;

    /// Host address of the P2P server.
    const boost::asio::ip::address serverHost;

    /// Port of the P2P server.
    const unsigned short serverPort;

    /// Number of threads for the P2P server.
    const unsigned int serverThreads;

    /// Pointer to the P2P server.
    const std::shared_ptr<P2PServer> server;

    /// Pointer to the blockchain history.
    const std::shared_ptr<Storage> storage;

    /// Reference to the blockchain.
    Blockchain& blockchain;

  public;
   /**
    * Constructor.
    * @param serverHost The server's host address.
    * @param serverPort The server's port.
    * @param serverThreads The server's threads.
    * @param storage Pointer to the blockchain history.
    * @param blockchain Reference to the blockchain.
    */
    P2PManager(
      const boost::asio::ip::address& serverHost, const unsigned short& serverPort,
      const unsigned int& serverThreads, const std::shared_ptr<Storage> storage,
      Blockchain& blockchain
    ) : serverHost(serverHost), serverPort(serverPort), serverThreads(serverThreads),
        storage(storage), blockchain(blockchain)
    {}

    /// Getter for `connServers`.
    const std::vector<Connection<P2PClient>>& getConnServers() { return this->connServers; }

    /// Getter for `connClients`.
    const std::vector<Connection<P2PServerSession>>& getConnClients() { return this->connClients; }

    /// Getter for `connCt`.
    const uint64_t& getConnCt() { return this->connCt; }

    /// Start the P2P server.
    void startServer();

    /**
     * Add a new client connection to the list.
     * @param conn The connection to add.
     */
    void addClient(const Connection<P2PServerSession> conn);

    /**
     * Remove a client connection from the list.
     * @param conn The connection to add.
     */
    void removeClient(const Connection<P2PServerSession> conn);

    /**
     * Connect to a new server and add it to the list.
     * @param host The server's address.
     * @param port The server's port.
     */
    void connectToServer(const boost::asio::ip::address& host, const unsigned short& port)

    /**
     * Disconnect from a server and remove it from the list.
     * @param conn The connection to remove.
     */
    void disconnectFromServer(const Connection<P2PClient> conn);

    /**
     * Parse a message from a client connection.
     * @param msg The message to parse.
     * @param conn The client connection that sent the message so an answer can be sent to it.
     */
    void parseClientMsg(const P2PMsg& msg, const shared_ptr<P2PServerSession>& conn);

    /**
     * Parse a message from a server connection.
     * @param msg The message to parse.
     * @param conn The server connection that sent the message so an answer can be sent to it.
     */
    void parseServerMsg(const P2PMsg& msg, const shared_ptr<P2PClient>& conn);

    /**
     * Broadcast a block transaction to the network.
     * @param tx The transaction to broadcast.
     */
    void broadcastTx(const TxBlock& tx);

    /**
     * Broadcast a Validator transaction to the network.
     * @param tx The transaction to broadcast.
     */
    void broadcastValidatorTx(const TxValidator& tx);

    /// Request Validator transactions from all connected nodes.
    void requestValidatorTxsToAll();
};

#endif  // P2PMANAGER_H
