#ifndef _ABCISERVER_H_
#define _ABCISERVER_H_

#include <boost/asio.hpp>

class ABCINetServer;

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
    const std::string cometUNIXSocketPath_; ///< Pathname for the abci.sock UNIX domain sockets file
    ABCIHandler* handler_; ///< The object that actually handles ABCI calls from cometbft
    std::mutex stateMutex_; ///< Mutex for start()/stop() state changes
    bool started_ = false; ///< Was the ABCI server ever started
    std::shared_ptr<ABCINetServer> abciNetServer_; ///< Boost ASIO ABCI TCP socket server implementation

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
     * @return `true` if the ABCI socket connections are still being processed, `false` otherwise.
     */
    bool running();
};

#endif