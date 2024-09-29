#include "callexecutor.h"
#include "../contracthost.h"

namespace cpp {

Bytes CallExecutor::executeCall(kind::Normal, Gas& gas, const evm::Message& msg) {
  gas.use(1000);

  const auto it = contracts_.find(msg.to);

  if (it == contracts_.end()) {
    throw std::runtime_error("Contract not found"); // TODO: proper error + more info
  }

  auto& contract = *it->second;

  contract.caller_ = msg.from;
  contract.value_ = msg.value;

  const evmc_message evmcMsg{
    .kind = EVMC_CALL,
    .flags = 0,
    .depth = msg.depth,
    .gas = gas.value(),
    .recipient = msg.to.toEvmcAddress(),
    .sender = msg.from.toEvmcAddress(),
    .input_data = msg.input.data(),
    .input_size = msg.input.size()
  };

  return contract.evmEthCall(evmcMsg, &host_);
}

} // namespace cpp
