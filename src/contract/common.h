#ifndef BDK_MESSAGES_COMMON_H
#define BDK_MESSAGES_COMMON_H

#include "utils/utils.h"
#include "utils/contractreflectioninterface.h"
#include "concepts.h"

Address generateContractAddress(uint64_t nonce, View<Address> address);

Address generateContractAddress(View<Address> from, View<Hash> salt, View<Bytes> code);

#ifdef BUILD_TESTNET
Address deprecatedGenerateContractAddress(uint64_t nonce, View<Address> address);
#endif

constexpr uint256_t messageValueOrZero(const auto& msg) {
  if constexpr (concepts::HasValueField<decltype(msg)>) {
    return msg.value();
  } else {
    return uint256_t(0);
  }
}

constexpr View<Address> messageCodeAddress(const auto& msg) {
  if constexpr (concepts::DelegateCallMessage<std::remove_cvref_t<decltype(msg)>>) {

    return msg.codeAddress();
  } else {
    return msg.to();
  }
}

constexpr Address messageRecipientOrDefault(const auto& msg) {
  if constexpr (concepts::CreateMessage<decltype(msg)>) {
    return Address{};
  } else {
    return Address(msg.to());
  }
}

constexpr Hash messageSaltOrDefault(const auto& msg) {
  if constexpr (concepts::SaltMessage<decltype(msg)>) {
    return Hash(msg.salt());
  } else {
    return Hash{};
  }
}

Bytes messageInputEncoded(const concepts::EncodedMessage auto& msg) {
  return Bytes(msg.input());
}

Bytes messageInputEncoded(const concepts::PackedMessage auto& msg) {
  return std::apply([&] (const auto&... args) -> Bytes {
    const std::string functionName = ContractReflectionInterface::getFunctionName(msg.method());

    if (functionName.empty()) {
      throw DynamicException("Contract fuction not found (contract not registered?)");
    }

    const BytesArr<4> encodedFunctor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<decltype(args)...>(std::string(functionName)).value);

    if constexpr (sizeof...(args) > 0) {
      const Bytes encodedArgs = ABI::Encoder::encodeData<decltype(args)...>(args...);
      return Utils::makeBytes(bytes::join(encodedFunctor, encodedArgs));
    }

    return Utils::makeBytes(encodedFunctor);
  }, msg.args());
}

#endif // BDK_MESSAGES_COMMON_H
