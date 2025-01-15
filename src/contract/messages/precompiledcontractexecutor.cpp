#include "precompiledcontractexecutor.h"

Bytes PrecompiledContractExecutor::execute(EncodedStaticCallMessage& msg) {
  const uint256_t randomValue = std::invoke(randomGen_);
  return Utils::makeBytes(UintConv::uint256ToBytes(randomValue));
}
