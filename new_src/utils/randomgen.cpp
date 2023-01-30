#include "randomgen.h"

template <typename Vector> void RandomGen::shuffle(Vector& v) {
  this->seedLock.lock();
  for (uint64_t i = 0; i < v.size(); ++i) {
    this->seed = Utils::sha3(this->seed.get());
    //std::cout << this->seed.hex() << std::endl; // Uncomment to print seed
    uint64_t n = i + this->seed.toUint256() % (v.size() - i);
    std::swap(v[n], v[i]);
  }
  this->seedLock.unlock();
}

uint256_t RandomGen::operator()() {
  this->seedLock.lock();
  this->seed = Utils::sha3(this->seed.get());
  uint256_t ret = this->seed.toUint256();
  this->seedLock.unlock();
  return ret;
}

