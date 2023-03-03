#include "tx.h"

TxBlock::TxBlock(const std::string_view& bytes) {
  uint64_t index = 0;

  // Check if first byte is equal or higher than 0xf7, meaning it is a list
  if (uint8_t(bytes[0]) < 0xf7) throw std::runtime_error("Tx is not a list");

  // Get list length
  uint8_t listLengthSize = uint8_t(bytes[index]) - 0xf7;
  index++;
  uint64_t listLength = Utils::fromBigEndian<uint64_t>(
    std::string_view(&bytes[index], listLengthSize)
  );
  index += listLengthSize; // Index is now at rlp[0] size

  // Size sanity check
  if (listLength <  bytes.size() - listLengthSize - 1) {
    throw std::runtime_error("Tx RLP returns smaller size than reported");
  }

  // If nonceLength > 0, get nonce from string.
  // Nonce can be a small string or the byte itself
  const uint8_t nonceLength = (uint8_t(bytes[index]) >= 0x80)
    ? uint8_t(bytes[index]) - 0x80 : 0;
  if (nonceLength != 0) {
    index++; // Index at rlp[0] payload
    uint64_t nonce = 0;
    this->nonce_ = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], nonceLength)
    );
    index += nonceLength; // Index at rlp[1] size
  } else {
    if (uint8_t(bytes[index]) == 0x80) {
      this->nonce_ = 0;
    } else {
      this->nonce_ = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], 1));
    }
    index++; // Index at rlp[1] size
  }

  // Get gas price - small string
  uint8_t gasPriceLength = bytes[index] - 0x80;
  if (gasPriceLength > 0x37) throw std::runtime_error("Gas price is not a small string");
  index++; // Index at rlp[1] payload
  this->gasPrice_ = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], gasPriceLength)
  );
  index += gasPriceLength; // Index at rlp[2] size.

  // Get gas limit - small string
  uint8_t gasLimitLength = bytes[index] - 0x80;
  if (gasLimitLength > 0x37) throw std::runtime_error("Gas limit is not a small string");
  index++; // Index at rlp[2] payload
  this->gas_ = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], gasLimitLength)
  );
  index += gasLimitLength; // Index at rlp[3] size

  // Get receiver addess (to) - small string.
  // We don't actually need to get the size, because to/from has a size of 20
  if (uint8_t(bytes[index]) != 0x94) throw std::runtime_error(
    "Receiver address (to) is not a 20 byte string (address)"
  );
  index++; // Index at rlp[3] payload
  this->to_ = Address(std::string_view(&bytes[index], 20), true);
  index += 20; // Index at rlp[4] size

  // Get value - small string
  uint8_t valueLength = bytes[index] - 0x80;
  if (valueLength > 0x37) throw std::runtime_error("Value is not a small string");
  index++; // Index at rlp[4] payload
  this->value_ = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], valueLength)
  );
  index += valueLength; // Index at rlp[5] size

  // Get data - it can be anything really, from nothing (0x80) to a big string (0xb7)
  if (uint8_t(bytes[index]) < 0x80) {
    this->data_ = bytes[index];
    index++; // Index at rlp[6] size
  } else if (uint8_t(bytes[index]) < 0xb7) {
    uint8_t dataLength = bytes[index] - 0x80;
    index++; // Index at rlp[5] payload
    if (dataLength > 0) {
      this->data_ = bytes.substr(index, dataLength);
      index += dataLength; // Index at rlp[6] size
    }
  } else {
    uint8_t dataLengthSize = bytes[index] - 0xb7;
    index++; // Index at rlp[5] payload size
    uint64_t dataLength = Utils::fromBigEndian<uint64_t>(
      std::string_view(&bytes[index], dataLengthSize)
    );
    index += dataLengthSize; // Index at rlp[5] payload
    this->data_ = bytes.substr(index, dataLength);
    index += dataLength; // Index at rlp[6] size
  }

  // Get v - small string or the byte itself
  uint8_t vLength = (uint8_t(bytes[index]) >= 0x80 ? uint8_t(bytes[index]) - 0x80 : 0);
  if (vLength != 0) {
    if (vLength > 0x37) throw std::runtime_error("V is not a small string");
    index++; // Index at rlp[6] payload
    this->v_ = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], vLength)
    );
    index += vLength; // Index at rlp[7] size
  } else {
    if (uint8_t(bytes[index]) == 0x80) {
      this->v_ = 0;
    } else {
      this->v_ = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], 1));
    }
    index++; // Index at rlp[7] size
  }

  // Get r - small string
  uint8_t rLength = bytes[index] - 0x80;
  if (rLength > 0x37) throw std::runtime_error("R is not a small string");
  index++; // Index at rlp[7] payload
  this->r_ = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], rLength)
  );
  index += rLength; // Index at rlp[8] size

  // Get s - small string
  uint8_t sLength = bytes[index] - 0x80;
  if (sLength > 0x37) throw std::runtime_error("S is not a small string");
  index++; // Index at rlp[8] payload
  this->s_ = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], sLength)
  );
  index += sLength; // Index at rlp[9] size

  // Get chainId - calculated from v
  if (this->v_ > 36) {
    this->chainId_ = static_cast<uint64_t>((this->v_ - 35) / 2);
    if (this->chainId_ > std::numeric_limits<uint64_t>::max()) {
      throw std::runtime_error("chainId too high");
    }
  } else if (this->v_ != 27 && this->v_ != 28) {
    throw std::runtime_error("Invalid tx signature - v is not 27 or 28");
  }


  // Verify signature and derive sender address (from) if not coming from DB
  uint8_t recoveryId = uint8_t{this->v_ - (uint256_t(this->chainId_) * 2 + 35)};
  if (!Secp256k1::verifySig(this->r_, this->s_, recoveryId)) {
    throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  Signature sig = Secp256k1::makeSig(this->r_, this->s_, recoveryId);
  Hash msgHash = this->hash(false); // Do not include signature
  UPubKey key = Secp256k1::recover(sig, msgHash);
  if (!Secp256k1::verify(msgHash, key, sig)) {
    throw std::runtime_error("Invalid transaction signature");
  }
  this->from_ = Secp256k1::toAddress(key);
}

