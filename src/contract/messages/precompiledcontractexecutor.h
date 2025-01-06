#ifndef BDK_CONTRACT_PRECOMPILEDCONTRACTEXECUTOR_H
#define BDK_CONTRACT_PRECOMPILEDCONTRACTEXECUTOR_H

#include "utils/randomgen.h"
#include "traits.h"
#include "contract/messages/encodedmessages.h"
#include "contract/abi.h"

class PrecompiledContractExecutor {
public:
  explicit PrecompiledContractExecutor(RandomGen randomGen) : randomGen_(std::move(randomGen)) {}

  Bytes execute(EncodedStaticCallMessage& msg);

  template<concepts::CallMessage Message, typename Result = traits::MessageResult<Message>>
  Result execute(Message&& msg) {

    if constexpr (concepts::DelegateCallMessage<decltype(msg)>) {
      throw DynamicException("Delegate calls not allowed for precompiled contracts");
    }

    const Bytes input = messageInputEncoded(msg);
    EncodedStaticCallMessage encodedMessage(msg.from(), msg.to(), msg.gas(), input);

    if constexpr (concepts::PackedMessage<Message>) {
      if constexpr (std::same_as<Result, void>) {
        this->execute(encodedMessage);
      } else {
        return std::get<0>(ABI::Decoder::decodeData<Result>(this->execute(encodedMessage)));
      }
    } else {
      return this->execute(encodedMessage);
    }
  }

  RandomGen& randomGenerator() { return randomGen_; }

private:
  RandomGen randomGen_;
};

#endif // BDK_CONTRACT_PRECOMPILEDCONTRACTEXECUTOR_H
