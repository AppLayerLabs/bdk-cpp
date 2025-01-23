#ifndef BDK_MESSAGES_CONCEPTS_H
#define BDK_MESSAGES_CONCEPTS_H

#include "utils/strings.h"
#include "gas.h"

namespace concepts {

/**
 * Basic message concept. Every message has as sender address and a gas reference to be consumed.
 */
template<typename M>
concept Message = requires (M m) {
  { m.from() } -> std::convertible_to<View<Address>>;
  { m.gas() } -> std::convertible_to<Gas&>;
};

/**
 * A message that also has value. Often used to describe composed concepts.
 */
template<typename M>
concept HasValueField = requires (M m) {
  { m.value() } -> std::convertible_to<const uint256_t&>;
};

template<typename M>
concept HasInputField = requires (M m) {
  { m.input() } -> std::convertible_to<View<Bytes>>;
};

template<typename M>
concept HasCodeField = requires (M m) {
  { m.code() } -> std::convertible_to<View<Bytes>>;
};

/**
 * A message that also has recipient address. Often used to describe composed concepts.
 */
template<typename M>
concept HasToField = requires (M m) {
  { m.to() } -> std::convertible_to<View<Address>>;
};

/**
 * A message aimed to create an address. In general, contract creation messages
 * do not have a target address (i.e. recipient) but can have value.
 */
template<typename M>
concept CreateMessage = Message<M> && HasValueField<M> && !HasToField<M>;

/**
 * Call messages can have value and must have a target address.
 */
template<typename M>
concept CallMessage = Message<M> && HasToField<M>;

/**
 * Static call messages are messages that calls const/view functions. They
 * can't have value but (as any call message) must have a recipient address
 */
template<typename M>
concept StaticCallMessage = CallMessage<M> && !HasValueField<M>;

/**
 * Solution based on the C++ standard: std::ranges::enable_borrowed_range.
 * In short, delegate calls have the exact same structure of a normal call,
 * but with different intention. Thus, any message that represents a
 * delegate message must specialize the EnableDelegate as a true_type,
 * allowing the DelegateCallMessage concept to be chosen during overload
 * resolution.
 */
template<typename M>
constexpr bool EnableDelegate = false;

template<typename M>
constexpr bool EnableCallCode = false;

/**
 * Concept of delegate call messages.
 */
template<typename M>
concept DelegateCallMessage = CallMessage<M> && requires (M m) {
  { m.codeAddress() } -> std::convertible_to<View<Address>>;
};

/**
 * Concept of delegate call messages.
 */
template<typename M>
concept CallCodeMessage = CallMessage<M> && EnableCallCode<std::remove_cvref_t<M>>;

template<typename M>
concept SaltMessage = CreateMessage<M> && requires (M m) {
  { m.salt() } -> std::convertible_to<View<Hash>>;
};

template<typename M>
concept EncodedMessage = (CallMessage<M> && HasInputField<M>) || (CreateMessage<M> && HasCodeField<M>);

template<typename M>
concept PackedMessage = Message<M> && requires (M m) {
  m.args();
};

} // namespace concepts

#endif // BDK_MESSAGES_CONCEPTS_H
