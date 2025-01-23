/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../utils/strconv.h" //cArrayToBytes

#include "../core/storage.h"

#include "contracthost.h"

uint256_t ContractHost::getRandomValue() {
  return std::invoke(messageHandler_.handler().precompiledExecutor().randomGenerator());
}

ContractHost::~ContractHost() {
  if (mustRevert_) {
    for (auto& var : this->stack_.getUsedVars()) {
      var.get().revert();
    }

    context_.revert();
  } else {
    for (auto& var : this->stack_.getUsedVars())
      var.get().commit();

    for (auto& [address, contract] : context_.getNewContracts()) {
      if (contract == nullptr) {
        continue;
      }

      this->manager_.pushBack(dynamic_cast<Dumpable*>(contract));
    }

    for (const auto& event : context_.getEvents()) {
      this->storage_.putEvent(event);
    }

    context_.commit();
  }

  if (messageHandler_.hasCallTrace()) {
    storage_.putCallTrace(Hash(context_.getTxHash()), messageHandler_.getCallTrace());
  }

  // TODO: save transaction additional data
}
