#include "randomgen.h"

uint256_t RandomGen::operator()() {
  std::lock_guard lock(this->seedLock_);
  this->seed_ = Utils::sha3(this->seed_.get());
  uint256_t ret = this->seed_.toUint256();
  return ret;
}

