/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "dynamiccontract.h"

void registerVariableUse(DynamicContract& contract, SafeBase& variable) {
  contract.registerVariableUse(variable);
}

