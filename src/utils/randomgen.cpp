/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "randomgen.h"

RandomGen::RandomGen(const RandomGen& other) {
  std::unique_lock lock(other.seedLock_);
  seed_ = other.seed_;
}

RandomGen& RandomGen::operator=(const RandomGen& other) {
  std::unique_lock lockA(seedLock_, std::defer_lock);
  std::unique_lock lockB(other.seedLock_, std::defer_lock);

  std::lock(lockA, lockB);

  seed_ = other.seed_;

  return *this;
}

RandomGen& RandomGen::operator=(RandomGen&& other) noexcept {
  std::unique_lock lock(seedLock_);
  seed_ = other.seed_;
  return *this;
}

uint256_t RandomGen::operator()() {
  std::lock_guard lock(this->seedLock_);
  this->seed_ = Utils::sha3(this->seed_);
  uint256_t ret = static_cast<uint256_t>(this->seed_);
  return ret;
}

