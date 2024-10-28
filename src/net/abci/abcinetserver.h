#ifndef _ABCINETSERVER_H_
#define _ABCINETSERVER_H_

#include <iostream>
#include <string>
#include <set>
#include <memory>
#include <boost/asio.hpp>


/**
 * TODO: 
 * 
 * need to heavily redesign this ABCI Server / Session implementation.
 * 
 * need to take a pointer/ref to a callback interface which actually implements
 *   handling the ABCI calls from the running cometbft process (which is the
 *   ABCI client to this server). this callback interface should be implemented
 *   by CometImpl, which is the reference/pointer we give to the ABCINetServer object.
 * 
 *   since it is the ABCISession that actually gets called back, we need to
 *   pass the CometImpl pointer (as a ABCIHandler implementor/interface) to
 *   the ABCISession objects as well, probably that's what we'll do here.
 * 
 * io_context should be an implementation detail of the ABCINetServer class.
 * 
 * ABCINetServer should cleanly start and stop and hide all networking details
 * within it. if needed, we should create another top level class to wrap 
 * everything up like we did in the net/p2p engine.
 * 
 * 
 */

namespace asio = boost::asio;
using boost::asio::local::stream_protocol;

const uint64_t MAX_MESSAGE_SIZE = 64 * 1024 * 1024; // 64 MB limit

class ABCINetSession; // Forward declaration

class ABCIHandler;

/**
 * ABCINetServer implements a boost::asio TCP socket server that accepts
 * ABCI TCP socket connection requests from cometbft and instantiates
 * an ABCINetSession to handle it.
 * It is given an ABCIHandler instance which actually handles the callbacks      gotInitChain_(false), lastBlockHeight_(0)

 * received from cometbft.
 */
class ABCINetServer : public std::enable_shared_from_this<ABCINetServer> {
public:
    ABCINetServer(ABCIHandler* handler, asio::io_context& io_context, const std::string& socket_path);

    void start(); // New method to start accepting connections

    void notify_failure(const std::string& reason);

    bool failed() const;

    const std::string& reason() const;


private:
    void do_accept();

    ABCIHandler* handler_;

    asio::io_context& io_context_;
    stream_protocol::acceptor acceptor_;
    std::set<std::weak_ptr<ABCINetSession>, std::owner_less<std::weak_ptr<ABCINetSession>>> sessions_;
    bool failed_;
    std::string reason_;
};




#endif