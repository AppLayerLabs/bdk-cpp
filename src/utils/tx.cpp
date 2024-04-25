/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "tx.h"

TxBlock::TxBlock(const BytesArrView bytes, const uint64_t&) {
  uint64_t index = 0;
  const auto txData = bytes.subspan(1);

  // Check if Tx is type 2
  if (bytes[0] != 0x02) throw DynamicException("Tx is not type 2");

  // Check if first byte is equal or higher than 0xf7, meaning it is a list
  if (txData[0] < 0xf7) throw DynamicException("Tx is not a list");

  // Get list length
  uint8_t listLengthSize = txData[index] - 0xf7;
  index++;
  uint64_t listLength = Utils::fromBigEndian<uint64_t>(
    txData.subspan(index, listLengthSize)
  );
  index += listLengthSize; // Index is now at rlp[0] size

  // Size sanity check
  if (listLength < txData.size() - listLengthSize - 1) {
    throw DynamicException("Tx RLP returns smaller size than reported");
  } else if (listLength > txData.size() - listLengthSize - 1) {
    throw DynamicException("Tx RLP returns larger size than reported");
  }

  // If chainId > 0, get chainId from string.
  // chainId can be a small string or the byte itself
  const uint8_t chainIdLength = (txData[index] >= 0x80)
                              ? txData[index] - 0x80 : 0;
  if (chainIdLength != 0) {
    if (chainIdLength > 0x37) throw DynamicException("ChainId is too large");
    index++; // Index at rlp[0] payload
    this->chainId_ = Utils::fromBigEndian<uint64_t>(
      txData.subspan(index, chainIdLength)
    );
    index += chainIdLength; // Index at rlp[1] size
  } else {
    this->chainId_ = (txData[index] == 0x80)
                  ? 0 : Utils::fromBigEndian<uint64_t>(txData.subspan(index, 1));
    index++; // Index at rlp[1] size
  }

  // If nonce > 0, get nonce from string.
  // nonce can be a small string or the byte itself
  const uint8_t nonceLength = (txData[index] >= 0x80)
                                ? txData[index] - 0x80 : 0;
  if (nonceLength != 0) {
    if (nonceLength > 0x37) throw DynamicException("Nonce is too large");
    index++; // Index at rlp[1] payload
    this->nonce_ = Utils::fromBigEndian<uint64_t>(
      txData.subspan(index, nonceLength)
    );
    index += nonceLength; // Index at rlp[2] size
  } else {
    this->nonce_ = (txData[index] == 0x80)
                    ? 0 : Utils::fromBigEndian<uint64_t>(txData.subspan(index, 1));
    index++; // Index at rlp[2] size
  }

  // If maxPriorityFeePerGas > 0, get maxPriorityFeePerGas from string.
  // maxPriorityFeePerGas can be a small string or the byte itself
  const uint8_t maxPriorityFeePerGasLength = txData[index] >= 0x80
                              ? txData[index] - 0x80 : 0;
  if (maxPriorityFeePerGasLength != 0) {
    if (maxPriorityFeePerGasLength > 0x37) throw DynamicException("MaxPriorityFeePerGas is too large");
    index++; // Index at rlp[2] payload
    this->maxPriorityFeePerGas_ = Utils::fromBigEndian<uint256_t>(
      txData.subspan(index, maxPriorityFeePerGasLength)
    );
    index += maxPriorityFeePerGasLength; // Index at rlp[3] size
  } else {
    this->maxPriorityFeePerGas_ = txData[index] == 0x80
                  ? 0 : Utils::fromBigEndian<uint256_t>(txData.subspan(index, 1));
    index++; // Index at rlp[3] size
  }

  // If maxFeePerGas > 0, get nonce from string.
  // maxFeePerGas can be a small string or the byte itself
  const uint8_t maxFeePerGasLength = txData[index] >= 0x80
                                             ? txData[index] - 0x80 : 0;
  if (maxFeePerGasLength != 0) {
    if (maxFeePerGasLength > 0x37) throw DynamicException("MaxFeePerGas is too large");
    index++; // Index at rlp[3] payload
    this->maxFeePerGas_ = Utils::fromBigEndian<uint256_t>(
      txData.subspan(index, maxFeePerGasLength)
    );
    index += maxFeePerGasLength; // Index at rlp[4] size
  } else {
    this->maxFeePerGas_ = txData[index] == 0x80
                                 ? 0 : Utils::fromBigEndian<uint256_t>(txData.subspan(index, 1));
    index++; // Index at rlp[4] size
  }

  // If gasLimit > 0, get gasLimit from string.
  // gasLimit can be a small string or the byte itself
  const uint8_t gasLimitLength = txData[index] >= 0x80
                                ? txData[index] - 0x80 : 0;
  if (gasLimitLength != 0) {
    if (gasLimitLength > 0x37) throw DynamicException("GasLimit is too large");
    index++; // Index at rlp[0] payload
    this->gasLimit_ = Utils::fromBigEndian<uint64_t>(
      txData.subspan(index, gasLimitLength)
    );
    index += gasLimitLength; // Index at rlp[1] size
  } else {
    this->gasLimit_ = txData[index] == 0x80
                    ? 0 : Utils::fromBigEndian<uint64_t>(txData.subspan(index, 1));
    index++; // Index at rlp[1] size
  }

  // Get receiver addess (to) - small string.
  // We don't actually need to get the size, because to/from has a size of 20
  if (txData[index] != 0x94) throw DynamicException(
    "Receiver address (to) is not a 20 byte string (address)"
  );
  index++; // Index at rlp[5] payload
  this->to_ = Address(txData.subspan(index, 20));
  index += 20; // Index at rlp[6] size

  // Get value - small string or byte itself.
  uint8_t valueLength = txData[index] >= 0x80 ? txData[index] - 0x80 : 0;
  if (valueLength != 0) {
    if (valueLength > 0x37) throw DynamicException("Value is not a small string");
    index++; // Index at rlp[6] payload
    this->value_ = Utils::fromBigEndian<uint256_t>(
      txData.subspan(index, valueLength)
    );
    index += valueLength; // Index at rlp[7] size
  } else {
    this->value_ = txData[index] == 0x80
      ? 0 : Utils::fromBigEndian<uint256_t>(txData.subspan(index, 1));
    index++; // Index at rlp[7] size
  }

  // Get data - it can be anything really, from nothing (0x80) to a big string (0xb7)
  if (uint8_t(txData[index]) < 0x80) {
    this->data_.assign(txData.begin() + index, txData.begin() + index + 1);
    index++; // Index at rlp[8] payload
  } else if (uint8_t(txData[index]) < 0xb7) {
    uint8_t dataLength = txData[index] - 0x80;
    index++; // Index at rlp[8] payload
    if (dataLength > 0) {
      this->data_.assign(txData.begin() + index, txData.begin() + index + dataLength);
      index += dataLength; // Index at rlp[8] size
    }
  } else if (uint8_t(txData[index]) < 0xc0) {
    uint8_t dataLengthSize = txData[index] - 0xb7;
    index++; // Index at rlp[8] payload size
    uint64_t dataLength = Utils::fromBigEndian<uint64_t>(
      txData.subspan(index, dataLengthSize)
    );
    index += dataLengthSize; // Index at rlp[8] payload
    this->data_.assign(txData.begin() + index, txData.begin() + index + dataLength);
    index += dataLength; // Index at rlp[8] size
  } else {
    throw DynamicException("Data is too large");
  }

  // Get access list
  // ALWAYS 0xc0 (empty list)
  if (txData[index] != 0xc0) throw DynamicException("Access list is not empty");
  index++; // Index at rlp[9] size

  // Get v - always byte itself (1 byte)
  if (txData[index] == 0x80) {
    this->v_ = 0;
  } else {
    this->v_ = Utils::fromBigEndian<uint8_t>(txData.subspan(index, 1));
    if (this->v_ > 0x01) throw DynamicException("V is not 0 or 1");
  }
  ++index; // Index at rlp[10] size

  // Get r - small string
  uint8_t rLength = txData[index] - 0x80;
  if (rLength > 0x20) throw DynamicException("R is bigger than 32 bytes");
  index++; // Index at rlp[10] payload
  this->r_ = Utils::fromBigEndian<uint256_t>(txData.subspan(index, rLength));
  index += rLength; // Index at rlp[11] size

  // Get s - small string
  uint8_t sLength = txData[index] - 0x80;
  if (sLength > 0x20) throw DynamicException("S is bigger than 32 bytes");
  index++; // Index at rlp[11] payload
  this->s_ = Utils::fromBigEndian<uint256_t>(txData.subspan(index, sLength));
  index += sLength; // Index at rlp[12] size

  if (!Secp256k1::verifySig(this->r_, this->s_, this->v_)) {
    throw DynamicException("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  Signature sig = Secp256k1::makeSig(this->r_, this->s_, this->v_);
  Hash msgHash = Utils::sha3(this->rlpSerialize(false)); // Do not include signature
  UPubKey key = Secp256k1::recover(sig, msgHash);
  if (!key) throw DynamicException("Invalid tx signature - cannot recover public key");

  this->from_ = Secp256k1::toAddress(key);
  this->hash_ = Utils::sha3(this->rlpSerialize(true)); // Include signature
}

TxBlock::TxBlock(
  const Address& to, const Address& from, const Bytes& data,
  const uint64_t& chainId, const uint256_t& nonce, const uint256_t& value,
  const uint256_t& maxPriorityFeePerGas, const uint256_t& maxFeePerGas, const uint256_t& gasLimit, const PrivKey& privKey
) : to_(to), from_(from), data_(data), chainId_(chainId), nonce_(nonce),
  value_(value), maxPriorityFeePerGas_(maxPriorityFeePerGas), maxFeePerGas_(maxFeePerGas), gasLimit_(gasLimit)
{
  UPubKey pubKey = Secp256k1::toUPub(privKey);
  Address add = Secp256k1::toAddress(pubKey);
  if (add != this->from_) throw DynamicException("Private key does not match sender address (from)");

  Hash msgHash = Utils::sha3(this->rlpSerialize(false)); // Do not include signature
  Signature sig = Secp256k1::sign(msgHash, privKey);
  this->r_ = Utils::bytesToUint256(sig.view(0, 32));
  this->s_ = Utils::bytesToUint256(sig.view(32,32));
  this->v_ = sig[64];

  if (pubKey != Secp256k1::recover(sig, msgHash)) {
    throw DynamicException("Invalid tx signature - derived key doesn't match public key");
  }
  if (!Secp256k1::verifySig(this->r_, this->s_, this->v_)) {
    throw DynamicException("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  this->hash_ = Utils::sha3(this->rlpSerialize(true)); // Include signature
}

Bytes TxBlock::rlpSerialize(bool includeSig) const {
  Bytes ret = { 0x02 };
  uint64_t total_size = 0;
  uint64_t reqBytesChainId = Utils::bytesRequired(this->chainId_);
  uint64_t reqBytesNonce = Utils::bytesRequired(this->nonce_);
  uint64_t reqBytesMaxPriorityFeePerGas = Utils::bytesRequired(this->maxPriorityFeePerGas_);
  uint64_t reqBytesMaxFeePerGas = Utils::bytesRequired(this->maxFeePerGas_);
  uint64_t reqBytesGasLimit = Utils::bytesRequired(this->gasLimit_);
  uint64_t reqBytesValue = Utils::bytesRequired(this->value_);
  uint64_t reqBytesData = this->data_.size();
  uint64_t reqBytesR = Utils::bytesRequired(this->r_);
  uint64_t reqBytesS = Utils::bytesRequired(this->s_);

  // Calculate total sizes
  total_size += (this->chainId_ < 0x80) ? 1 : 1 + reqBytesChainId;
  total_size += (this->nonce_ < 0x80) ? 1 : 1 + reqBytesNonce;
  total_size += (this->maxPriorityFeePerGas_ < 0x80) ? 1 : 1 + reqBytesMaxPriorityFeePerGas;
  total_size += (this->maxFeePerGas_ < 0x80) ? 1 : 1 + reqBytesMaxFeePerGas;
  total_size += (this->gasLimit_ < 0x80) ? 1 : 1 + reqBytesGasLimit;
  total_size += 1 + 20; // To
  total_size += (this->value_ < 0x80) ? 1 : 1 + reqBytesValue;
  total_size += 1; // Access List

  if (this->data_.size() == 0) {
    total_size += 1;
  } else if (reqBytesData <= 55) {
    total_size += 1 + reqBytesData;
  } else {
    total_size += 1 + Utils::bytesRequired(reqBytesData) + reqBytesData;
  }

  if (includeSig) {
    total_size += 1; // V
    total_size += 1 + reqBytesR;
    total_size += 1 + reqBytesS;
  }

  // Serialize everything
  if (total_size <= 55) {
    ret.reserve(total_size + 1);
    ret.insert(ret.end(), char(total_size + 0xc0));
  } else {
    uint64_t sizeBytes = Utils::bytesRequired(total_size);
    ret.reserve(total_size + sizeBytes + 1);
    ret.insert(ret.end(), char(sizeBytes + 0xf7));
    Utils::appendBytes(ret, Utils::uintToBytes(total_size));
  }

  // chainId
  if (this->chainId_ == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (this->chainId_ < 0x80) {
    ret.insert(ret.end(), char(this->chainId_));
  } else {
    ret.insert(ret.end(), char(reqBytesChainId + 0x80));
    Utils::appendBytes(ret, Utils::uintToBytes(this->chainId_));
  }
  // Nonce
  if (this->nonce_ == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (this->nonce_ < 0x80) {
    ret.insert(ret.end(), char(this->nonce_));
  } else {
    ret.insert(ret.end(), char(reqBytesNonce + 0x80));
    Utils::appendBytes(ret, Utils::uintToBytes(this->nonce_));
  }

  // max Priority Fee Per Gas
  if (this->maxPriorityFeePerGas_ == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (this->maxPriorityFeePerGas_ < 0x80) {
    ret.insert(ret.end(), char(this->maxPriorityFeePerGas_));
  } else {
    ret.insert(ret.end(), char(reqBytesMaxPriorityFeePerGas + 0x80));
    Utils::appendBytes(ret, Utils::uintToBytes(this->maxPriorityFeePerGas_));
  }

  // max Fee Per Gas
  if (this->maxFeePerGas_ == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (this->maxFeePerGas_ < 0x80) {
    ret.insert(ret.end(), char(this->maxFeePerGas_));
  } else {
    ret.insert(ret.end(), char(reqBytesMaxFeePerGas + 0x80));
    Utils::appendBytes(ret, Utils::uintToBytes(this->maxFeePerGas_));
  }

  // Gas Limit
  if (this->gasLimit_ == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (this->gasLimit_ < 0x80) {
    ret.insert(ret.end(), char(this->gasLimit_));
  } else {
    ret.insert(ret.end(), char(reqBytesGasLimit + 0x80));
    Utils::appendBytes(ret, Utils::uintToBytes(this->gasLimit_));
  }

  // To
  ret.insert(ret.end(), 0x94);
  Utils::appendBytes(ret, this->to_.get());

  // Value
  if (this->value_ == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (this->value_ < 0x80) {
    ret.insert(ret.end(), char(this->value_));
  } else {
    ret.insert(ret.end(), char(reqBytesValue + 0x80));
    Utils::appendBytes(ret, Utils::uintToBytes(this->value_));
  }

  // Data
  if (this->data_.size() == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (reqBytesData <= 55) {
    ret.insert(ret.end(), char(reqBytesData + 0x80));
    ret.insert(ret.end(), this->data_.begin(), this->data_.end());
  } else {
    ret.insert(ret.end(), char(Utils::bytesRequired(reqBytesData) + 0xb7));
    Utils::appendBytes(ret, Utils::uintToBytes(reqBytesData));
    ret.insert(ret.end(), this->data_.begin(), this->data_.end());
  }

  // Access List
  ret.insert(ret.end(), 0xc0);

  // V/chainId
  if (includeSig) {
    if (this->v_ == 0x00) {
      ret.insert(ret.end(), 0x80);
    } else {
      ret.insert(ret.end(), this->v_);
    }
  }

  // R
  if (includeSig) {
    ret.insert(ret.end(), char(reqBytesR + 0x80));
    Utils::appendBytes(ret, Utils::uintToBytes(this->r_));
  }

  // S
  if (includeSig) {
    ret.insert(ret.end(), char(reqBytesS + 0x80));
    Utils::appendBytes(ret, Utils::uintToBytes(this->s_));
  }
  return ret;
}

evmc_message TxBlock::txToMessage() const {
  // evmc_message:
  // struct evmc_message
  // {
  //   enum evmc_call_kind kind;
  //   uint32_t flags;
  //   int32_t depth;
  //   int64_t gas;
  //   evmc_address recipient;
  //   evmc_address sender;
  //   const uint8_t* input_data;
  //   size_t input_size;
  //   evmc_uint256be value;
  //   evmc_bytes32 create2_salt;
  //   evmc_address code_address;
  // };
  evmc_message msg;
  if (this->to_ == Address())
    msg.kind = EVMC_CREATE;
  else
    msg.kind = EVMC_CALL;
  msg.flags = 0;
  msg.depth = 1;
  msg.gas = static_cast<int64_t>(this->gasLimit_);
  msg.recipient = this->to_.toEvmcAddress();
  msg.sender = this->from_.toEvmcAddress();
  msg.input_data = (this->data_.empty()) ? nullptr : this->data_.data();
  msg.input_size = this->data_.size();
  msg.value = Utils::uint256ToEvmcUint256(this->value_);
  msg.create2_salt = {};
  msg.code_address = this->to_.toEvmcAddress();
  return msg;
}

TxValidator::TxValidator(const BytesArrView bytes, const uint64_t&) {
  uint64_t index = 0;

  // Check if first byte is equal or higher than 0xf7, meaning it is a list
  if (bytes[0] < 0xf7) throw DynamicException("Tx is not a list");

  // Get list length
  uint8_t listLengthSize = bytes[index] - 0xf7;
  index++;
  uint64_t listLength = Utils::fromBigEndian<uint64_t>(
    bytes.subspan(index, listLengthSize)
  );
  index += listLengthSize; // Index is now at rlp[0] size

  // Size sanity check
  if (listLength < bytes.size() - listLengthSize - 1) {
    throw DynamicException("Tx RLP returns smaller size than reported");
  } else if (listLength > bytes.size() - listLengthSize - 1) {
    throw DynamicException("Tx RLP returns larger size than reported");
  }

  // Get data - it can be anything really, from nothing (0x80) to a big string (0xb7)
  if (uint8_t(bytes[index]) < 0x80) {
    this->data_.assign(bytes.begin() + index, bytes.begin() + index + 1);
    index++; // Index at rlp[1] size
  } else if (bytes[index] < 0xb7) {
    uint8_t dataLength = bytes[index] - 0x80;
    index++; // Index at rlp[0] payload
    if (dataLength > 0) {
      this->data_.assign(bytes.begin() + index, bytes.begin() + index + dataLength);
      index += dataLength; // Index at rlp[1] size
    }
  } else {
    uint8_t dataLengthSize = bytes[index] - 0xb7;
    index++; // Index at rlp[0] payload size
    uint64_t dataLength = Utils::fromBigEndian<uint64_t>(
      bytes.subspan(index, dataLengthSize)
    );
    index += dataLengthSize; // Index at rlp[0] payload
    this->data_.assign(bytes.begin() + index, bytes.begin() + index + dataLength);
    index += dataLength; // Index at rlp[1] size
  }

  // Get nHeight - can be a small string or the byte itself
  const uint8_t nHeightLength = (bytes[index] >= 0x80) ? bytes[index] - 0x80 : 0;
  if (nHeightLength != 0) {
    index++; // Index at rlp[1] payload
    this->nHeight_ = Utils::fromBigEndian<uint64_t>(
      bytes.subspan(index, nHeightLength)
    );
    index += nHeightLength; // Index at rlp[2] size
  } else {
    this->nHeight_ = (bytes[index] == 0x80)
      ? 0 : Utils::fromBigEndian<uint64_t>(bytes.subspan(index, 1));
    index++; // Index at rlp[2] size
  }

  // Get v - small string or the byte itself
  uint8_t vLength = (bytes[index] >= 0x80) ? bytes[index] - 0x80 : 0;
  if (vLength != 0) {
    if (vLength > 0x37) throw DynamicException("V is not a small string");
    index++; // Index at rlp[2] payload
    this->v_ = Utils::fromBigEndian<uint256_t>(
      bytes.subspan(index, vLength)
    );
    index += vLength; // Index at rlp[3] size
  } else {
    this->v_ = (bytes[index] == 0x80)
      ? 0 : Utils::fromBigEndian<uint256_t>(bytes.subspan(index, 1));
    index++; // Index at rlp[3] size
  }

  // Get r - small string
  uint8_t rLength = bytes[index] - 0x80;
  if (rLength > 0x37) throw DynamicException("R is not a small string");
  index++; // Index at rlp[3] payload
  this->r_ = Utils::fromBigEndian<uint256_t>(bytes.subspan(index, rLength));
  index += rLength; // Index at rlp[4] size

  // Get s - small string
  uint8_t sLength = bytes[index] - 0x80;
  if (sLength > 0x37) throw DynamicException("S is not a small string");
  index++; // Index at rlp[4] payload
  this->s_ = Utils::fromBigEndian<uint256_t>(bytes.subspan(index, sLength));
  index += sLength; // Index at rlp[5] size. rlp[5] doesn't exist on Validator txs

  // Get chainId - calculated from v
  if (this->v_ > 36) {
    this->chainId_ = static_cast<uint64_t>((this->v_ - 35) / 2);
    if (this->chainId_ > std::numeric_limits<uint64_t>::max()) {
      throw DynamicException("chainId too high");
    }
  } else if (this->v_ != 27 && this->v_ != 28) {
    throw DynamicException("Invalid tx signature - v is not 27 or 28, v is "
      + boost::lexical_cast<std::string>(this->v_));
  }

  // Get recoveryId, verify the signature and derive sender address (from)
  auto recoveryId = uint8_t{this->v_ - (uint256_t(this->chainId_) * 2 + 35)};
  if (!Secp256k1::verifySig(this->r_, this->s_, recoveryId)) {
    throw DynamicException("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  Signature sig = Secp256k1::makeSig(this->r_, this->s_, recoveryId);
  Hash msgHash = Utils::sha3(this->rlpSerialize(false)); // Do not include signature
  UPubKey key = Secp256k1::recover(sig, msgHash);
  if (key == UPubKey()) throw DynamicException("Invalid tx signature - cannot recover public key");
  this->from_ = Secp256k1::toAddress(key);
  this->hash_ = Utils::sha3(this->rlpSerialize(true)); // Include signature
}

TxValidator::TxValidator(
  const Address& from, const Bytes& data, const uint64_t& chainId,
  const uint64_t& nHeight, const PrivKey& privKey
) : from_(from), data_(data), chainId_(chainId), nHeight_(nHeight) {
  UPubKey pubKey = Secp256k1::toUPub(privKey);
  Address add = Secp256k1::toAddress(pubKey);
  Hash msgHash = Utils::sha3(this->rlpSerialize(false)); // Do not include signature
  if (add != this->from_) throw DynamicException("Private key does not match sender address (from)");

  Signature sig = Secp256k1::sign(msgHash, privKey);
  this->r_ = Utils::bytesToUint256(sig.view(0, 32));
  this->s_ = Utils::bytesToUint256(sig.view(32,32));
  uint8_t recoveryIds = sig[64];
  this->v_ = recoveryIds + (this->chainId_ * 2 + 35);

  if (!Secp256k1::verifySig(this->r_, this->s_, recoveryIds)) {
    throw DynamicException("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  if (pubKey != Secp256k1::recover(sig, msgHash)) {
    throw DynamicException("Invalid transaction signature, signature derived key doens't match public key");
  }
  this->hash_ = Utils::sha3(this->rlpSerialize(true)); // Include signature
}

Bytes TxValidator::rlpSerialize(bool includeSig) const {
  Bytes ret;
  uint64_t total_size = 0;
  uint64_t reqBytesData = this->data_.size();
  uint64_t reqBytesnHeight = Utils::bytesRequired(this->nHeight_);
  uint64_t reqBytesV = Utils::bytesRequired(includeSig ? this->v_ : this->chainId_);
  uint64_t reqBytesR = Utils::bytesRequired(this->r_);
  uint64_t reqBytesS = Utils::bytesRequired(this->s_);

  // Calculate total sizes
  if (this->data_.size() == 0) {
    total_size += 1;
  } else if (reqBytesData <= 55) {
    total_size += 1 + reqBytesData;
  } else {
    total_size += 1 + Utils::bytesRequired(reqBytesData) + reqBytesData;
  }

  total_size += (this->nHeight_ < 0x80) ? 1 : 1 + reqBytesnHeight;

  if (includeSig) {
    total_size += (this->v_ < 0x80) ? 1 : 1 + reqBytesV;
  } else {
    total_size += (this->chainId_ < 0x80) ? 1 : 1 + reqBytesV;
  }

  total_size += (!includeSig) ? 1 : 1 + reqBytesR;
  total_size += (!includeSig) ? 1 : 1 + reqBytesS;

  // Serialize everything
  if (total_size <= 55) {
    ret.reserve(total_size + 1);
    ret.insert(ret.end(), total_size + 0xc0);
  } else {
    uint64_t sizeBytes = Utils::bytesRequired(total_size);
    ret.reserve(total_size + sizeBytes + 1);
    ret.insert(ret.end(), sizeBytes + 0xf7);
    Utils::appendBytes(ret, Utils::uintToBytes(total_size));
  }

  // Data
  if (this->data_.size() == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (reqBytesData <= 55) {
    ret.insert(ret.end(), reqBytesData + 0x80);
    ret.insert(ret.end(), this->data_.begin(), this->data_.end());
  } else {
    ret.insert(ret.end(), char(Utils::bytesRequired(reqBytesData) + 0xb7));
    Utils::appendBytes(ret, Utils::uintToBytes(reqBytesData));
    ret.insert(ret.end(), this->data_.begin(), this->data_.end());
  }

  // nHeight
  if (this->nHeight_ == 0) {
    ret.insert(ret.end(), 0x80);
  } else if (this->nHeight_ < 0x80) {
    ret.insert(ret.end(), this->nHeight_);
  } else {
    ret.insert(ret.end(), reqBytesnHeight + 0x80);
    Utils::appendBytes(ret, Utils::uintToBytes(this->nHeight_));
  }

  // V/chainId
  if (includeSig) {
    if (this->v_ < 0x80) {
      ret.insert(ret.end(), uint8_t(this->v_));
    } else {
      ret.insert(ret.end(), reqBytesV + 0x80);
      Utils::appendBytes(ret, Utils::uintToBytes(this->v_));
    }
  } else {
    if (this->chainId_ < 0x80) {
      ret.insert(ret.end(), this->chainId_);
    } else {
      ret.insert(ret.end(), reqBytesV + 0x80);
      Utils::appendBytes(ret, Utils::uintToBytes(this->chainId_));
    }
  }

  // R
  if (!includeSig) {
    ret.insert(ret.end(), 0x80);
  } else {
    ret.insert(ret.end(), reqBytesR + 0x80);
    Utils::appendBytes(ret, Utils::uintToBytes(this->r_));
  }

  // S
  if (!includeSig) {
    ret.insert(ret.end(), 0x80);
  } else {
    ret.insert(ret.end(), reqBytesS + 0x80);
    Utils::appendBytes(ret, Utils::uintToBytes(this->s_));
  }

  return ret;
}

