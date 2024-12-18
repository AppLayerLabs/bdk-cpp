#ifndef BDK_MESSAGES_TRAITS_H
#define BDK_MESSAGES_TRAITS_H

#include <type_traits>
#include "concepts.h"

namespace traits {

template<typename T>
struct Methods {};

template<typename C, typename R, typename... Ts>
struct Methods<R (C::*)(Ts...)> {
  using Return = R;
  using Class = C;
  static constexpr bool IS_VIEW = false;
};

template<typename C, typename R, typename... Ts>
struct Methods<R (C::*)(Ts...) const> {
  using Return = R;
  using Class = C;
  static constexpr bool IS_VIEW = true;
};

template<typename T>
using MethodReturn = typename Methods<std::remove_cvref_t<T>>::Return;

template<typename T>
using MethodClass = typename Methods<std::remove_cvref_t<T>>::Class;

template<typename T>
constexpr bool IsViewMethod = Methods<std::remove_cvref_t<T>>::IS_VIEW;

template<typename T>
struct MessageResultHelper;

template<concepts::CallMessage M>
  requires concepts::PackedMessage<M>
struct MessageResultHelper<M> {
  using Type = MethodReturn<decltype(std::declval<M>().method())>;
};

template<concepts::CallMessage M>
  requires concepts::EncodedMessage<M>
struct MessageResultHelper<M> {
  using Type = Bytes;
};

template<concepts::CreateMessage M>
struct MessageResultHelper<M> {
  using Type = Address;
};

template<typename T>
using MessageResult = typename MessageResultHelper<T>::Type;

template<typename M>
struct MessageContractHelper;

template<concepts::CallMessage M>
  requires (concepts::PackedMessage<M>)
struct MessageContractHelper<M> {
  using Type = MethodClass<decltype(std::declval<M>().method())>;
};

template<concepts::CreateMessage M>
  requires (concepts::PackedMessage<M>)
struct MessageContractHelper<M> {
  using Type = typename std::remove_cvref_t<M>::ContractType;
};

template<typename M>
using MessageContract = MessageContractHelper<M>::Type;

} // namespace traits

#endif // BDK_MESSAGES_TRAITS_H
