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
    std::shared_ptr<ABCINetServer> server_;
    ABCIHandler* handler_;

    std::atomic<bool> closed_ = false;

    boost::asio::local::stream_protocol::socket socket_;
    boost::asio::strand<boost::asio::any_io_executor> strand_; ///< Strand that synchronizes all access to the socket object.

    std::vector<uint8_t> varint_buffer_;

    std::vector<uint8_t> message_data_;
    std::vector<uint8_t> response_data_;
    uint8_t varint_byte_;
    uint64_t varint_value_;
    int varint_shift_;

    void do_close();
    void start_read_varint();
    void start_read_varint_byte();
    void handle_read_varint_byte(boost::system::error_code ec, std::size_t length);
    void handle_read_message_length(bool success, uint64_t msg_len);
    void handle_read_message(boost::system::error_code ec, std::size_t length);
    void do_write_message();
    void handle_write_varint(boost::system::error_code ec, std::size_t length);
    void handle_write_message(boost::system::error_code ec, std::size_t);
    void write_varint(uint64_t value, std::vector<uint8_t> &buffer);
    void process_request();

  public:
    ABCINetSession(ABCIHandler* handler, boost::asio::local::stream_protocol::socket socket, std::shared_ptr<ABCINetServer> server);

    void start();

    void close();
};

#endif