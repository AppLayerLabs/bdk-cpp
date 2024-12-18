#ifndef BDK_MESSAGES_CPPCONTRACTEXECUTOR_H
#define BDK_MESSAGES_CPPCONTRACTEXECUTOR_H

#include "executioncontext.h"
#include "traits.h"
#include "bytes/cast.h"
#include "utils/contractreflectioninterface.h"

struct ContractHost;

class CppContractExecutor {
public:
  CppContractExecutor(ExecutionContext& context, ContractHost& host)
    : context_(context), host_(host) {}

  template<concepts::Message M>
  auto execute(M&& msg) -> traits::MessageResult<M> {
    if constexpr (concepts::DelegateCallMessage<M>) {
      throw DynamicException("Delegate call not supported for C++ contracts");
    }    

    auto guard = transactional::checkpoint(currentGas_);
    currentGas_ = &msg.gas();

    if constexpr (concepts::CreateMessage<M>) {
      return createContract(std::forward<M>(msg));
    } else {
      return callContract(std::forward<M>(msg));
    }
  }

  messages::Gas& currentGas() { return *currentGas_; }

private:
  decltype(auto) callContract(concepts::PackedMessage auto&& msg) {
    transactional::Group guard = {
      transactional::checkpoint(msg.caller().caller_),
      transactional::checkpoint(msg.caller().value_)
    };

    auto& contract = getContractAs<traits::MessageContract<decltype(msg)>>(msg.to());

    return std::apply([&] (auto&&... args) {
      if constexpr (concepts::StaticCallMessage<decltype(msg)>) {
        return std::invoke(msg.method(), contract, std::forward<decltype(args)>(args)...);
      } else {
        return contract.callContractFunction(&host_, msg.method(), std::forward<decltype(args)>(args)...);
      }
    }, std::forward<decltype(msg)>(msg).args());
  }

  decltype(auto) callContract(concepts::EncodedMessage auto&& msg) {
    const evmc_message evmcMsg{
      .kind = EVMC_CALL,
      .flags = (concepts::StaticCallMessage<decltype(msg)> ? EVMC_STATIC : evmc_flags(0)),
      .depth = 0,
      .gas = msg.gas().value(),
      .recipient = bytes::cast<evmc_address>(msg.to()),
      .sender = bytes::cast<evmc_address>(msg.from()),
      .input_data = msg.input().data(),
      .input_size = msg.input().size(),
      .value = evmc_uint256be{},
      .create2_salt = evmc_bytes32{},
      .code_address = bytes::cast<evmc_address>(msg.to())
    };

    auto& contract = context_.getContract(msg.to());
    contract.caller_ = Address(msg.from());
    contract.value_ = messageValueOrZero(msg);

    return contract.evmEthCall(evmcMsg, &host_);
  }

  Address createContract(concepts::CreateMessage auto&& msg) {
    using ContractType = traits::MessageContract<decltype(msg)>;

    const std::string createSignature = "createNew"
      + Utils::getRealTypeName<ContractType>()
      + "Contract("
      + ContractReflectionInterface::getConstructorArgumentTypesString<ContractType>()
      + ")";

    // We only need the first 4 bytes for the function signature
    Bytes fullData = Utils::makeBytes(Utils::sha3(bytes::view(createSignature)) | std::views::take(4));

    std::apply(Utils::Overloaded{
      [] () {},
      [&fullData] (const auto&... args) {
        Utils::appendBytes(fullData, ABI::Encoder::encodeData(args...));
      }
    }, msg.args());

    const Address& to = ProtocolContractAddresses.at("ContractManager");

    const evmc_message evmcMsg{
      .kind = EVMC_CREATE,
      .flags = 0,
      .depth = 1,
      .gas = msg.gas().value(),
      .recipient = bytes::cast<evmc_address>(to),
      .sender = bytes::cast<evmc_address>(msg.from()),
      .input_data = fullData.data(),
      .input_size = fullData.size(),
      .value = Utils::uint256ToEvmcUint256(msg.value()),
      .create2_salt{},
      .code_address = bytes::cast<evmc_address>(to)
    };

    auto& contract = context_.getContract(to);

    transactional::Group guard = {
      transactional::checkpoint(contract.caller_),
      transactional::checkpoint(contract.value_)
    };

    contract.caller_ = Address(msg.from()); // TODO: these copies can be avoided
    contract.value_ = 0;

    const Address contractAddress = generateContractAddress(context_.getAccount(msg.from()).nonce, msg.from());
    contract.ethCall(evmcMsg, &host_);
    context_.incrementNonce(msg.from());

    return contractAddress;
  }

  template<typename C>
  C& getContractAs(View<Address> address) {
    C* ptr = dynamic_cast<C*>(&context_.getContract(address));

    if (ptr == nullptr) {
      throw DynamicException("Wrong contract type"); // TODO: add more info
    }

    return *ptr;
  }

  ExecutionContext& context_;
  ContractHost& host_;
  messages::Gas *currentGas_ = nullptr;
};

#endif // BDK_MESSAGES_CPPCONTRACTEXECUTOR_H
