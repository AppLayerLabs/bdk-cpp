/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "session.h"


namespace P2P {
  /**
   * ClientFactory
   * Creates and manages multiple Client Sessions
   * Previously, OrbiterSDK was creating a new std::thread and io_context for each client session.
   * This is not very efficient, as it creates a lot of overhead.
   * The ClientFactory uses a single io_context and multiple threads to handle multiple sessions.
   * Instead of creating a new std::thread for each connection, a new "connection" task is posted to the io_context.
   * Strands within the factory and client are used to ensure thread safety.
   * ClientFactory doesn't necesarily "own" the client session, it only creates them in a shared manner.
   * Registration/Unregistration is responsibility of the Manager.
   */
  class ClientFactory {
    private:
      /// io_context for the factory.
      net::io_context io_context_;

      /// Work guard for the io_context.
      net::executor_work_guard<net::io_context::executor_type> work_guard_;

      /// Strand for opening new client sessions.
      net::strand<net::any_io_executor> connectorStrand_;

      /// Thread count
      const uint8_t threadCount_;

      /// future for the server thread.
      std::future<bool> executor_;

      /// Function for running the server thread.
      bool run();

      /// Reference to the manager.
      ManagerBase& manager_;

      /// Reference to the thread pool.
      const std::unique_ptr<BS::thread_pool_light>& threadPool_;

      /// Internal function for creating a new client session.
      void createClientSession(const boost::asio::ip::address &address, const unsigned short &port);

    public:

      /**
      * Constructor for the ClientFactory.
      * @param manager Reference to the manager.
      * @param threadCount Number of threads to use.
      * @param threadPool Reference to the thread pool.
      */
      ClientFactory(ManagerBase& manager, const uint8_t &threadCount, const std::unique_ptr<BS::thread_pool_light>& threadPool) :
        work_guard_(boost::asio::make_work_guard(io_context_)),
        connectorStrand_(io_context_.get_executor()),
        threadCount_(threadCount),
        manager_(manager),
        threadPool_(threadPool) {}

      /// Start the Factory.
      bool start();

      /// Stop the Factory.
      bool stop();

      /// Check if the server is running.
      bool isRunning();

      /// Start a new Client Session and connect to a remote host.
      void connectToServer(const boost::asio::ip::address &address, const unsigned short &port);

  };
}
