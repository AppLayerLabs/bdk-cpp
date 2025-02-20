#include "precompiledcontractexecutor.h"
#include "precompiles.h"
#include "bytes/hex.h"

constexpr Address RANDOM_GENERATOR_ADDRESS = bytes::hex("0x1000000000000000000000000000100000000001");
constexpr Address ECRECOVER_ADDRESS = bytes::hex("0x0000000000000000000000000000000000000001");

constexpr auto ECRECOVER_CALL_COST = 3'000;

Bytes PrecompiledContractExecutor::execute(EncodedStaticCallMessage& msg) {
  if (msg.to() == RANDOM_GENERATOR_ADDRESS) {  
    return Utils::makeBytes(UintConv::uint256ToBytes(std::invoke(randomGen_)));
  }

  if (msg.to() == ECRECOVER_ADDRESS) {
    msg.gas().use(ECRECOVER_CALL_COST);
    const auto [hash, v, r, s] = ABI::Decoder::decodeData<Hash, uint8_t, Hash, Hash>(msg.input());
    return ABI::Encoder::encodeData(ecrecover(hash, v, r, s));
  }

  throw DynamicException("Precompiled contract not found");
}

bool PrecompiledContractExecutor::isPrecompiled(View<Address> address) const {
  return address == RANDOM_GENERATOR_ADDRESS || address == ECRECOVER_ADDRESS;
}
