#ifndef BDK_PACKEDMESSAGES_H
#define BDK_PACKEDMESSAGES_H

#include "contract/concepts.h"
#include "contract/basemessage.h"

template<typename M, typename... Args>
struct PackedCallMessage : BaseMessage<
  FromField,
  ToField,
  GasField,
  ValueField,
  MethodField<M>, 
  ArgsField<Args...>> {
  
  using BaseMessage<FromField, ToField, GasField, ValueField, MethodField<M>, ArgsField<Args...>>::BaseMessage;
};

template<typename M, typename... Args>
struct PackedStaticCallMessage : BaseMessage<
  FromField,
  ToField,
  GasField,
  MethodField<M>, 
  ArgsField<Args...>> {
  
  using BaseMessage<FromField, ToField, GasField, MethodField<M>, ArgsField<Args...>>::BaseMessage;
};

template<typename C, typename... Args>
struct PackedCreateMessage : BaseMessage<FromField, GasField, ValueField, ArgsField<Args...>> {
  using BaseMessage<FromField, GasField, ValueField, ArgsField<Args...>>::BaseMessage;
  using ContractType = C;
};

#endif // BDK_PACKEDMESSAGES_H