TxBlock::TxBlock(
  const Address to, const Address from, const std::string data,
  const uint64_t chainId, const uint256_t nonce, const uint256_t value,
  const uint256_t gas, const uint256_t gasPrice, const PrivKey privKey
) : to_(to), from_(from), data_(data), chainId_(chainId), nonce_(nonce),
  value_(value), gas_(gas), gasPrice_(gasPrice)
{
  if (privKey.size() != 32) throw std::runtime_error(
    "Invalid privKey size - expected 32, got " + std::to_string(privKey.size())
  );
  UPubKey pubKey = Secp256k1::toUPub(privKey);
  Address add = Secp256k1::toAddress(pubKey);
  if (add != this->from_) throw std::runtime_error(
    "Private key does not match sender address (from)"
  );
  auto hash = this->hash(false);
  Signature sig = Secp256k1::sign(hash, privKey);

  this->r_ = Utils::bytesToUint256(sig.view(0, 32));
  this->s_ = Utils::bytesToUint256(sig.view(32,32));

  uint8_t recoveryIds = sig[64];

  this->v_ = recoveryIds + (this->chainId_ * 2 + 35);
  if (pubKey != Secp256k1::recover(sig, hash)) {
    throw std::runtime_error("Invalid transaction signature, signature derived key doens't match public key");
  }
  if (!Secp256k1::verifySig(this->r_, this->s_, recoveryIds)) {
    throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
  }
}

