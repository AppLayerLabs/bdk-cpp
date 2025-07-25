/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../utils/strconv.h" //cArrayToBytes

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

    auto transaction = this->storage_.events().transaction();

    for (const auto& event : context_.getEvents()) {
      this->storage_.events().putEvent(event);
    }

    transaction->commit();
    context_.commit();
  }

  if (messageHandler_.hasCallTrace()) {
    storage_.putCallTrace(Hash(context_.getTxHash()), messageHandler_.getCallTrace());
  }
}

void ContractHost::addContractObservers(const BaseContract& contract) {
  if (blockObservers_ == nullptr) {
    return;
  }

  for (const auto& observer : contract.getBlockNumberObservers()) {
    blockObservers_->add(observer);
  }

  for (const auto& observer : contract.getBlockTimestampObservers()) {
    blockObservers_->add(observer);
  }
}
