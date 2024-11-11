#include "callexecutor.h"
#include "../contracthost.h"

namespace cpp {

GasGuard::GasGuard(ContractHost& host)
  : host_(host), prevGas_(host.getGas()) {}

GasGuard::~GasGuard() {
  host_.setGas(prevGas_);
}

Bytes CallExecutor::executeCall(kind::Normal, Gas& gas, const evm::Message& msg) {
  const evmc_message evmcMsg{
    .kind = EVMC_CALL,
    .flags = 0,
    .depth = msg.depth,
    .gas = gas.value(),
    .recipient = bytes::cast<evmc_address>(msg.to),
    .sender = bytes::cast<evmc_address>(msg.from),
    .input_data = msg.input.data(),
    .input_size = msg.input.size()
  };

  const GasGuard gasContextGuard = setGasContext(gas);

  return prepareCall(gas, msg).evmEthCall(evmcMsg, &host_);
}

Bytes CallExecutor::executeCall(kind::Static, Gas& gas, const evm::Message& msg) {
  const evmc_message evmcMsg{
    .kind = EVMC_CALL,
    .flags = EVMC_STATIC,
    .depth = msg.depth,
    .gas = gas.value(),
    .recipient = bytes::cast<evmc_address>(msg.to),
    .sender = bytes::cast<evmc_address>(msg.from),
    .input_data = msg.input.data(),
    .input_size = msg.input.size()
  };

  // TODO: construct evmc_message only after call is prepared, so gas will be correct

  const GasGuard gasContextGuard = setGasContext(gas);

  return prepareCall(gas, msg).ethCallView(evmcMsg, &host_);
}

BaseContract& CallExecutor::prepareCall(Gas& gas, const evm::Message& msg) {
  gas.use(1000);

  const auto it = contracts_.find(msg.to);

  if (it == contracts_.end()) {
    throw ExecutionFailure("Contract not found"); // TODO: proper error + more info
  }

  auto& contract = *it->second;

  contract.caller_ = msg.from;
  contract.value_ = msg.value;

  return contract;
}

GasGuard CallExecutor::setGasContext(Gas& gas) {
  GasGuard guard(host_);
  host_.setGas(gas);
  return guard;
}

} // namespace cpp