std::string TxBlock::rlpSerialize(bool includeSig) const {
  std::string serial;
  uint64_t total_size = 0;

  uint64_t reqBytesNonce = Utils::bytesRequired(this->nonce_);
  uint64_t reqBytesGasPrice = Utils::bytesRequired(this->gasPrice_);
  uint64_t reqBytesGas = Utils::bytesRequired(this->gas_);
  uint64_t reqBytesValue = Utils::bytesRequired(this->value_);
  uint64_t reqBytesData = this->data_.size();
  uint64_t reqBytesV = Utils::bytesRequired((includeSig) ? this->v_ : this->chainId_);
  uint64_t reqBytesR = Utils::bytesRequired(this->r_);
  uint64_t reqBytesS = Utils::bytesRequired(this->s_);

  // Calculate total size
  // nonce
  if (this->nonce_ < 0x80) total_size += 1;
  else total_size += 1 + reqBytesNonce;

  // Gas Price
  if (this->gasPrice_ < 0x80) total_size += 1;
  else total_size += 1 + reqBytesGasPrice;

  // Gas
  if (this->gas_ < 0x80) total_size += 1;
  else total_size += 1 + reqBytesGas;

  // To
  total_size += 1 + 20;

  // Value
  if (this->value_ < 0x80) total_size += 1;
  else total_size += 1 + reqBytesValue;

  // Data
  if (this->data_.size() == 0) total_size += 1;
  else if(reqBytesData <= 55) total_size += 1 + reqBytesData;
  else(total_size += 1 + Utils::bytesRequired(reqBytesData) + reqBytesData);

  // V/chainId
  if (includeSig) {
    if (this->v_ < 0x80) total_size += 1;
    else total_size += 1 + reqBytesV;
  } else {
    if (this->chainId_ < 0x80) total_size += 1;
    else total_size += 1 + reqBytesV;
  }

  // R 
  if (!includeSig) total_size += 1;
  else total_size += 1 + reqBytesR;

  // S
  if (!includeSig) total_size += 1;
  else total_size += 1 + reqBytesS;

  // Straight Serialize
  if (total_size <= 55) {
    serial.reserve(total_size + 1);
    serial += char(total_size + 0xc0);
  } else {
    uint64_t sizeBytes = Utils::bytesRequired(total_size);
    serial.reserve(total_size + sizeBytes + 1);
    serial += char(sizeBytes + 0xf7);
    serial += Utils::uintToBytes(total_size);
  }

  // Nonce
  if (this->nonce_ == 0) serial += char(0x80);
  else if (this->nonce_ < 0x80) serial += char(this->nonce_);
  else {
    serial += char(reqBytesNonce + 0x80);
    serial += Utils::uintToBytes(this->nonce_);
  }

  // Gas Price
  if (this->gasPrice_ == 0) serial += char(0x80);
  else if (this->gasPrice_ < 0x80) serial += char(this->gasPrice_);
  else {
    serial += char(reqBytesGasPrice + 0x80);
    serial += Utils::uintToBytes(this->gasPrice_);
  }

  // Gas
  if (this->gas_ == 0) serial += char(0x80);
  else if (this->gas_ < 0x80) serial += char(this->gas_);
  else {
    serial += char(reqBytesGas + 0x80);
    serial += Utils::uintToBytes(this->gas_);
  }

  // To
  serial += char(0x94);
  serial += this->to_.get();

  // Value
  if (this->value_ == 0) serial += char(0x80);
  else if (this->value_ < 0x80) serial += char(this->value_);
  else {
    serial += char(reqBytesValue + 0x80);
    serial += Utils::uintToBytes(this->value_);
  }

  // Data
  if (this->data_.size() == 0) serial += char(0x80);
  else if (reqBytesData <= 55) {
    serial += char(reqBytesData + 0x80);
    serial += this->data_;
  } else {
    serial += char(Utils::bytesRequired(reqBytesData) + 0xb7);
    serial += Utils::uintToBytes(reqBytesData);
    serial += this->data_;
  }

  // V/chainId
  if (includeSig) {
    if (this->v_ < 0x80) serial += char(this->v_);
    else {
      serial += char(reqBytesV + 0x80);
      serial += Utils::uintToBytes(this->v_);
    }
  } else {
    if (this->chainId_ < 0x80) serial += char(this->chainId_);
    else {
      serial += char(reqBytesV + 0x80);
      serial += Utils::uintToBytes(this->chainId_);
    }
  }

  // R
  if (!includeSig) serial += char(0x80);
  else {
    serial += char(reqBytesR + 0x80);
    serial += Utils::uintToBytes(this->r_);
  }

  // S
  if (!includeSig) serial += char(0x80);
  else {
    serial += char(reqBytesS + 0x80);
    serial += Utils::uintToBytes(this->s_);
  }

  return serial;
}

