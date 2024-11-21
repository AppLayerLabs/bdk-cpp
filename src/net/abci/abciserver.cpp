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

  abciNetServer_ = std::make_shared<ABCINetServer>(handler_, cometUNIXSocketPath_);
  abciNetServer_->start();

  started_ = true;
  return true;
}

bool ABCIServer::stop() {
  std::scoped_lock lock(stateMutex_);
  if (!started_) {
    return false;
  }

  abciNetServer_->stop("ABCIServer::stop()");

  // Destroy the net engine
  // We should not have to worry about ABCI net engine objects still
  //   existing and calling back the handler_ object after this.
  abciNetServer_.reset();

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
