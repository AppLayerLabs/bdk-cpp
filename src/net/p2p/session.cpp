/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "session.h"
#include "managerbase.h"
#include <functional>

namespace P2P {

  bool Session::handle_error(const std::string& func, const boost::system::error_code& ec) {
    /// TODO: return true/false depending on err code is necessary?
    Logger::logToDebug(LogType::ERROR, Log::P2PSession, std::string(func),
                       "Client Error Code: " + std::to_string(ec.value()) + " message: " + ec.message());
    if (ec != boost::system::errc::operation_canceled) {
      /// operation_canceled == close() was already called, we cannot close or deregister again.
      if (this->doneHandshake_) {
        this->manager_.unregisterSession(shared_from_this());
      }
      this->close();
    }
    /// TODO: Automatically close on error.
    return true;
  }

  void Session::do_connect() {
    boost::asio::ip::tcp::resolver resolver(this->socket_.get_executor());
    auto endpoints = resolver.resolve({this->address_, this->port_});
    net::async_connect(this->socket_, endpoints, net::bind_executor(
      this->writeStrand_, std::bind(
        &Session::on_connect, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::on_connect(boost::system::error_code ec, const net::ip::tcp::endpoint&) {
    if (ec && this->handle_error(__func__, ec)) return;
    this->write_handshake();
  }

  void Session::write_handshake() {
    this->outboundHandshake_[0] = (this->manager_.nodeType() == NodeType::NORMAL_NODE) ? 0x00 : 0x01;
    auto serverPort = Utils::uint16ToBytes(this->manager_.serverPort());
    this->outboundHandshake_[1] = serverPort[0];
    this->outboundHandshake_[2] = serverPort[1];
    net::async_write(this->socket_, net::buffer(this->outboundHandshake_, 3), net::bind_executor(
      this->writeStrand_, std::bind(
        &Session::read_handshake, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::read_handshake(boost::system::error_code ec, std::size_t) {
    if (ec && this->handle_error(__func__, ec)) return;
    net::async_read(this->socket_, net::buffer(this->inboundHandshake_, 3), net::bind_executor(
      this->readStrand_, std::bind(
        &Session::finish_handshake, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::finish_handshake(boost::system::error_code ec, std::size_t) {
    if (ec && this->handle_error(__func__, ec)) return;
    if (this->inboundHandshake_.size() != 3) {
      Logger::logToDebug(LogType::ERROR, Log::P2PSession, __func__, "Invalid handshake size");
      this->close();
      return;
    }
    this->type_ = (!this->inboundHandshake_[0]) ? NodeType::NORMAL_NODE : NodeType::DISCOVERY_NODE;
    this->serverPort_ = Utils::bytesToUint16(Utils::create_view_span(this->inboundHandshake_, 1, 2));
    this->doneHandshake_ = true;
    this->nodeId_ = {this->address_, this->serverPort_};
    if (!this->manager_.registerSession(shared_from_this())) { this->close(); return; }
    this->do_read_header(); // Start reading messages.
  }

  void Session::do_read_header() {
    inboundHeader_.fill(0x00);
    net::async_read(this->socket_, net::buffer(this->inboundHeader_), net::bind_executor(
      this->readStrand_, std::bind(
        &Session::on_read_header, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::on_read_header(boost::system::error_code ec, std::size_t) {
    if (ec && this->handle_error(__func__, ec)) return;
    uint64_t messageSize = Utils::bytesToUint64(this->inboundHeader_);
    if (messageSize > this->maxMessageSize_) {
      Logger::logToDebug(LogType::WARNING, Log::P2PSession, __func__,
        "Message size too large: " + std::to_string(messageSize)
        + " max: " + std::to_string(this->maxMessageSize_) + " closing session..."
      );
      this->close();
      return;
    }
    this->do_read_message(messageSize);
  }

  void Session::do_read_message(const uint64_t& messageSize) {
    this->inboundMessage_ = std::make_shared<Message>();
    net::dynamic_vector_buffer readBuffer(this->inboundMessage_->rawMessage_);
    auto mutableBuffer = readBuffer.prepare(messageSize);
    net::async_read(this->socket_, mutableBuffer, net::bind_executor(this->readStrand_, std::bind(
      &Session::on_read_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2
    )));
  }

  void Session::on_read_message(boost::system::error_code ec, std::size_t) {
    if (ec && this->handle_error(__func__, ec)) return;
    // Make it a unique_ptr<const Message> so that we can pass it to the thread pool.
    this->threadPool_.push_task(
      &ManagerBase::handleMessage, &this->manager_, weak_from_this(), this->inboundMessage_
    );
    this->inboundMessage_ = nullptr;
    this->do_read_header();
  }

  void Session::do_write_header() {
    // Nothing to do, someone called us by mistake.
    if (this->outboundMessage_ == nullptr) return;
    this->outboundHeader_ = Utils::uint64ToBytes(this->outboundMessage_->rawMessage_.size());
    net::async_write(this->socket_, net::buffer(this->outboundHeader_), net::bind_executor(
      this->writeStrand_, std::bind(
        &Session::on_write_header, shared_from_this(), std::placeholders::_1, std::placeholders::_2
      )
    ));
  }

  void Session::on_write_header(boost::system::error_code ec, std::size_t) {
    if (ec && this->handle_error(__func__, ec)) return;
    this->do_write_message();
  }

  void Session::do_write_message() {
    net::async_write(this->socket_, net::buffer(
      this->outboundMessage_->rawMessage_, this->outboundMessage_->rawMessage_.size()
    ), net::bind_executor(this->writeStrand_, std::bind(
      &Session::on_write_message, shared_from_this(), std::placeholders::_1, std::placeholders::_2
    )));
  }

  void Session::on_write_message(boost::system::error_code ec, std::size_t) {
    if (ec && this->handle_error(__func__, ec)) return;
    std::unique_lock lock(this->writeQueueMutex_);
    if (this->outboundMessages_.empty()) {
      this->outboundMessage_ = nullptr;
    } else {
      this->outboundMessage_ = this->outboundMessages_.front();
      this->outboundMessages_.pop_front();
      this->do_write_header();
    }
  }

  void Session::run() {
    if (this->connectionType_ == ConnectionType::INBOUND) {
      Logger::logToDebug(LogType::INFO, Log::P2PSession, __func__, "Starting new inbound session");
      boost::asio::dispatch(this->socket_.get_executor(), std::bind(&Session::write_handshake, shared_from_this()));
    } else {
      Logger::logToDebug(LogType::INFO, Log::P2PSession, __func__, "Starting new outbound session");
      boost::asio::dispatch(this->socket_.get_executor(), std::bind(&Session::do_connect, shared_from_this()));
    }
  }

  void Session::close() {
    boost::asio::post(this->socket_.get_executor(), std::bind(&Session::do_close, shared_from_this()));
  }

  void Session::do_close() {
    boost::system::error_code ec;
    // Cancel all pending operations.
    this->socket_.cancel(ec);
    if (ec) {
      Logger::logToDebug(LogType::ERROR, Log::P2PSession, __func__, "Failed to cancel socket operations: " + ec.message());
      return;
    }
    // Shutdown the socket;
    this->socket_.shutdown(net::socket_base::shutdown_both, ec);
    if (ec) {
      Logger::logToDebug(LogType::ERROR, Log::P2PSession, __func__, "Failed to shutdown socket: " + ec.message());
      return;
    }
    // Close the socket.
    this->socket_.close(ec);
    if (ec) {
      Logger::logToDebug(LogType::ERROR, Log::P2PSession, __func__, "Failed to close socket: " + ec.message());
      return;
    }
  }

  void Session::write(const std::shared_ptr<const Message>& message) {
    std::unique_lock lock(this->writeQueueMutex_);
    if (this->outboundMessage_ == nullptr) {
      this->outboundMessage_ = message;
      net::post(this->writeStrand_, std::bind(&Session::do_write_header, shared_from_this()));
    } else {
      this->outboundMessages_.push_back(message);
    }
  }
}

