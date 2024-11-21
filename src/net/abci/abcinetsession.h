#ifndef _ABCINETSESSION_H_
#define _ABCINETSESSION_H_

#include <boost/asio.hpp>

class ABCINetServer;

class ABCIHandler;

/**
 * ABCINetSession implements a boost::asio socket connection handler for a
 * server-side accepted ABCI socket connection from cometbft.
 * It receives, from the ABCINetServer, a pointer to an ABCIHandler instance
 * which actually handles the callbacks received from cometbft.
 */
class ABCINetSession : public std::enable_shared_from_this<ABCINetSession> {
  private:
    std::shared_ptr<ABCINetServer> server_; ///< The ASIO-based net engine (stream socket server for all ABCI connections).
    ABCIHandler* handler_; ///< ABCI application callback receiver and handler.

    std::mutex startedMutex_; ///< Mutex for protecting start_ (close_ state transition is protected by strand_).
    bool started_ = false; ///< Set to true once this session is started (it can only be started once).
    std::atomic<bool> closed_ = false; ///< Set to true once this session has closed itself (means the socket is closing or closed).

    boost::asio::local::stream_protocol::socket socket_; ///< Socket object for reading from this established ABCI stream socket connection.
    boost::asio::strand<boost::asio::any_io_executor> strand_; ///< Strand that synchronizes all access to the socket object.

    std::vector<uint8_t> databuf_; ///< Preallocated raw data buffer for in/out ABCI request/response messages (keeps size of largest message ever received or sent).
    uint64_t databuf_message_size_; ///< Size of the message stored in the data buffer, since message size <= buffer size.

    std::vector<uint8_t> varint_buffer_; ///< Write buffer for sending a varint.
    uint8_t varint_byte_; ///< Read buffer for reading one varint byte.
    uint64_t varint_value_; ///< Read buffer for reconstructing the whole varint_value_.
    int varint_shift_; ///< Bit shift counter that is increased for each varint_byte_ received while assembling the varint_value_.

    void do_close(); ///< Closes this session (net handler in strand_).
    void start_read_varint(); ///< Starts reading a new incoming varint (net handler in strand_).
    void start_read_varint_byte(); ///< Starts reading the next byte in the current varint being read (net handler in strand_).
    void handle_read_varint_byte(boost::system::error_code ec, std::size_t length); ///< Processes an incoming byte for the current varint being read (net handler in strand_).
    void handle_read_message_length(bool success, uint64_t msg_len); ///< Incoming varint finished being assembled, must read the incoming message (net handler in strand_).
    void handle_read_message(boost::system::error_code ec, std::size_t length);  ///< Finished reading the incoming message, must parse/process it as an ABCI request (net handler in strand_).
    void do_write_message(); ///< Finished serializing an ABCI response message in the outgoing buffer, must send it (net handler in strand_).
    void handle_write_varint(boost::system::error_code ec, std::size_t length); ///< Finished serializing an outgoing varint value, must send it (net handler in strand_).
    void handle_write_message(boost::system::error_code ec, std::size_t length); ///< Finished serializing an outgoing ABCI response message, must send it(net handler in strand_).
    void process_request(); ///< Deserializes an incoming message into an ABCI resquest, computes an ABCI response and serializes it for sending (net handler in strand_).

  public:

    /**
     * Creates an ABCI session object to handle one ABCI stream socket connection with a cometbft client process and starts reading the first ABCI request.
     * @param handler The application object that actually handles ABCI requests and provides an ABCI response.
     * @param socket The socket object for the newly established stream socket connection.
     * @param server The parent ABCINetServer object that created this session (so it can be notified if this session fails/closes).
     */
    ABCINetSession(ABCIHandler* handler, boost::asio::local::stream_protocol::socket socket, std::shared_ptr<ABCINetServer> server);

    /**
     * Starts reading the first ABCI request (can only start a session once).
     */
    void start();

    /**
     * Closes this ABCI session (if it hasn't yet been closed).
     */
    void close();
};

#endif