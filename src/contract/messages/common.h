#ifndef BDK_MESSAGES_COMMON_H
#define BDK_MESSAGES_COMMON_H

#include "utils/utils.h"
#include "concepts.h"

Address generateContractAddress(uint64_t nonce, View<Address> address);

Address generateContractAddress(View<Address> from, View<Hash> salt, View<Bytes> code);

constexpr uint256_t messageValueOrZero(const auto& msg) {
  if constexpr (concepts::HasValueField<decltype(msg)>) {
    return msg.value();
  } else {
    return uint256_t(0);
  }
}

constexpr View<Address> messageCodeAddress(const auto& msg) {
  if constexpr (concepts::DelegateCallMessage<decltype(msg)>) {
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

#endif // BDK_MESSAGES_COMMON_H
