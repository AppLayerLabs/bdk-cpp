
#include "abcinetserver.h"
#include "abcinetsession.h"

#include "../../utils/logger.h"

#define ABCI_NET_SERVER_NUM_THREADS 4

ABCINetServer::ABCINetServer(ABCIHandler *handler, const std::string &cometUNIXSocketPath)
  : handler_(handler),
    acceptor_(ioContext_),
    cometUNIXSocketPath_(cometUNIXSocketPath),
    threadPool_(ABCI_NET_SERVER_NUM_THREADS),
    workGuard_(boost::asio::make_work_guard(ioContext_))
{
}

void ABCINetServer::start() {
  if (started_ || stopped_) {
    return;
  }
  started_ = true;

  for (std::size_t i = 0; i < ABCI_NET_SERVER_NUM_THREADS; ++i) {
    boost::asio::post(threadPool_.get_executor(), [this]() { this->ioContext_.run(); });
  }

  try {
    acceptor_.open(boost::asio::local::stream_protocol());
    acceptor_.bind(boost::asio::local::stream_protocol::endpoint(cometUNIXSocketPath_));
    acceptor_.listen();
  } catch (const std::exception& e) {
    LOGERROR("Error while trying to start ABCI listen socket: " + std::string(e.what()));
    stop("ABCINetServer::start() failed acceptor: " + std::string(e.what()));
    return;
  }

  do_accept();
}

void ABCINetServer::stop(const std::string &reason) {
  if (stopped_) {
    return;
  }

  std::unique_lock<std::mutex> lock(sessionsMutex_);

  stopped_ = true;

  // stop creating new sessions
  acceptor_.close();

  // Close all active sessions
  // Since we only keep weak pointers we don't need to actually clear the sessions list
  for (auto &weak_session : sessions_) {
    if (auto session = weak_session.lock()) {
      session->close();
    }
  }

  lock.unlock();

  // stop the net engine
  workGuard_.reset();
  ioContext_.stop();
  threadPool_.join();
}

bool ABCINetServer::running() {
  return started_ && !stopped_;
}

void ABCINetServer::do_accept() {
  auto self(shared_from_this());
  acceptor_.async_accept(
    [this, self](boost::system::error_code ec, boost::asio::local::stream_protocol::socket socket)
    {
      if (!ec) {
        std::scoped_lock lock(sessionsMutex_);
        if (!stopped_) {
          auto session = std::make_shared<ABCINetSession>(handler_, std::move(socket), self);
          sessions_.push_back(session);
          session->start();
          do_accept();
        } else {
          socket.close();
        }
      } else {
        stop("Error accepting connection: " + ec.message());
      }
    }
  );
}