#ifndef P2PCLIENT_H
#define P2PCLIENT_H

#include <memory>
#include <mutex>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "p2pencoding.h"
#include "p2pmanager.h"
#include "../utils/utils.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace tcp = boost::asio::ip::tcp;

/// Abstraction of the client side of a P2P connection.
class P2PClient : public std::enable_shared_from_this<P2PClient> {
  private:
    /// Pointer to the P2P manager.
    const std::shared_ptr<P2PManager> mgr;

    /// TCP resolver for connecting to other endpoints.
    tcp::resolver resolver;

    /// Websocket object that maintains the connection and data transfers.
    websocket::stream<beast::tcp_stream> ws;

    /// Data buffer for read and write operations.
    beast::flat_buffer buf;

    /// Mutex for managing read/write access to the buffer during a write operation.
    std::mutex writeLock;

  public:
    /// The host address where the client will connect to.
    const std::string host;

    /// The port where the client will connect to.
    const unsigned short port;

    /**
     * Constructor.
     * @param ioc The I/O context object to strand for the resolver.
     * @param host The host address to connect to.
     * @param port The port to connect to.
     * @param mgr Pointer to the P2P manager.
     */
    P2PClient(
      net::io_context& ioc, const std::string& host,
      const unsigned short& port, const std::shared_ptr<P2PManager>& mgr
    ) : resolver(net::make_strand(ioc)), ws(net::make_strand(ioc)),
      host(host), port(port), mgr(mgr)
    { ws.binary(true); }

    /// Start a resolve operation.
    void start();

    /// Stop. TODO: for what? Is there even a need to?
    void stop();

    /// Attempt to resolve the endpoint.
    void resolve();

    /**
     * Callback for the resolve operation.
     * Automatically calls `connect()`.
     * @param ec The error code to parse.
     * @param results The results of the resolve operation.
     */
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

    /**
     * Attempt to connect to the endpoint.
     * @param results The results of the resolve operation.
     */
    void connect(tcp::resolver::results_type& results);

    /**
     * Callback for the connect operation.
     * Automatically calls `handshake()`.
     * @param ec The error code to parse.
     * @param ep The endpoint successfully connected to.
     */
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

    /**
     * Give a handshake to the endpoint.
     * @param host The full endpoint ("host:port") to give a handshake to.
     */
    void handshake(const std::string& host);

    /**
     * Callback for the handshake operation.
     * Automatically calls `read()`.
     * @param ec The error code to parse.
     */
    void on_handshake(beast::error_code ec);

    /**
     * Read a message from the buffer.
     * Keeps on listening asynchronously until the message arrives from the endpoint.
     */
    void read();

    /**
     * Callback for the read operation (after a message is successfully read).
     * Automatically parses the message, calls `write()` to answer it, and
     * calls `read()` again to keep on listening to the next message.
     * @param ec The error code to parse.
     * @param bytes The number of bytes that were read.
     */
    void on_read(beast::error_code ec, std::size_t bytes);

    /**
     * Write a message to the buffer and send it to the endpoint.
     * Also locks the buffer.
     * @param data The data string to write and send.
     */
    void write(const P2PMessage data);

    /**
     * Callback for the write operation (after a message is successfully written).
     * Also unlocks the buffer.
     * @param ec The error code to parse.
     * @param bytes The number of bytes that were written.
     */
    void on_write(beast::error_code ec, std::size_t bytes);
};

#endif  // P2PCLIENT_H
