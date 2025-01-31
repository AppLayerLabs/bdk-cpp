
#include "abcinetserver.h"
#include "abcinetsession.h"

#include "../../utils/logger.h"

// Since the ABCI opens 4 stream connections, run 4 threads just so no request can ever block other requests in other connections.
// REVIEW: Do we actually need 4 threads, or can we get away with processing just one (or two) ABCI requests at a time?
#define ABCI_NET_SERVER_NUM_THREADS 4

ABCINetServer::ABCINetServer(ABCIHandler *handler, const std::string &cometUNIXSocketPath)
  : handler_(handler),
    acceptor_(ioContext_),
    cometUNIXSocketPath_(cometUNIXSocketPath),
    threadPool_(ABCI_NET_SERVER_NUM_THREADS),
    workGuard_(boost::asio::make_work_guard(ioContext_))
{
}

ABCINetServer::~ABCINetServer() {
  LOGXTRACE("~ABCINetServer()");
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
        failed("Error accepting connection: " + ec.message());
      }
    }
  );
}

bool ABCINetServer::start() {
  if (started_) {
    // calling start() twice is an error
    LOGDEBUG("ABCINetServer::start(): ERROR: already started.");
    return false;
  }
  if (stopped_) {
    // object is not reusable, cannot restart
    LOGDEBUG("ABCINetServer::start(): ERROR: already stopped.");
    return false;
  }
  started_ = true;
  failed_ = false;

  for (std::size_t i = 0; i < ABCI_NET_SERVER_NUM_THREADS; ++i) {
    boost::asio::post(
      threadPool_.get_executor(),
      [this]() {
        try {
          this->ioContext_.run();
        } catch (const std::exception& ex) {
          GLOGDEBUG("EXCEPTION caught in ABCINetServer IO thread: " + std::string(ex.what()));
        }
      }
    );
  }

  try {
    acceptor_.open(boost::asio::local::stream_protocol());
    acceptor_.bind(boost::asio::local::stream_protocol::endpoint(cometUNIXSocketPath_));
    acceptor_.listen();
  } catch (const std::exception& e) {
    LOGERROR("Error while trying to start ABCI listen socket: " + std::string(e.what()));
    failed("ABCINetServer::start() failed acceptor: " + std::string(e.what()));
    stop();
    return false;
  }

  do_accept();
  return true;
}

void ABCINetServer::sessionDestroyed() {
  ++sessionDestroyed_;
  LOGXTRACE("ABCINetServer::sessionDestroyed() #" + std::to_string(sessionDestroyed_));
}

void ABCINetServer::stop() {
  if (stopped_) {
    return;
  }

  std::unique_lock<std::mutex> lock(sessionsMutex_);

  LOGXTRACE("ABCINetServer::stop(): closing acceptor");

  // Toggle this inside sessionsMutex_ to sync with do_accept()
  stopped_ = true;

  // stop creating new sessions
  boost::system::error_code ec;
  acceptor_.cancel(ec); // Cancel the acceptor.
  if (ec) { LOGDEBUG("Failed to cancel acceptor operations: " + ec.message()); }
  acceptor_.close(ec); // Close the acceptor.
  if (ec) { LOGDEBUG("Failed to close acceptor: " + ec.message()); }

  LOGXTRACE("ABCINetServer::stop(): closing all sessions");

  // Close all active sessions.
  // This will close the socket and ensure the event handler queue for that
  // connection starves, which eventually causes ASIO to forget its shared_ptr
  // to the ABCINetSession.
  for (auto &session : sessions_) {
    session->close();
  }

  // We only need to count the ~ABCINetSession callbacks, since they all
  // close their sockets and get destroyed at the same time.
  int sessionCount = sessions_.size();

  // Delete all our session refs so ASIO is the only thing referencing them,
  // and when it no longer does so, ~ABCINetSession can run.
  sessions_.clear();

  lock.unlock();

  LOGXTRACE("ABCINetServer::stop(): waiting for all sessions to be destroyed");

  // Here we ensure all sessions have removed themselves, meaning
  // the destructor of all of them has been called. So we don't
  // have any ABCINetSession objects dangling.
  int tries = 200;
  while (true) {
    if (sessionDestroyed_ >= sessionCount) {
      break;
    }
    if (--tries <= 0) {
      // Should never happen
      LOGDEBUG("WARNING: Timed out (4s) waiting for sessions to be destroyed; sessionDestroyed_ == " + std::to_string(sessionDestroyed_));
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  LOGXTRACE("ABCINetServer::stop(): stop IOContext & join threadpool...");

  // completely stop the ABCI net engine so it can be deleted
  workGuard_.reset();
  ioContext_.stop();
  threadPool_.join();

  LOGXTRACE("ABCINetServer::stop(): stopped IOContext & joined threadpool.");
}

void ABCINetServer::failed(const std::string &reason) {
  LOGXTRACE("ABCINetServer::failed(): " + reason);
  failed_ = true;
}

bool ABCINetServer::running() {
  return started_ && !stopped_ && !failed_;
}