TxValidator::TxValidator(const std::string_view& bytes) {
  uint64_t index = 0;

  // Check if first byte is equal or higher than 0xf7, meaning it is a list
  if (uint8_t(bytes[0]) < 0xf7) throw std::runtime_error("Tx is not a list");

  // Get list length
  uint8_t listLengthSize = uint8_t(bytes[index]) - 0xf7;
  index++;
  uint64_t listLength = Utils::fromBigEndian<uint64_t>(
    std::string_view(&bytes[index], listLengthSize)
  );
  index += listLengthSize; // Index is now at rlp[0] size

  // Size sanity check
  if (listLength < bytes.size() - listLengthSize - 1) {
    throw std::runtime_error("Tx RLP returns smaller size than reported");
  }

  // Get data - it can be anything really, from nothing (0x80) to a big string (0xb7)
  if (uint8_t(bytes[index]) < 0x80) {
    this->data_ = bytes[index];
    index++; // Index at rlp[1] size
  } else if (uint8_t(bytes[index]) < 0xb7) {
    uint8_t dataLength = bytes[index] - 0x80;
    index++; // Index at rlp[0] payload
    if (dataLength > 0) {
      this->data_ = bytes.substr(index, dataLength);
      index += dataLength; // Index at rlp[1] size
    }
  } else {
    uint8_t dataLengthSize = bytes[index] - 0xb7;
    index++; // Index at rlp[0] payload size
    uint64_t dataLength = Utils::fromBigEndian<uint64_t>(
      std::string_view(&bytes[index], dataLengthSize)
    );
    index += dataLengthSize; // Index at rlp[0] payload
    this->data_ = bytes.substr(index, dataLength);
    index += dataLength; // Index at rlp[1] size
  }

  // Get nHeight - can be a small string or the byte itself
  const uint8_t nHeightLength = (uint8_t(bytes[index]) >= 0x80) ? uint8_t(bytes[index]) - 0x80 : 0;
  if (nHeightLength != 0) {
    index++; // Index at rlp[1] payload
    this->nHeight_ = Utils::fromBigEndian<uint64_t>(
      std::string_view(&bytes[index], nHeightLength)
    );
    index += nHeightLength; // Index at rlp[2] size
  } else {
    if (uint8_t(bytes[index]) == 0x80) {
      this->nHeight_ = 0;
    } else {
      this->nHeight_ = Utils::fromBigEndian<uint64_t>(std::string_view(&bytes[index], 1));
    }
    index++; // Index at rlp[2] size
  }

  // Get v - small string or the byte itself
  uint8_t vLength = (uint8_t(bytes[index]) >= 0x80) ? uint8_t(bytes[index]) - 0x80 : 0;
  if (vLength != 0) {
    if (vLength > 0x37) throw std::runtime_error("V is not a small string");
    index++; // Index at rlp[2] payload
    this->v_ = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], vLength)
    );
    index += vLength; // Index at rlp[3] size
  } else {
    if  (uint8_t(bytes[index]) == 0x80) {
      this->v_ = 0;
    } else {
      this->v_ = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], 1));
    }
    index++; // Index at rlp[3] size
  }

  // Get r - small string
  uint8_t rLength = bytes[index] - 0x80;
  if (rLength > 0x37) throw std::runtime_error("R is not a small string");
  index++; // Index at rlp[3] payload
  this->r_ = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], rLength)
  );
  index += rLength; // Index at rlp[4] size

  // Get s - small string
  uint8_t sLength = bytes[index] - 0x80;
  if (sLength > 0x37) throw std::runtime_error("S is not a small string");
  index++; // Index at rlp[4] payload
  this->s_ = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], sLength)
  );
  index += sLength; // Index at rlp[5] size. rlp[5] doesn't exist on Validator txs

  // Get chainId - calculated from v
  if (this->v_ > 36) {
    this->chainId_ = static_cast<uint64_t>((this->v_ - 35) / 2);
    if (this->chainId_ > std::numeric_limits<uint64_t>::max()) {
      throw std::runtime_error("chainId too high");
    }
  } else if (this->v_ != 27 && this->v_ != 28) {
    throw std::runtime_error("Invalid tx signature - v is not 27 or 28, v is "
      + boost::lexical_cast<std::string>(this->v_));
  }

  // Get recoveryId, verify the signature and derive sender address (from)
  uint8_t recoveryId = uint8_t{this->v_ - (uint256_t(this->chainId_) * 2 + 35)};
  if (!Secp256k1::verifySig(this->r_, this->s_, recoveryId)) {
    throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  Signature sig = Secp256k1::makeSig(this->r_, this->s_, recoveryId);
  Hash msgHash = this->hash(false); // Do not include signature
  UPubKey key = Secp256k1::recover(sig, msgHash);
  if (!Secp256k1::verify(msgHash, key, sig)) throw std::runtime_error("Invalid tx signature");
  this->from_ = Secp256k1::toAddress(key);
}

