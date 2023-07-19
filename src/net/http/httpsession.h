#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include "httpparser.h"

// Forward declarations.
class HTTPSession;  // HTTPQueue depends on HTTPSession and vice-versa
class State;
class Storage;
namespace P2P { class ManagerNormal; }

/// Class used for HTTP pipelining.
class HTTPQueue {
  private:
    /// Type-erased, saved work item.
    struct work {
      virtual ~work() = default;      ///< Default destructor.
      virtual void operator()() = 0;  ///< Default call operator.
    };

    unsigned int limit = 8; ///< Maximum number of responses to queue.
    HTTPSession& session;   ///< Reference to the HTTP session that is handling the queue.
    std::vector<std::unique_ptr<work>> items; ///< Array of pointers to work structs.

  public:
    /**
     * Constructor.
     * @param session Reference to the HTTP session that will handle the queue.
     */
    HTTPQueue(HTTPSession& session);

    /**
     * Check if the queue limit was hit.
     * @return `true` if queue is full, `false` otherwise.
     */
    bool full();

    /**
     * Callback for when a message is sent.
     * @return `true` if the caller should read a message, `false` otherwise.
     */
    bool on_write();

    /**
     * Call operator.
     * Called by the HTTP handler to send a response.
     * @param msg The message to send as a response.
     */
    template<bool isRequest, class Body, class Fields> void operator()(
      http::message<isRequest, Body, Fields>&& msg
    );
};

/// Class that handles an HTTP connection session.
class HTTPSession : public std::enable_shared_from_this<HTTPSession> {
  private:
    /// TCP/IP stream socket.
    beast::tcp_stream stream;

    /// Internal buffer to read and write from.
    beast::flat_buffer buf;

    /// Pointer to the root directory of the endpoint.
    std::shared_ptr<const std::string> docroot;

    /// Queue object that the session is responsible for.
    HTTPQueue queue;

    /**
     * HTTP/1 parser for producing a request message.
     * The parser is stored in an optional container so we can construct it
     * from scratch at the beginning of each new message.
     */
    boost::optional<http::request_parser<http::string_body>> parser;

    /// Reference pointer to the blockchain's state.
    const std::unique_ptr<State>& state;

    /// Reference pointer to the blockchain's storage.
    const std::unique_ptr<Storage>& storage;

    /// Reference pointer to the P2P connection manager.
    const std::unique_ptr<P2P::ManagerNormal>& p2p;

    /// Reference pointer to the options singleton.
    const std::unique_ptr<Options>& options;

    /// Read whatever is on the internal buffer.
    void do_read();

    /**
     * Callback for do_read().
     * Tries to pipeline another request if the queue isn't full.
     * @param ec The error code to parse.
     * @param bytes The number of read bytes.
     */
    void on_read(beast::error_code ec, std::size_t bytes);

    /**
     * Callback for when HTTPQueue writes something.
     * Automatically reads another request.
     * @param close If `true`, calls do_close() at the end.
     * @param ec The error code to parse.
     * @param bytes The number of written bytes.
     */
    void on_write(bool close, beast::error_code ec, std::size_t bytes);

    /// Send a TCP shutdown and close the connection.
    void do_close();

  public:
    /**
     * Constructor.
     * @param sock The socket to take ownership of.
     * @param docroot Reference pointer to the root directory of the endpoint.
     * @param state Reference pointer to the blockchain's state.
     * @param storage Reference pointer to the blockchain's storage.
     * @param p2p Reference pointer to the P2P connection manager.
     * @param options Reference pointer to the options singleton.
     */
    HTTPSession(
      tcp::socket&& sock, std::shared_ptr<const std::string>& docroot,
      const std::unique_ptr<State>& state,
      const std::unique_ptr<Storage>& storage,
      const std::unique_ptr<P2P::ManagerNormal>& p2p,
      const std::unique_ptr<Options>& options
    ) : stream(std::move(sock)), docroot(docroot), queue(*this), state(state),
    storage(storage), p2p(p2p), options(options)
    {}

    /// Start the HTTP session.
    void start();

    friend class HTTPQueue;
};

#endif  // HTTPSESSION_H
