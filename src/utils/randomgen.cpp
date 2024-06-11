/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "randomgen.h"

uint256_t RandomGen::operator()() {
  std::lock_guard lock(this->seedLock_);
  this->seed_ = Utils::sha3(this->seed_);
  uint256_t ret = this->seed_.toUint256();
  return ret;
}

