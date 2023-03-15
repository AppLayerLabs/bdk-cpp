#include "randomgen.h"

uint256_t RandomGen::operator()() {
  std::lock_guard lock(seedLock);
  this->seed = Utils::sha3(this->seed.get());
  uint256_t ret = this->seed.toUint256();
  return ret;
}

