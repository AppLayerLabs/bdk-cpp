#ifndef _ABCINETSERVER_H_
#define _ABCINETSERVER_H_

#include <boost/asio.hpp>

class ABCINetSession;

class ABCIHandler;

/**
 * ABCINetServer implements a boost::asio stream socket server that accepts
 * ABCI stream socket connection requests from cometbft and instantiates
 * an ABCINetSession to handle it.
 * It is given an ABCIHandler instance which actually handles the callbacks
 * received from cometbft.
 */
class ABCINetServer : public std::enable_shared_from_this<ABCINetServer> {
  private:
    const std::string cometUNIXSocketPath_; ///< Pathname for the abci.sock UNIX domain sockets file

    ABCIHandler *handler_; ///< Listener for all ABCI callbacks received by this netserver.

    boost::asio::io_context ioContext_; ///< io_context for this netserver.
    boost::asio::thread_pool threadPool_; ///< thread pool for this netserver.
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard_; ///< Work guard for the io_context.

    boost::asio::local::stream_protocol::acceptor acceptor_; ///< UNIX listen socket connection acceptor.

    std::mutex sessionsMutex_; ///< Serialize access to sessions_ (also protects acceptor_ so we don't need an acceptor strand).
    std::vector<std::weak_ptr<ABCINetSession>> sessions_; ///< So we can explicitly close all sockets on stop().

    std::atomic<bool> started_; ///< If net engine was ever started.
    std::atomic<bool> stopped_; ///< If net engine was ever stopped (cannot be restarted).

    void do_accept(); ///< Accepts another incoming ABCI stream connection through acceptor_ (net handler).

  public:

    /**
     * Creates an ABCI stream socket net server for cometbft to connect to.
     * @param handler The application object that actually handles ABCI requests and provides an ABCI response.
     * @param cometUNIXSocketPath UNIX socket file path to listen for incoming stream connections.
     */
    ABCINetServer(ABCIHandler *handler, const std::string &cometUNIXSocketPath);

    /**
     * Start the ABCI net engine (only once).
     */
    void start();

    /**
     * Stop the ABCI net engine (cannot be restarted).
     */
    void stop(const std::string &reason);

    /**
     * Check if the net server is running.
     */
    bool running();
};

#endif