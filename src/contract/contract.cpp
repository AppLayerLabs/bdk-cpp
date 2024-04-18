/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "contract.h"
#include "contracthost.h"

Address ContractGlobals::coinbase_ = Address(Hex::toBytes("0x0000000000000000000000000000000000000000"));
Hash ContractGlobals::blockHash_ = Hash();
uint64_t ContractGlobals::blockHeight_ = 0;
uint64_t ContractGlobals::blockTimestamp_ = 0;

Address BaseContract::getOrigin() const {
  if (this->host_ == nullptr) {
    throw DynamicException("Contracts going haywire! trying to get origin without a host!");
  }
  return this->host_->get_tx_context().tx_origin;
}

uint64_t BaseContract::getNonce(const Address& address) const {
  if (this->host_ == nullptr) {
    throw DynamicException("Contracts going haywire! trying to get nonce without a host!");
  }
  return this->host_->getNonce(address);
}