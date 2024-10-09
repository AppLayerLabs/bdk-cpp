#pragma once

#include "../gas.h"
#include "message.h"

namespace evm {
  
class AnyCallHandler {
public:
  template<typename T>
  AnyCallHandler(T& callHandler)
    : object_(&callHandler),
      func_([] (void *object, kind::Any callKind, Gas& gas, const Message& msg) {
        return std::visit([&] (auto callKind) { return static_cast<T*>(object)->onCall(callKind, gas, msg); }, callKind);
      }) {}

  Bytes onCall(kind::Any callKind, Gas& gas, const Message& msg) {
    return std::invoke(func_, object_, callKind, gas, msg);
  }

private:
  void *object_;
  Bytes (*func_)(void*, kind::Any, Gas&, const Message&);
};

} // namespace evm
