#include "session.h"

namespace P2P {
  /**
   * ServerListener class
   * This class has the purpose of opening a tcp socket and listening for incoming connections.
   * Creating a new ServerSession for each connection.
   */
  class ServerListener : public std::enable_shared_from_this<ServerListener> {
    private:
      /// Reference to server io_context.
      net::io_context& io_context_;
      /// Server Acceptor
      net::ip::tcp::acceptor acceptor_;
      /// Accept a incoming connection.
      void do_accept();
      /// Callback for do_accept();
      void on_accept(boost::system::error_code ec, net::ip::tcp::socket socket);
      /// Pointer back to the Manager object.
      ManagerBase& manager_;
      /// Reference to the thread pool.
      const std::unique_ptr<BS::thread_pool_light>& threadPool_;
    public:
      /**
       * Constructor.
       * @param ioc Reference to the core I/O functionality object.
       * @param endpoint The endpoint (host and port) to listen to.
       */
      ServerListener(net::io_context& io_context,
                     tcp::endpoint endpoint,
                     ManagerBase& manager,
                     const std::unique_ptr<BS::thread_pool_light> &threadPool) :
        io_context_(io_context),
        acceptor_(io_context),
        manager_(manager),
        threadPool_(threadPool) {
         boost::system::error_code ec;
         acceptor_.open(endpoint.protocol(), ec); // Open the acceptor
         if (ec) { Utils::logToDebug(Log::P2PServerListener, __func__, "Open Acceptor: " + ec.message()); return; }
         acceptor_.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
         if (ec) { Utils::logToDebug(Log::P2PServerListener, __func__, "Set Option: " + ec.message()); return; }
         acceptor_.bind(endpoint, ec); // Bind to the server address
         if (ec) { Utils::logToDebug(Log::P2PServerListener, __func__, "Bind Acceptor: " + ec.message()); return; }
         acceptor_.listen(net::socket_base::max_listen_connections, ec); // Start listening
         if (ec) { Utils::logToDebug(Log::P2PServerListener, __func__, "Listen Acceptor: " + ec.message()); return; }
      }

      void run();   ///< Start accepting incoming connections.
      void stop();  ///< Stop accepting incoming connections.
  };

  class Server {
    private:
      /// io_context for the server.
      net::io_context io_context_;
      /// The server ip address.
      const net::ip::address localAddress_;
      /// The server port.
      const uint16_t localPort_;
      /// Thread count
      const uint8_t threadCount_;
      /// The server listener.
      std::shared_ptr<ServerListener> listener_;

      /// future for the server thread.
      std::future<bool> executor;

      /// Function for running the server thread.
      bool run();

      /// Pointer to the manager.
      ManagerBase& manager_;

      /// Reference to the thread pool.
      const std::unique_ptr<BS::thread_pool_light>& threadPool_;

    public:
      Server(const net::ip::address &localAddress,
             const uint16_t &localPort,
             const uint8_t& threadCount,
             ManagerBase& manager,
             const std::unique_ptr<BS::thread_pool_light>& threadPool) :
        localAddress_(localAddress),
        localPort_(localPort),
        threadCount_(threadCount),
        manager_(manager),
        threadPool_(threadPool)
        {}

      /// Start the Server.
      bool start();

      /// Stop the server.
      bool stop();

      /// Check if the server is running.
      bool isRunning();

      /// Get the server ip address.
      net::ip::address getLocalAddress() { return localAddress_; }
  };
}