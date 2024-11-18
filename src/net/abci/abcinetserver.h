#ifndef _ABCINETSERVER_H_
#define _ABCINETSERVER_H_

#include <boost/asio.hpp>

class ABCINetSession;

class ABCIHandler;

/**
 * ABCINetServer implements a boost::asio TCP socket server that accepts
 * ABCI TCP socket connection requests from cometbft and instantiates
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

    std::mutex sessionsMutex_; ///< Serilize access to sessions_.
    std::vector<std::weak_ptr<ABCINetSession>> sessions_; ///< So we can explicitly close all sockets on stop().

    std::atomic<bool> started_; ///< If net engine was ever started.
    std::atomic<bool> stopped_; ///< If net engine was ever stopped (cannot be restarted).

    void do_accept();

  public:
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