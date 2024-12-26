#include "contracthost.h"

MessageHandler makeMessageHandler(ContractHost& host, ExecutionContext& context, evmc_vm *vm, Storage& storage) {
  MessageDispatcher dispatcher(context, CppContractExecutor(context, host), EvmContractExecutor(context, vm));

  if (context.getTxHash() && storage.getIndexingMode() == IndexingMode::RPC_TRACE) {
    return CallTracer(std::move(dispatcher));
  }

  return dispatcher;
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

  std::visit(Utils::Overloaded{
    [] (MessageDispatcher& handler) {},
    [this] (CallTracer<MessageDispatcher>& tracer) {
      if (tracer.hasCallTrace()) {
        storage_.putCallTrace(Hash(context_.getTxHash()), tracer.getCallTrace()); // TODO: do not create a hash
      }
    }
  }, messageHandler_);

  // TODO: save transaction additional data
}
