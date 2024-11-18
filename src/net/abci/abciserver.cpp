#include "abciserver.h"
#include "abcinetserver.h"

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
  std::scoped_lock lock(stateMutex_);
  if (started_) {
    return false;
  }

  // Remove the socket file if it already exists
  ::unlink(cometUNIXSocketPath_.c_str());

  // create the net server
  abciNetServer_ = std::make_shared<ABCINetServer>(handler_, cometUNIXSocketPath_);

  // start the net server
  abciNetServer_->start();

  started_ = true;
  return true;
}

bool ABCIServer::stop() {
  std::scoped_lock lock(stateMutex_);
  if (!started_) {
    return false;
  }

  // Stop the net engine
  abciNetServer_->stop("ABCIServer::stop()");

  // Destroy the net engine
  abciNetServer_.reset();

  // Here we can do a check for whether the abciNetServer_ net engine
  // object was actually deleted (or whether ASIO is still hanging on
  // to a hard reference to it) like it is done in the p2p engine.

  started_ = false;
  return true;
}

bool ABCIServer::running() {
  std::scoped_lock lock(stateMutex_);
  if (abciNetServer_) {
    return abciNetServer_->running();
  }
  return false;
}
