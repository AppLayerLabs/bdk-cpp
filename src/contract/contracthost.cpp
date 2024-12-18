#include "contracthost.h"

ContractHost::~ContractHost() {
  if (mustRevert_) {
    for (auto& var : this->stack_.getUsedVars())
      var.get().revert();

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
}
