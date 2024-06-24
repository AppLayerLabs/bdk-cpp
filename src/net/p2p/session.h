/*
Copyright (c) [2023-2024] [AppLayer Developers]

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
#include "encoding.h"

using boost::asio::ip::tcp;
namespace net = boost::asio;  // from <boost/asio.hpp>

namespace P2P {
  /// Forward declaration of the ManagerBase class.
  class ManagerBase;

  /**
   * The session class is the base class for both client and server connections.
   * It contains the basic functionality for reading and writing messages to the socket.
   */
  class Session : public std::enable_shared_from_this<Session>, public Log::LogicalLocationProvider {
    private:
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

      /// Set to `true` when `socket_` is closed.
      std::atomic<bool> closed_ = false;

      /// Reference back to the Manager object.
      ManagerBase& manager_;

      net::strand<net::any_io_executor> strand_; ///< Strand that synchronizes all access to the socket object.

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

      /// Track if this Session is currently registered with the manager
      std::atomic<bool> registered_ = false;

      /// Track if this Session was ever unregistered by the manager
      std::atomic<bool> unregistered_ = false;

      /// Mutex to guard callbacks to ManagerBase vs. internal state transitions (such as registered_)
      std::mutex stateMutex_;

      /// My value for getLogicalLocation() LOG macros
      std::string logSrc_;

      /// Update logSrc_
      void setLogSrc();

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
      void do_close(const std::string& reason);

      /// Handle an error from the socket.
      void handle_error(const std::string& func, const boost::system::error_code& ec);

    public:

      /// Construct a server session with the given socket.
      explicit Session(tcp::socket &&socket,
                       ConnectionType connectionType,
                       ManagerBase& manager);

      /// Construct a client session with the given socket.
      explicit Session(tcp::socket &&socket,
                       ConnectionType connectionType,
                       ManagerBase& manager,
                       const net::ip::address& address,
                       unsigned short port);

      std::string getLogicalLocation() const override; ///< Log instance from P2P.
      const uint64_t maxMessageSize_ = 1024 * 1024 * 128; ///< Max message size (128 MB).

      /// Runs the session.
      void run();

      /// Closes the session with a reason log message.
      void close(std::string&& reason);

      /// Closes the session without a reason log message.
      void close() { close(""); }

      /// Writes a message to the socket.
      void write(const std::shared_ptr<const Message>& message);

      /// ManagerBase notifies this session that it has been unregistered; returns whether this session was handshaked.
      bool notifyUnregistered();

      ///@{
      /* Getter. */
      const net::ip::address& address() const { return this->address_; }
      const unsigned short& port() const { return port_; }
      std::string addressAndPortStr() const {
        return this->address_.to_string() + ":" + std::to_string(this->port_);
      }
      const NodeID& hostNodeId() const { return this->nodeId_; }
      const NodeType& hostType() const { return this->type_; }
      const ConnectionType& connectionType() const { return this->connectionType_; }
      ///@}
  };
}

#endif  // P2P_SESSION_H
