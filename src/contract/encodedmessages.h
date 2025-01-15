#ifndef BDK_ENCODEDMESSAGES_H
#define BDK_ENCODEDMESSAGES_H

#include "contract/concepts.h"
#include "contract/basemessage.h"

struct EncodedCallMessage : BaseMessage<FromField, ToField, GasField, ValueField, InputField> {
  using BaseMessage::BaseMessage;
};

struct EncodedStaticCallMessage : BaseMessage<FromField, ToField, GasField, InputField> {
  using BaseMessage::BaseMessage;
};

struct EncodedCreateMessage : BaseMessage<FromField, GasField, ValueField, CodeField> {
  using BaseMessage::BaseMessage;
};

struct EncodedSaltCreateMessage : BaseMessage<FromField, GasField, ValueField, CodeField, SaltField> {
  using BaseMessage::BaseMessage;
};

struct EncodedDelegateCallMessage : BaseMessage<FromField, ToField, GasField, ValueField, InputField, CodeAddressField> {
  using BaseMessage::BaseMessage;
};

struct EncodedCallCodeMessage : EncodedCallMessage {
  using EncodedCallMessage::EncodedCallMessage;
};

template<>
constexpr bool concepts::EnableDelegate<EncodedDelegateCallMessage> = true;

template<>
constexpr bool concepts::EnableCallCode<EncodedCallCodeMessage> = true;

using EncodedMessageVariant = std::variant<
  EncodedCreateMessage,
  EncodedSaltCreateMessage,
  EncodedCallMessage,
  EncodedStaticCallMessage,
  EncodedDelegateCallMessage
>;

#endif // BDK_ENCODEDMESSAGES_H
