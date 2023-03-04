#include "randomgen.h"

uint256_t RandomGen::operator()() {
  this->seedLock.lock();
  this->seed = Utils::sha3(this->seed.get());
  uint256_t ret = this->seed.toUint256();
  this->seedLock.unlock();
  return ret;
}

