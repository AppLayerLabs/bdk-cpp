/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "finalizedblock.h"
#include "mutableblock.h"

FinalizedBlock FinalizedBlock::fromBytes(const BytesArrView bytes, const uint64_t& requiredChainId) {
  try {
    // Verify minimum size for a valid block
    if (bytes.size() < 217) throw std::runtime_error("Invalid block size - too short");
    // Parsing fixed-size fields
    Signature validatorSig = Signature(bytes.subspan(0, 65));
    Hash blockRandomness = Hash(bytes.subspan(97, 32));
    Hash validatorMerkleRoot = Hash(bytes.subspan(129, 32));
    Hash txMerkleRoot = Hash(bytes.subspan(161, 32));

    // Initialization for transaction counts is not required here
    // since they will be calculated during the deserialization process

    Logger::logToDebug(LogType::INFO, Log::finalizedBlock, __func__, "Deserializing block...");
    MutableBlock block(bytes, requiredChainId);

    Hash hash = Utils::sha3(block.serializeMutableHeader(validatorMerkleRoot, txMerkleRoot));
    UPubKey validatorPubKey = Secp256k1::recover(validatorSig, hash);
    return FinalizedBlock(
        std::move(validatorSig),
        std::move(validatorPubKey),
        std::move(block.getPrevBlockHash()),
        std::move(blockRandomness),
        std::move(validatorMerkleRoot),
        std::move(txMerkleRoot),
        block.getTimestamp(),
        block.getNHeight(),
        std::move(block.getTxValidators()),
        std::move(block.getTxs()),
        std::move(hash),
        bytes.size()
    );
  } catch (const std::exception &e) {
    Logger::logToDebug(LogType::ERROR, Log::finalizedBlock, __func__, "Error when deserializing a FinalizedBlock: " + std::string(e.what()));
    throw std::runtime_error(std::string("Error when deserializing a FinalizedBlock: ") + e.what());
  }
}

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