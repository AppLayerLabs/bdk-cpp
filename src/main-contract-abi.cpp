/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "utils/jsonabi.h"

int main() {
    return JsonAbi::writeContractsToJson<ContractTypes>();
}