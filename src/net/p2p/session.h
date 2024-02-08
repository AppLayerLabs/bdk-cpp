/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef P2P_SESSION_H
#define P2P_SESSION_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <shared_mutex>
#include <functional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/asio/buffer.hpp>

#include "../../utils/utils.h"
#include "../../libs/BS_thread_pool_light.hpp"
#include "encoding.h"

using boost::asio::ip::tcp;
namespace net = boost::asio;  // from <boost/asio.hpp>

namespace P2P {
  /// Forward declaration of the ManagerBase class.
  class ManagerBase;

  /**
  * The session class is the base class for both the client and server sessions.
  * It contains the basic functionality for reading and writing messages to the
  * socket.
  */
  class Session : public std::enable_shared_from_this<Session> {
    protected:
      /// The socket used to communicate with the client.
      net::ip::tcp::socket socket_;

      /// Target IP.
      const net::ip::address address_;

      /// Target Port.
      const unsigned short port_;

      /// Target node ID.
      NodeID nodeId_;

      /// Target server port.
      unsigned short serverPort_;

      /// Target node type.
      NodeType type_;

      /// Indicates which type of connection this session is.
      const ConnectionType connectionType_;

      /// Reference back to the Manager object.
      ManagerBase& manager_;

      /// Reference to the thread pool.
      const std::unique_ptr<BS::thread_pool_light>& threadPool_;

      net::strand<net::any_io_executor> readStrand_; ///< Strand for read operations.
      net::strand<net::any_io_executor> writeStrand_; ///< Strand for write operations.

      std::shared_ptr<Message> inboundMessage_; ///< Pointer to the inbound message.
      std::shared_ptr<const Message> outboundMessage_; ///< Pointer to the outbound message.

      BytesArr<3> inboundHandshake_; ///< Array for the inbound handshake.
      BytesArr<3> outboundHandshake_; ///< Array for the outbound handshake.

      BytesArr<8> inboundHeader_; ///< Array for the inbound header.
      BytesArr<8> outboundHeader_; ///< Array for the outbound header.

      /// Queue and mutex for outgoing messages.
      std::deque<std::shared_ptr<const Message>> outboundMessages_;

      /// Mutex for adding to the queue.
      std::mutex writeQueueMutex_;

      /// Handshake flag
      std::atomic<bool> doneHandshake_ = false;

      /// Flag for whether the session is closed.
      std::atomic<bool> closed_ = false;

      /// CLIENT SPECIFIC FUNCTIONS (CONNECTING TO A SERVER)
      /// Connect to a specific endpoint.
      void do_connect();

      /// Callback for connecting to a specific endpoint.
      void on_connect(boost::system::error_code ec, const net::ip::tcp::endpoint& endpoint);

      /// Functions and respective handlers for session operations.
      /// Write the handshake to the socket.
      void write_handshake();

      /// Read the handshake from the socket.
      void read_handshake(boost::system::error_code ec, std::size_t);

      /// Finish the handshake and decide what to do next (disconnect
      void finish_handshake(boost::system::error_code ec, std::size_t);

      /// Read the header from the socket.
      void do_read_header();

      /// Callback for reading the header.
      void on_read_header(boost::system::error_code ec, std::size_t);

      /// Read the message from the socket.
      void do_read_message(const uint64_t& messageSize);

      /// Callback for reading the message.
      void on_read_message(boost::system::error_code ec, std::size_t);

      /// Write a message to the socket.
      void do_write_header();

      /// Callback for writing the header.
      void on_write_header(boost::system::error_code ec, std::size_t);

      /// Write a message to the socket.
      void do_write_message();

      /// Callback for writing the message.
      void on_write_message(boost::system::error_code ec, std::size_t);

      /// do_close, for closing using the io_context
      void do_close();

      /// Handle an error from the socket.
      bool handle_error(const std::string& func, const boost::system::error_code& ec);

    public:

      /// Construct a session with the given socket. (Used by the server)
      explicit Session(tcp::socket &&socket,
                       ConnectionType connectionType,
                       ManagerBase& manager,
                       const std::unique_ptr<BS::thread_pool_light>& threadPool)
          : socket_(std::move(socket)),
            readStrand_(socket_.get_executor()),
            writeStrand_(socket_.get_executor()),
            manager_(manager),
            threadPool_(threadPool),
            address_(socket_.remote_endpoint().address()),
            port_(socket_.remote_endpoint().port()),
            connectionType_(connectionType)
            {
              if (connectionType == ConnectionType::OUTBOUND) {
                /// Not a server, it will not call do_connect().
                throw DynamicException("Session: Invalid connection type.");
              }
            }

      /// Construct a session with the given socket (Used by the client)
      explicit Session(tcp::socket &&socket,
                       ConnectionType connectionType,
                       ManagerBase& manager,
                       const std::unique_ptr<BS::thread_pool_light>& threadPool,
                       const net::ip::address& address,
                       unsigned short port
                       )
          : socket_(std::move(socket)),
            readStrand_(socket_.get_executor()),
            writeStrand_(socket_.get_executor()),
            manager_(manager),
            threadPool_(threadPool),
            address_(address),
            port_(port),
            connectionType_(connectionType)
      {
        if (connectionType == ConnectionType::INBOUND) {
          /// Not a client, it will try to write handshake without connecting.
          throw DynamicException("Session: Invalid connection type.");
        }
      }

      /// Max message size
      const uint64_t maxMessageSize_ = 1024 * 1024 * 128; // (128 MB)

      /// Function for running the session.
      void run();

      /// Function for closing the session.
      void close();

      /// Function for writing a message to the socket.
      void write(const std::shared_ptr<const Message>& message);

      /// Check if the session is closed.
      inline bool isDisconnected() const { return !socket_.is_open(); }

      /// Getter for `address_`.
      const net::ip::address& address() const { return this->address_; }

      /// Getter for `port_`.
      const unsigned short& port() const { return port_; }

      /// Getter for `address_` and `port_`, in form of a pair.
      const std::pair<net::ip::address, unsigned short> addressAndPort() const {
        return std::make_pair(this->address_, this->port_);
      }

      /// Getter for `hostNodeId_`.
      const NodeID& hostNodeId() const { return this->nodeId_; }

      /// Getter for `connectionType_`.
      const ConnectionType& connectionType() const { return connectionType_; }

      /// Getter for `hostType_`.
      const NodeType& hostType() const { return this->type_; }

      /// Getter for `hostServerPort_`.
      const unsigned short& hostServerPort() const { return this->port_; }

      /// Getter for `doneHandshake_`.
      const std::atomic<bool>& doneHandshake() const { return this->doneHandshake_; }
  };
}

#endif  // P2P_SESSION_H
