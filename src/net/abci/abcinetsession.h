#ifndef _ABCINETSESSION_H_
#define _ABCINETSESSION_H_

#include "abcinetserver.h"

//???
#include "abcihandler.h"

/**
 * ABCINetSession implements a boost::asio socket connection handler for a
 * server-side accepted ABCI socket connection from cometbft.
 * It receives, from the ABCINetServer, a pointer to an ABCIHandler instance
 * which actually handles the callbacks received from cometbft.
 */
class ABCINetSession : public std::enable_shared_from_this<ABCINetSession> {
public:
    ABCINetSession(ABCIHandler* handler, stream_protocol::socket socket, std::shared_ptr<ABCINetServer> server);

    void start();

    void close();

private:
    void do_read_message();
    void process_request();
    void do_write_message();
    void read_varint(std::function<void(bool, uint64_t)> handler);
    void do_read_varint_byte(std::function<void(bool, uint64_t)> handler);
    void write_varint(uint64_t value, std::vector<uint8_t>& buffer);

    ABCIHandler* handler_;

    stream_protocol::socket socket_;
    std::shared_ptr<ABCINetServer> server_;
    std::vector<uint8_t> message_data_;
    std::vector<uint8_t> response_data_;
    uint8_t varint_byte_;
    uint64_t varint_value_;
    int varint_shift_;
};

#endif