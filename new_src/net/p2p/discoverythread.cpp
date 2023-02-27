#include "p2pmanagerbase.h"

namespace P2P {

  void ManagerBase::handleDiscoveryStop() {
    this->discoveryThreadRunning_ = false;
  }

  void ManagerBase::discoveryThread() {
    this->discoveryThreadRunning_ = true;
    while (!this->discoveryThreadStopFlag_) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  void ManagerBase::startDiscovery() {
    this->discoveryThread_ = std::thread(&ManagerBase::discoveryThread, this);
    this->discoveryThread_.detach();
  }

  void ManagerBase::stopDiscovery() {
    this->discoveryThreadStopFlag_ = true;
  }

}