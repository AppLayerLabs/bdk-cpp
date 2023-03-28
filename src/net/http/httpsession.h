#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include "httpparser.h"

// Forward declarations.
class HTTPSession;  // HTTPQueue depends on HTTPSession and vice-versa
class State;
class Storage;
namespace P2P {
  class ManagerNormal;
}

/**
 * Helper class used for HTTP pipelining.
 * TODO: explain better what this is and what it's used for.
 */
class HTTPQueue {
  private:
    /// Maximum number of responses to queue.
    unsigned int limit = 8;

    /// Type-erased, saved work item. TODO: what is this?
    struct work { virtual ~work() = default; virtual void operator()() = 0; };

    /// Reference to the HTTP session that is handling the queue.
    HTTPSession& session;

    /// Array of pointers to work structs.
    std::vector<std::unique_ptr<work>> items;

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
     * TODO: same as `handle_request()` - also why does this have a struct inside it, why does it look similar to the work struct and why is it needed here?
     */
    template<bool isRequest, class Body, class Fields> void operator()(
      http::message<isRequest, Body, Fields>&& msg
    );
};

/// Helper class that handles an HTTP connection session.
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

    /// Reference to the state.
    const std::unique_ptr<State>& state;

    /// Refence to the storage.
    const std::unique_ptr<Storage>& storage;

    /// Reference to the P2P manager.
    const std::unique_ptr<P2P::ManagerNormal>& p2p;

    /// Read whatever is on the internal buffer.
    void do_read();

    /**
     * Callback to handle what is read from the internal buffer.
     * Also tries to pipeline another request if the queue isn't full.
     * @param ec The error code to parse.
     * @param bytes The number of read bytes.
     */
    void on_read(beast::error_code ec, std::size_t bytes);

    /**
     * Callback to handle what is written to the internal buffer.
     * Also automatically reads another request.
     * @param ec The error code to parse.
     * @param bytes The number of written bytes.
     */
    void on_write(bool close, beast::error_code ec, std::size_t bytes);

    /// Send a TCP shutdown and close the connection.
    void do_close();

  public:
    /**
     * Constructor.
     * @param sock The socket to take ownership or.
     * @param docroot Pointer to the root directory of the endpoint.
     * @param unique_ptr reference to the state.
     */
    HTTPSession(
      tcp::socket&& sock,
      std::shared_ptr<const std::string>& docroot,
      const std::unique_ptr<State>& state,
      const std::unique_ptr<Storage>& storage,
      const std::unique_ptr<P2P::ManagerNormal>& p2p
    ) : stream(std::move(sock)), docroot(docroot), queue(*this), state(state), storage(storage), p2p(p2p)
    {}

    /// Start the HTTP session.
    void start();
    
    friend class HTTPQueue;
};

#endif  // HTTPSESSION_H
