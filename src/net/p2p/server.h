/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

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
      BS::thread_pool_light& threadPool_;
    public:
      /**
       * Constructor for the ServerListener.
       * @param io_context Reference to the server io_context.
       * @param endpoint The endpoint to listen on.
       * @param manager Reference to the manager.
       * @param threadPool Reference to the thread pool.
       */
      ServerListener(net::io_context& io_context,
                     tcp::endpoint endpoint,
                     ManagerBase& manager,
                     BS::thread_pool_light& threadPool) :
        io_context_(io_context),
        acceptor_(io_context),
        manager_(manager),
        threadPool_(threadPool) {
         boost::system::error_code ec;
         acceptor_.open(endpoint.protocol(), ec); // Open the acceptor
         if (ec) { Logger::logToDebug(LogType::ERROR, Log::P2PServerListener, __func__, "Open Acceptor: " + ec.message()); return; }
         acceptor_.set_option(net::socket_base::reuse_address(true), ec); // Allow address reuse
         if (ec) { Logger::logToDebug(LogType::ERROR, Log::P2PServerListener, __func__, "Set Option: " + ec.message()); return; }
         acceptor_.bind(endpoint, ec); // Bind to the server address
         if (ec) { Logger::logToDebug(LogType::ERROR, Log::P2PServerListener, __func__, "Bind Acceptor: " + ec.message()); return; }
         acceptor_.listen(net::socket_base::max_listen_connections, ec); // Start listening
         if (ec) { Logger::logToDebug(LogType::ERROR, Log::P2PServerListener, __func__, "Listen Acceptor: " + ec.message()); return; }
      }

      void run();   ///< Start accepting incoming connections.
      void stop();  ///< Stop accepting incoming connections.
  };

  /**
  * Server class
  * This class has the purpose of opening a tcp socket and listening for incoming connections.
  * Creating a new ServerSession for each connection.
  */
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
      std::future<bool> executor_;

      /// Function for running the server thread.
      bool run();

      /// Pointer to the manager.
      ManagerBase& manager_;

      /// Reference to the thread pool.
      BS::thread_pool_light& threadPool_;

    public:
      /**
      * Constructor for the server.
      * @param localAddress Reference to the local address.
      * @param localPort The local port.
      * @param threadCount Reference to the thread count.
      * @param manager Reference to the manager.
      * @param threadPool Reference to the thread pool.
      */
      Server(const net::ip::address &localAddress,
             const uint16_t &localPort,
             const uint8_t& threadCount,
             ManagerBase& manager,
             BS::thread_pool_light& threadPool) :
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
      bool isRunning() const;

      /// Get the server ip address.
      net::ip::address getLocalAddress() const { return localAddress_; }
  };
}

