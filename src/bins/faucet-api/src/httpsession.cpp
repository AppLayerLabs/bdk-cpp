/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "httpsession.h"

namespace Faucet {
  HTTPQueue::HTTPQueue(HTTPSession& session) : session_(session) {
    assert(this->limit_ > 0);
    this->items_.reserve(this->limit_);
  }

  bool HTTPQueue::full() const { return this->items_.size() >= this->limit_; }

  bool HTTPQueue::on_write() {
    BOOST_ASSERT(!this->items_.empty());
    bool wasFull = this->full();
    this->items_.erase(this->items_.begin());
    if (!this->items_.empty()) (*this->items_.front())();
    return wasFull;
  }

  template<bool isRequest, class Body, class Fields> void HTTPQueue::operator()(
    http::message<isRequest, Body, Fields>&& msg
  ) {
    // This holds a work item
    struct work_impl : work {
      HTTPSession& session;
      http::message<isRequest, Body, Fields> msg; // This msg is internal
      work_impl(HTTPSession& session, http::message<isRequest, Body, Fields>&& msg)
        : session(session), msg(std::move(msg)) {}
      void operator()() override {
        http::async_write(
          session.stream_, msg, beast::bind_front_handler(
            &HTTPSession::on_write, session.shared_from_this(), msg.need_eof()
          )
        );
      }
    };

    // Allocate and store the work, and if there was no previous work, start this one
    this->items_.push_back(boost::make_unique<work_impl>(this->session_, std::move(msg))); // This msg is from the header
    if (this->items_.size() == 1) (*this->items_.front())();
  }

  void HTTPSession::do_read() {
    this->parser_.emplace();  // Construct a new parser for each message
    this->parser_->body_limit(512000); // Apply a reasonable limit to body size in bytes to prevent abuse
    // Read a request using the parser-oriented interface
    http::async_read(this->stream_, this->buf_, *this->parser_, beast::bind_front_handler(
      &HTTPSession::on_read, this->shared_from_this()
    ));
  }

  void HTTPSession::on_read(beast::error_code ec, std::size_t bytes) {
    boost::ignore_unused(bytes);
    // This means the other side closed the connection
    if (ec == http::error::end_of_stream) return this->do_close();
    if (ec) return fail("HTTPSession", __func__, ec, "Failed to close connection");
    // Send the response
    handle_request(
      *this->docroot_, this->parser_->release(), this->queue_, this->faucet_
    );
    // If queue still has free space, try to pipeline another request
    if (!this->queue_.full()) this->do_read();
  }

  void HTTPSession::on_write(bool close, beast::error_code ec, std::size_t bytes) {
    boost::ignore_unused(bytes);
    if (ec) return fail("HTTPSession", __func__, ec, "Failed to write to buffer");
    // This means we should close the connection, usually because the
    // response indicated the "Connection: close" semantic
    if (close) return this->do_close();
    // Inform the queue that a write was completed and read another request
    if (this->queue_.on_write()) this->do_read();
  }

  void HTTPSession::do_close() {
    // Send a TCP shutdown
    beast::error_code ec;
    this->stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    // At this point the connection is closed gracefully
  }

  void HTTPSession::start() {
    net::dispatch(this->stream_.get_executor(), beast::bind_front_handler(
      &HTTPSession::do_read, this->shared_from_this()
    ));
  }
}

