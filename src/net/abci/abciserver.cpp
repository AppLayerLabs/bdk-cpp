#include "abciserver.h"
#include "abcinetserver.h"

#include "../../utils/logger.h"

ABCIServer::ABCIServer(ABCIHandler *handler, const std::string& cometUNIXSocketPath)
  : handler_(handler), cometUNIXSocketPath_(cometUNIXSocketPath)
{
}

ABCIServer::~ABCIServer() {
  LOGDEBUG("~ABCIServer(): stopping");
  stop();
  LOGDEBUG("~ABCIServer(): stopped");
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
  if (!abciNetServer_->start()) {
    abciNetServer_.reset();
    return false;
  }

  started_ = true;
  return true;
}

bool ABCIServer::stop() {
  LOGXTRACE("ABCIServer::stop()");
  std::scoped_lock lock(stateMutex_);
  if (!started_) {
    return false;
  }

  LOGXTRACE("ABCIServer::stop() NetServer stopping");
  abciNetServer_->stop();
  LOGXTRACE("ABCIServer::stop() NetServer stopped, waiting for it to be destroyed");

  std::weak_ptr<ABCINetServer> weakNetServer = abciNetServer_;

  // Destroy the net engine
  // We should not have to worry about ABCI net engine objects still
  //   existing and calling back the handler_ object after this.
  abciNetServer_.reset();

  while (true) {
    {
      auto locked = weakNetServer.lock();
      if (!locked) {
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  LOGXTRACE("ABCIServer::stop() NetServer destroyed");

  started_ = false;
  return true;
}

bool ABCIServer::running() {
  std::scoped_lock lock(stateMutex_);
  if (abciNetServer_) {
    // This catches ABCINetServer::failed_ == true set by I/O ops
    // so that the caller (owner of ABCIServer) can call ABCIServer::stop(),
    // which calls ABCINetServer::stop().
    return abciNetServer_->running();
  }
  return false;
}
