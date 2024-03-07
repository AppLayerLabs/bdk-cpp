/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "finalizedblock.h"

Bytes FinalizedBlock::serializeHeader() const {
  Bytes ret;
  ret.reserve(144);
  ret.insert(ret.end(), this->prevBlockHash_.cbegin(), this->prevBlockHash_.cend());
  ret.insert(ret.end(), this->blockRandomness_.cbegin(), this->blockRandomness_.cend());
  ret.insert(ret.end(), this->validatorMerkleRoot_.cbegin(), this->validatorMerkleRoot_.cend());
  ret.insert(ret.end(), this->txMerkleRoot_.cbegin(), this->txMerkleRoot_.cend());
  Utils::appendBytes(ret, Utils::uint64ToBytes(this->timestamp_));
  Utils::appendBytes(ret, Utils::uint64ToBytes(this->nHeight_));
  return ret;
}

Bytes FinalizedBlock::serializeBlock() const {
  Bytes ret;
  ret.insert(ret.end(), this->validatorSig_.cbegin(), this->validatorSig_.cend());
  Utils::appendBytes(ret, this->serializeHeader());

  // Fill in the txValidatorStart with 0s for now, keep track of the index
  uint64_t txValidatorStartLoc = ret.size();
  ret.insert(ret.end(), 8, 0x00);

  // Serialize the transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txs_) {
    Bytes txBytes = tx.rlpSerialize();
    Utils::appendBytes(ret, Utils::uint32ToBytes(txBytes.size()));
    ret.insert(ret.end(), txBytes.begin(), txBytes.end());
  }

  // Insert the txValidatorStart
  BytesArr<8> txValidatorStart = Utils::uint64ToBytes(ret.size());
  std::memcpy(&ret[txValidatorStartLoc], txValidatorStart.data(), 8);

  // Serialize the Validator Transactions [4 Bytes + Tx Bytes]
  for (const auto &tx : this->txValidators_) {
    Bytes txBytes = tx.rlpSerialize();
    Utils::appendBytes(ret, Utils::uint32ToBytes(txBytes.size()));
    ret.insert(ret.end(), txBytes.begin(), txBytes.end());
  }

  return ret;
}