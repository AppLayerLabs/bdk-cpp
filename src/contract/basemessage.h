#ifndef BDK_MESSAGES_BASEMESSAGE_H
#define BDK_MESSAGES_BASEMESSAGE_H

#include "bytes/range.h"
#include "utils/address.h"
#include "utils/hash.h"
#include "gas.h"

struct BaseContract;

template<typename T, typename... Ts>
struct BaseMessage : BaseMessage<T>, BaseMessage<Ts...> {
  template<typename U, typename... Us>
  constexpr BaseMessage(U&& first, Us&&... others)
    : BaseMessage<T>(std::forward<U>(first)),
      BaseMessage<Ts...>(std::forward<Us>(others)...) {}
};

template<typename T>
struct BaseMessage<T> : T {
  template<typename U>
  explicit constexpr BaseMessage(U&& first) : T(std::forward<U>(first)) {}
};

class FromField {
public:
  template<bytes::BorrowedDataRange R>
    requires std::convertible_to<R, View<Address>>
  explicit constexpr FromField(R&& range) : from_(range) {}

  constexpr View<Address> from() const { return from_; }

private:
  View<Address> from_;
};

class ToField {
public:
  template<bytes::BorrowedDataRange R>
    requires std::convertible_to<R, View<Address>>
  explicit constexpr ToField(R&& range) : to_(range) {}

  constexpr View<Address> to() const { return to_; }

private:
  View<Address> to_;
};

class GasField {
public:
  explicit constexpr GasField(Gas& gas) : gas_(gas) {}
  constexpr Gas& gas() { return gas_; }
  constexpr const Gas& gas() const { return gas_; }

private:
  Gas& gas_;
};

class ValueField {
public:
  explicit constexpr ValueField(const uint256_t& value) : value_(value) {}
  constexpr const uint256_t& value() const { return value_; }

private:
  const uint256_t& value_;
};

class InputField {
public:
  explicit constexpr InputField(View<Bytes> input) : input_(input) {}
  constexpr View<Bytes> input() const { return input_; }

private:
  View<Bytes> input_;
};

class CodeField {
public:
  explicit constexpr CodeField(View<Bytes> code) : code_(code) {}
  constexpr View<Bytes> code() const { return code_; }

private:
  View<Bytes> code_;
};

class SaltField {
public:
  template<bytes::BorrowedDataRange R>
    requires std::convertible_to<R, View<Hash>>
  explicit constexpr SaltField(R&& range) : salt_(range) {}
  constexpr View<Hash> salt() const { return salt_; }

private:
  View<Hash> salt_;
};

class CodeAddressField {
public:
  template<bytes::BorrowedDataRange R>
    requires std::convertible_to<R, View<Address>>
  explicit constexpr CodeAddressField(R&& range) : codeAddress_(range) {}
  constexpr View<Address> codeAddress() const { return codeAddress_; }

private:
  View<Address> codeAddress_;
};

template<typename M>
class MethodField {
public:
  explicit constexpr MethodField(M method) : method_(method) {}
  constexpr M& method() { return method_; }
  constexpr const M& method() const { return method_; }

private:
  M method_;
};

template<typename... Args>
class ArgsField {
public:
  explicit constexpr ArgsField(Args&&... args) : args_(std::forward<Args>(args)...) {}
  constexpr std::tuple<Args...>& args() & { return args_; }
  constexpr std::tuple<Args...>&& args() && { return std::move(args_); }
  constexpr const std::tuple<Args...>& args() const& { return args_; }

private:
  std::tuple<Args...> args_;
};

template<typename... Args>
ArgsField(Args&&...) -> ArgsField<Args...>;

template<typename... Args>
struct BaseMessage<ArgsField<Args...>> : ArgsField<Args...> {
  explicit constexpr BaseMessage(auto&&... args)
    : ArgsField<Args...>(std::forward<decltype(args)>(args)...) {}
};

#endif // BDK_MESSAGES_BASEMESSAGE_H