TxValidator::TxValidator(
  const Address from, const std::string data, const uint64_t chainId,
  const uint64_t nHeight, const PrivKey privKey
) : from_(from), data_(data), chainId_(chainId), nHeight_(nHeight) {
  if (privKey.size() != 32) throw std::runtime_error(
    "Invalid private key size - expected 32, got " + std::to_string(privKey.size())
  );
  UPubKey pubKey = Secp256k1::toUPub(privKey);
  Address add = Secp256k1::toAddress(pubKey);
  auto hash = this->hash(false);
  if (add != this->from_) throw std::runtime_error(
    "Private key does not match sender address (from)"
  );
  Signature sig = Secp256k1::sign(hash, privKey);
  this->r_ = Utils::bytesToUint256(sig.view(0, 32));
  this->s_ = Utils::bytesToUint256(sig.view(32,32));
  uint8_t recoveryIds = sig[64];
  this->v_ = recoveryIds + (this->chainId_ * 2 + 35);
  if (!Secp256k1::verifySig(this->r_, this->s_, recoveryIds)) {
    throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  if (pubKey != Secp256k1::recover(sig, hash)) {
    throw std::runtime_error("Invalid transaction signature, signature derived key doens't match public key");
  }
}

std::string TxValidator::rlpSerialize(bool includeSig) const {
  std::string serial;
  uint64_t total_size = 0;

  uint64_t reqBytesData = this->data_.size();
  uint64_t reqBytesnHeight = Utils::bytesRequired(this->nHeight_);
  uint64_t reqBytesV = Utils::bytesRequired((includeSig) ? this->v_ : this->chainId_);
  uint64_t reqBytesR = Utils::bytesRequired(this->r_);
  uint64_t reqBytesS = Utils::bytesRequired(this->s_);

  // Calculate total size
  // Data
  if (this->data_.size() == 0) total_size += 1;
  else if(reqBytesData <= 55) total_size += 1 + reqBytesData;
  else(total_size += 1 + Utils::bytesRequired(reqBytesData) + reqBytesData);
  
  // nHeight
  if (this->nHeight_ < 0x80) total_size += 1;
  else total_size += 1 + reqBytesnHeight;

  // V/chainId
  if (includeSig) {
    if (this->v_ < 0x80) total_size += 1;
    else total_size += 1 + reqBytesV;
  } else {
    if (this->chainId_ < 0x80) total_size += 1;
    else total_size += 1 + reqBytesV;
  }

  // R 
  if (!includeSig) total_size += 1;
  else total_size += 1 + reqBytesR;

  // S
  if (!includeSig) total_size += 1;
  else total_size += 1 + reqBytesS;
  
  // Straight Serialize
  if (total_size <= 55) {
    serial.reserve(total_size + 1);
    serial += char(total_size + 0xc0);
  } else {
    uint64_t sizeBytes = Utils::bytesRequired(total_size);
    total_size += sizeBytes;
    serial.reserve(total_size + 1);
    serial += char(sizeBytes + 0xf7);
    serial += Utils::uintToBytes(total_size);
  }

  // Data
  if (this->data_.size() == 0) serial += char(0x80);
  else if (reqBytesData <= 55) {
    serial += char(reqBytesData + 0x80);
    serial += this->data_;
  } else {
    serial += char(Utils::bytesRequired(reqBytesData) + 0xb7);
    serial += Utils::uintToBytes(reqBytesData);
    serial += this->data_;
  }

  // nHeight
  if (this->nHeight_ < 0x80) serial += char(this->nHeight_);
  else {
    serial += char(reqBytesnHeight + 0x80);
    serial += Utils::uintToBytes(this->nHeight_);
  }

  // V/chainId
  if (includeSig) {
    if (this->v_ < 0x80) serial += char(this->v_);
    else {
      serial += char(reqBytesV + 0x80);
      serial += Utils::uintToBytes(this->v_);
    }
  } else {
    if (this->chainId_ < 0x80) serial += char(this->chainId_);
    else {
      serial += char(reqBytesV + 0x80);
      serial += Utils::uintToBytes(this->chainId_);
    }
  }

  // R
  if (!includeSig) serial += char(0x80);
  else {
    serial += char(reqBytesR + 0x80);
    serial += Utils::uintToBytes(this->r_);
  }

  // S
  if (!includeSig) serial += char(0x80);
  else {
    serial += char(reqBytesS + 0x80);
    serial += Utils::uintToBytes(this->s_);
  }

  return serial;
}