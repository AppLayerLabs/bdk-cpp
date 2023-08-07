#include "contract.h"

Address ContractGlobals::coinbase = Address(Hex::toBytes("0x0000000000000000000000000000000000000000"));
uint256_t ContractGlobals::blockHeight = 0;
uint256_t ContractGlobals::blockTimestamp = 0;