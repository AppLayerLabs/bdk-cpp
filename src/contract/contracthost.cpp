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
    // Revert all CPP contract state on tx revert
    for (auto& var : this->stack_.getUsedVars()) {
      var.get().revert();
    }
    context_.revert();
  } else {
    // Commit all CPP contract state on tx success
    for (auto& var : this->stack_.getUsedVars()) {
      var.get().commit();
    }
    // Metadata persistence: event logs
    // NOTE: With a bit of work, we could possibly get rid of event logging on our side
    //       and use CometBFT event/log DB and indexing for this. But it's fine as it is.
    auto transaction = this->storage_.events().transaction();
    for (const auto& event : context_.getEvents()) {
      this->storage_.events().putEvent(event);
    }
    transaction.commit();
    // This clears the context_ so it has to be the last step
    context_.commit();
  }

  // Metadata persistence: call traces for debugging
  // (Debug info is something we should definitely keep on our side).
  if (messageHandler_.hasCallTrace()) {
    storage_.putCallTrace(Hash(context_.getTxHash()), messageHandler_.getCallTrace());
  }
}
