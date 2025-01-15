#ifndef BDK_MESSAGES_ANYENCODEDMESSAGEHANDLER_H
#define BDK_MESSAGES_ANYENCODEDMESSAGEHANDLER_H

#include "encodedmessages.h"

class AnyEncodedMessageHandler {
public:
  template<typename MessageHandler>
  static AnyEncodedMessageHandler from(MessageHandler& handler) {
    AnyEncodedMessageHandler anyHandler;

    const auto generic = [] (void *obj, auto& msg) { return static_cast<MessageHandler*>(obj)->onMessage(msg); };

    anyHandler.handler_ = &handler;
    anyHandler.onCreate_ = static_cast<Address(*)(void*, EncodedCreateMessage&)>(generic);
    anyHandler.onSaltCreate_ = static_cast<Address(*)(void*, EncodedSaltCreateMessage&)>(generic);
    anyHandler.onCall_ = static_cast<Bytes(*)(void*, EncodedCallMessage&)>(generic);
    anyHandler.onStaticCall_ = static_cast<Bytes(*)(void*, EncodedStaticCallMessage&)>(generic);
    anyHandler.onDelegateCall_ = static_cast<Bytes(*)(void*, EncodedDelegateCallMessage&)>(generic);

    return anyHandler;
  }

  Address onMessage(EncodedCreateMessage& msg) { return std::invoke(onCreate_, handler_, msg); }

  Address onMessage(EncodedSaltCreateMessage& msg) { return std::invoke(onSaltCreate_, handler_, msg); }

  Bytes onMessage(EncodedCallMessage& msg) { return std::invoke(onCall_, handler_, msg); }

  Bytes onMessage(EncodedStaticCallMessage& msg) { return std::invoke(onStaticCall_, handler_, msg); }

  Bytes onMessage(EncodedDelegateCallMessage& msg) { return std::invoke(onDelegateCall_, handler_, msg); }

private:
  void *handler_;
  Address (*onCreate_)(void*, EncodedCreateMessage&);
  Address (*onSaltCreate_)(void*, EncodedSaltCreateMessage&);
  Bytes (*onCall_)(void*, EncodedCallMessage&);
  Bytes (*onStaticCall_)(void*, EncodedStaticCallMessage&);
  Bytes (*onDelegateCall_)(void*, EncodedDelegateCallMessage&);
};

#endif // BDK_MESSAGES_ANYENCODEDMESSAGEHANDLER_H
