#include "abciserver.h"

#include "abcinetserver.h"

// TODO: can we take this off?
#include "abcihandler.h"

ABCIServer::ABCIServer(ABCIHandler *handler, const std::string& cometUNIXSocketPath) 
  : handler_(handler), cometUNIXSocketPath_(cometUNIXSocketPath)
{
}

ABCIServer::~ABCIServer() {
  stop();
}

const std::string ABCIServer::getSocketPath() {
  return this->cometUNIXSocketPath_;
}

bool ABCIServer::start() {
  if (started_) {
    return false;
  }

  // need to be set before (started_ = true) is set below, in any case, since running_ = false
  // is set within the runThread_ to signal that the net engine stopped (e.g. errors).
  //
  // we are ready to accept TCP connections (heuristic, but should be fine; cometbft
  // will redial the ABCI server until it can connect to it anyways).
  running_ = true;

  // start abci net thread
  // the abci net thread does everything, starts everything, etc.
  // if something goes wrong, the running_ flag will be set to false within that thread.
  runThread_ = std::make_unique<std::thread>(&ABCIServer::run, this);

  // transition to started state
  started_ = true;

  return true;
}

bool ABCIServer::stop() {
  if (!started_) {
    return false;
  }

  // stop the io_context, which will cause runThread_->join() to actually resolve, since
  // all io_context::run() calls for all threadpool threads will return.
  // TODO: should we also set an atomic stopped flag at abciServer_ so it knows that it
  // is not a failure but a programmed shutdown?
  if (ioContext_) {
    ioContext_->stop();
  }

  if (runThread_) {
    runThread_->join();
    runThread_.reset();
  }

  // transition to not started state
  started_ = false;

  return true;
}

void ABCIServer::run() {

  // Remove the socket file if it already exists
  ::unlink(cometUNIXSocketPath_.c_str());

  // create all networking objects
  ioContext_ = std::make_unique<boost::asio::io_context>();
  threadPool_ = std::make_unique<boost::asio::thread_pool>(std::thread::hardware_concurrency());
  abciNetServer_ = std::make_shared<ABCINetServer>(handler_, *ioContext_, cometUNIXSocketPath_);

  // post the TCP socket accept task
  abciNetServer_->start();

  // run the I/O context using multiple threads
  for (std::size_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    asio::post(threadPool_->get_executor(), [this]() { this->ioContext_->run(); });
  }

  // block waiting for the server to finish, which happens when e.g. the ABCI session
  // TCP sockets get closed, which mean that e.g. the cometbft process was terminated.
  threadPool_->join();

  // signals that the net engine is no longer running (even if it is still set to a started state)
  running_ = false;

  // TOOD: must distinguish actual failures from normal shutdown
  //if (abciServer_->failed()) {

  abciNetServer_.reset(); // get rid of our shared pointer to the server component

  ioContext_->stop(); // TODO: remove? probably not necessary since we already joined the threadPool_
  ioContext_.reset(); // should ensure that all shared abcinetserver and abcinetsession objects are destroyed

  threadPool_.reset();
}

bool ABCIServer::running() {
  return running_;
}
