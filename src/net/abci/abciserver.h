#ifndef _ABCISERVER_H_
#define _ABCISERVER_H_

#include <thread>
#include <boost/asio.hpp>

#include <google/protobuf/message.h>

//#include "abcinetserver.h"
class ABCINetServer;
//#include "abcihandler.h"
class ABCIHandler;

/**
 * The ABCIServer class is the public interface of the net/abci component.
 * It starts and stops the ABCI Socket server, and holds the io_context and
 * the thread_pool that runs it.
 * It is given a pointer to an ABCIHandler implementation from the caller
 * that will actually handle the ABCI requests that cometbft makes.
 * 
 * FIXME: Need to review and refactor all of the boost ASIO net code in this
 *        class and in the ABCINetSession class as well.
 */
class ABCIServer {
  private:
    std::string cometUNIXSocketPath_; ///< Pathname for the abci.sock UNIX domain sockets file

    ABCIHandler* handler_; ///< The object that actually handles ABCI calls from cometbft

    // This was for GRPC, right? so no longer needed
    //std::unique_ptr<cometbft::abci::v1::ABCIService::Service> abciService_; ///< Pointer to the ABCI interface impl

    std::atomic<bool> started_ = false; ///< Was the ABCI server ever started
    std::atomic<bool> running_ = false; ///< Is the ABCI server actually running now (i.e. the thread pool running the io_context did not terminate)

    std::shared_ptr<ABCINetServer> abciNetServer_; ///< Boost ASIO ABCI TCP socket server implementation
    std::unique_ptr<boost::asio::io_context> ioContext_; ///< Boost ASIO io_context that runs the ABCI TCP socket server
    std::unique_ptr<boost::asio::thread_pool> threadPool_; ///< Boost ASIO threadpool that runs ioContext.run()

    std::unique_ptr<std::thread> runThread_; ///< Thread that runs the ABCI server and joins the thread pool

    void run(); ///< Dedicated thread that runs and then joins with the ABCI Server threads.

public:
  /**
   * Create an ABCIServer that will issue allbacks to the provided ABCIHandler interface
   * for each ABCI request received from comebft.
   * @param handler Pointer to the ABCIHandler instance that will handle ABCI requests.
   * @param cometUNIXSocketPath Path for the unix:// domain socket that cometbft will connect to.
   */
  ABCIServer(ABCIHandler* handler, const std::string& cometUNIXSocketPath = "/tmp/abci.sock");

  /**
   * Destroy this ABCIServer instance; calls stop() to ensure the server is stopped.
   */
  virtual ~ABCIServer();

  /**
   * Get the configured UNIX socket path.
   */
  const std::string getSocketPath(); 

  /**
   * Starts the ABCIServer. Does nothing if already started.
   * This must be called before the cometbft process is started, since it will immediately
   * attempt to connect to the BDK node.
   * @return `true` if the server was started, `false` if it was already started.
   */
  bool start();

  /**
   * Stops the ABCIServer. Does nothing if already stopped.
   * Even if the cometbft process hsa already terminated (the client), must call stop() 
   * eventually to shut down the threading/networking objects on our side.
   * @return `true` if the server was stopped, `false` if it was already stopped.
   */
  bool stop();

  /**
   * Checks if the ABCIServer is (still) running.
   */
  bool running();
};

#endif