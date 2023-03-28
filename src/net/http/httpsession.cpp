#include "httpsession.h"

HTTPQueue::HTTPQueue(HTTPSession& session) : session(session) {
  assert(this->limit > 0);
  items.reserve(this->limit);
}

bool HTTPQueue::full() { return this->items.size() >= this->limit; }

bool HTTPQueue::on_write() {
  BOOST_ASSERT(!this->items.empty());
  bool wasFull = this->full();
  this->items.erase(this->items.begin());
  if (!this->items.empty()) (*this->items.front())();
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
        session.stream, msg, beast::bind_front_handler(
          &HTTPSession::on_write, session.shared_from_this(), msg.need_eof()
        )
      );
    }
  };

  // Allocate and store the work, and if there was no previous work, start this one
  this->items.push_back(boost::make_unique<work_impl>(session, std::move(msg))); // This msg is from the header
  if (this->items.size() == 1) (*this->items.front())();
}

void HTTPSession::do_read() {
  this->parser.emplace();  // Construct a new parser for each message
  this->parser->body_limit(10000); // Apply a reasonable limit to body size in bytes to prevent abuse
  this->stream.expires_after(std::chrono::seconds(30)); // Set a reasonable timeout
  // Read a request using the parser-oriented interface
  http::async_read(this->stream, this->buf, *this->parser, beast::bind_front_handler(
    &HTTPSession::on_read, this->shared_from_this()
  ));
}

void HTTPSession::on_read(beast::error_code ec, std::size_t bytes) {
  boost::ignore_unused(bytes);
  if (ec == http::error::end_of_stream) return this->do_close(); // This means the other side closed the connection
  if (ec) return fail("HTTPSession", __func__, ec, "Failed to close connection");
  handle_request(*this->docroot, this->parser->release(), this->queue, this->state, this->storage, this->p2p); // Send the response
  if (!this->queue.full()) this->do_read(); // If queue still has free space, try to pipeline another request
}

void HTTPSession::on_write(bool close, beast::error_code ec, std::size_t bytes) {
  boost::ignore_unused(bytes);
  if (ec) return fail("HTTPSession", __func__, ec, "Failed to write to buffer");
  // This means we should close the connection, usually because the response indicated the "Connection: close" semantic
  if (close) return this->do_close();
  if (this->queue.on_write()) this->do_read(); // Inform the queue that a write was completed and read another request
}

void HTTPSession::do_close() {
  // Send a TCP shutdown
  beast::error_code ec;
  this->stream.socket().shutdown(tcp::socket::shutdown_send, ec);
  // At this point the connection is closed gracefully
}

void HTTPSession::start() {
  net::dispatch(this->stream.get_executor(), beast::bind_front_handler(
    &HTTPSession::do_read, this->shared_from_this()
  ));
}

