#include "contract.h"

void registerVariableUse(DynamicContract& contract, SafeBase& variable) {
  contract.registerVariableUse(variable);
}