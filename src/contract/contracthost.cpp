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
