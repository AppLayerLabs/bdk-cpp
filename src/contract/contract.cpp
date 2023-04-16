#include "contract.h"

void registerVariableUse(Contract& contract, SafeBase& variable) {
  contract.registerVariableUse(variable);
}