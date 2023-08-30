#include "contract.h"

Address ContractGlobals::coinbase_ = Address(Hex::toBytes("0x0000000000000000000000000000000000000000"));
uint256_t ContractGlobals::blockHeight_ = 0;
uint256_t ContractGlobals::blockTimestamp_ = 0;