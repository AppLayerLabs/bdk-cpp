#ifndef P2PSERVER_H
#define P2PSERVER_H

#include <iostream>
#include <thread>
#include <future>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/lexical_cast.hpp>

#include "../../libs/BS_thread_pool_light.hpp"
#include "../../utils/utils.h"

#include "p2pbase.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace P2P {
  // Forward declarations.
  class ManagerBase;

  /// Abstraction of a server-specific connection/session.
  class ServerSession : public BaseSession, public std::enable_shared_from_this<ServerSession> {
    private:
      // TODO: change this to timed mutex, so that a thread doesn't lock forever.
      /// Mutex that manages read/write access to the write/answer buffer.
      std::mutex writeLock_;

      /// Read-specific buffer.
      beast::flat_buffer buffer_;

      /// Write-specific buffer.
      beast::flat_buffer answerBuffer_;

      /// Object that defines the type of the response.
      http::request<http::string_body> upgrade_request_;

    public:
      /**
       * Constructor.
       * @param socket The socket to take ownership of.
       * @param manager Reference to the parent connection manager.
       * @param threadPool Reference pointer to the thread pool.
       */
      ServerSession(
        tcp::socket&& socket, ManagerBase& manager,
        const std::unique_ptr<BS::thread_pool_light>& threadPool
      ) : BaseSession(std::move(socket), manager, ConnectionType::SERVER, threadPool)
      {}

      /// Startup the server. Implementation overriden from BaseSession.
      void run() override;

      /// Shutdown the server. Implementation overriden from BaseSession.
      void stop() override;

      /// Callback for the startup operation. Accepts an incoming handshake.
      void on_run();

      /**
       * Callback for when a handshake is accepted. Finishes setting up the server.
       * @param ec The error code to parse if necessary.
       * @param bytes_transferred The number of bytes transferred in the operation.
       */
      void accept(beast::error_code ec, std::size_t bytes_transferred);

      /**
       * Callback for accept(). Calls read().
       * @param ec The error code to parse if necessary.
       */
      void on_accept(beast::error_code ec);

      /// Do a read operation. Implementation overriden from BaseSession.
      void read() override;

      /**
       * Callback for read operation. Implementation overriden from BaseSession.
       * @param ec The error code to parse if necessary.
       * @param bytes_transferred The number of bytes transferred in the operation.
       */
      void on_read(beast::error_code ec, std::size_t bytes_transferred) override;

      /// Do a write operation. Implementation overriden from BaseSession.
      void write(const Message& message) override;

      /**
       * Callback for write operation. Implementation overriden from BaseSession.
       * @param ec The error code to parse if necessary.
       * @param bytes_transferred The number of bytes transferred in the operation.
       */
      void on_write(beast::error_code ec, std::size_t bytes_transferred) override;

      /// Close the websocket connection. Implementation overriden from BaseSession.
      void close() override;

      /**
       * Callback for close operation. Implementation overriden from BaseSession.
       * @param ec The error code to parse if necessary.
       */
      void on_close(beast::error_code ec) override;

      /**
       * Handle an error and close the connection.
       * @param func The name of the function where the error occurred.
       * @param ec The error code object to parse.
       */
      void handleError(const std::string& func, const beast::error_code& ec);
  };

  /// Abstraction of a P2P server.
  class Server : public std::enable_shared_from_this<Server>  {
    /// Class for listening to, accepting and dispatching incoming connections/sessions.
    class listener : public std::enable_shared_from_this<listener> {
      private:
        /// Provides core I/O functionality.
        net::io_context& ioc_;

        /// Accepts incoming connections.
        tcp::acceptor acceptor_;

        /// Reference to the parent connection manager.
        ManagerBase& manager_;

        /// Reference pointer to the thread pool.
        const std::unique_ptr<BS::thread_pool_light>& threadPool;

        /// Accept an incoming connection from the endpoint. The new connection gets its own strand.
        void accept();

        /**
         * Callback for do_accept().
         * Automatically listens to another session when finished dispatching.
         * @param ec The error code to parse.
         * @param socket The socket to use for creating the HTTP session.
         */
        void on_accept(beast::error_code ec, tcp::socket socket);

      public:
        /**
         * Constructor.
         * @param ioc Reference to the core I/O functionality object.
         * @param endpoint The endpoint (host and port) to listen to.
         * @param manager Reference to the parent connection manager.
         * @param threadPool Reference pointer to the thread pool.
         */
        listener(
          net::io_context& ioc, tcp::endpoint endpoint, ManagerBase& manager,
          const std::unique_ptr<BS::thread_pool_light>& threadPool
        ) : ioc_(ioc), acceptor_(ioc), manager_(manager), threadPool(threadPool) {
          beast::error_code ec;
          acceptor_.open(endpoint.protocol(), ec); // Open the acceptor
          if (ec) { Utils::logToDebug(Log::P2PServer, __func__, "Open Acceptor: " + ec.message()); return; }
          acceptor_.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
          if (ec) { Utils::logToDebug(Log::P2PServer, __func__, "Set Option: " + ec.message()); return; }
          acceptor_.bind(endpoint, ec); // Bind to the server address
          if (ec) { Utils::logToDebug(Log::P2PServer, __func__, "Bind Acceptor: " + ec.message()); return; }
          acceptor_.listen(net::socket_base::max_listen_connections, ec); // Start listening
          if (ec) { Utils::logToDebug(Log::P2PServer, __func__, "Listen Acceptor: " + ec.message()); return; }
        }

        void run();   ///< Start accepting incoming connections.
        void stop();  ///< Stop accepting incoming connections.
    };

    private:
      /// Reference to the parent connection manager.
      ManagerBase& manager_;

      /// Reference pointer to the thread pool.
      const std::unique_ptr<BS::thread_pool_light>& threadPool;

      /// Provides core I/O functionality.
      net::io_context ioc;

      /// Pointer to the listener.
      std::shared_ptr<listener> listener_;

      /// The address where the server is running.
      const boost::asio::ip::address address;

      /// The port where the server is running.
      const unsigned short port;

      /// The number of threads that the server is using.
      const unsigned int threads;

      /// Future for the run function so we know when it should stop.
      std::future<bool> runFuture;

      /// Run the server.
      bool run();

    public:
      /**
       * Constructor.
       * @param address The server's address.
       * @param port The server's port.
       * @param threads The number of threads the server will use.
       * @param manager Reference to the parent connection manager.
       * @param threadPool Reference pointer to the thread pool.
       */
      Server(
        boost::asio::ip::address address, unsigned short port,
        unsigned int threads, ManagerBase& manager,
        const std::unique_ptr<BS::thread_pool_light>& threadPool
      ) : address(address), port(port), threads(threads), manager_(manager), threadPool(threadPool)
      {};

      bool start(); ///< Start the server.
      void stop();  ///< Stop the server.

      /**
       * Check if the server is currently active and running.
       * @return `true` if the server is running, `false` otherwise.
       */
      bool isRunning() const { return this->runFuture.valid(); }
  };
};

#endif  // P2PSERVER_H
