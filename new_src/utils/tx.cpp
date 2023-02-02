#include "tx.h"

TxBlock::TxBlock(const std::string_view& bytes, bool fromDB) {
  std::string_view appendedBytes;
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
  if (listLength < ((fromDB) ? (bytes.size() - 25) : bytes.size()) - listLengthSize - 1) {
    throw std::runtime_error("Tx RLP returns smaller size than reported");
  }

  // If nonceLength > 0, get nonce from string.
  // Nonce can be a small string or the byte itself
  const uint8_t nonceLength = (uint8_t(bytes[index]) >= 0x80)
    ? uint8_t(bytes[index]) - 0x80 : 0;
  if (nonceLength != 0) {
    index++; // Index at rlp[0] payload
    uint64_t nonce = 0;
    this->nonce = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], nonceLength)
    );
    index += nonceLength; // Index at rlp[1] size
  } else {
    this->nonce = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], 1)
      - (uint8_t(bytes[index]) == 0x80) ? 0x80 : 0
    );
    index++; // Index at rlp[1] size
  }

  // Get gas price - small string
  uint8_t gasPriceLength = bytes[index] - 0x80;
  if (gasPriceLength > 0x37) throw std::runtime_error("Gas price is not a small string");
  index++; // Index at rlp[1] payload
  this->gasPrice = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], gasPriceLength)
  );
  index += gasPriceLength; // Index at rlp[2] size.

  // Get gas limit - small string
  uint8_t gasLimitLength = bytes[index] - 0x80;
  if (gasLimitLength > 0x37) throw std::runtime_error("Gas limit is not a small string");
  index++; // Index at rlp[2] payload
  this->gas = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], gasLimitLength)
  );
  index += gasLimitLength; // Index at rlp[3] size

  // Get receiver addess (to) - small string.
  // We don't actually need to get the size, because to/from has a size of 20
  if (uint8_t(bytes[index]) != 0x94) throw std::runtime_error(
    "Receiver address (to) is not a 20 byte string (address)"
  );
  index++; // Index at rlp[3] payload
  this->to = Utils::fromBigEndian<uint160_t>(
    std::string_view(&bytes[index], 20)
  );
  index += 20; // Index at rlp[4] size

  // Get value - small string
  uint8_t valueLength = bytes[index] - 0x80;
  if (valueLength > 0x37) throw std::runtime_error("Value is not a small string");
  index++; // Index at rlp[4] payload
  this->value = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], valueLength)
  );
  index += valueLength; // Index at rlp[5] size

  // Get data - it can be anything really, from nothing (0x80) to a big string (0xb7)
  if (uint8_t(bytes[index]) < 0x80) {
    this->data = bytes[index];
    index++; // Index at rlp[6] size
  } else if (uint8_t(bytes[index]) < 0xb7) {
    uint8_t dataLength = bytes[index] - 0x80;
    index++; // Index at rlp[5] payload
    if (dataLength > 0) {
      this->data = bytes.substr(index, dataLength);
      index += dataLength; // Index at rlp[6] size
    }
  } else {
    uint8_t dataLengthSize = bytes[index] - 0xb7;
    index++; // Index at rlp[5] payload size
    uint64_t dataLength = Utils::fromBigEndian<uint64_t>(
      std::string_view(&bytes[index], dataLengthSize)
    );
    index += dataLengthSize; // Index at rlp[5] payload
    this->data = bytes.substr(index, dataLength);
    index += dataLength; // Index at rlp[6] size
  }

  // Get v - small string or the byte itself
  uint8_t vLength = (uint8_t(bytes[index]) >= 0x80 ? uint8_t(bytes[index]) - 0x80 : 0);
  if (vLength != 0) {
    if (vLength > 0x37) throw std::runtime_error("V is not a small string");
    index++; // Index at rlp[6] payload
    this->v = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], vLength)
    );
    index += vLength; // Index at rlp[7] size
  } else {
    this->v = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], 1)
      - (uint8_t(bytes[index]) == 0x80) ? 0x80 : 0;
    );
    index++; // Index at rlp[7] size
  }

  // Get r - small string
  uint8_t rLength = bytes[index] - 0x80;
  if (rLength > 0x37) throw std::runtime_error("R is not a small string");
  index++; // Index at rlp[7] payload
  this->r = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], rLength)
  );
  index += rLength; // Index at rlp[8] size

  // Get s - small string
  uint8_t sLength = bytes[index] - 0x80;
  if (sLength > 0x37) throw std::runtime_error("S is not a small string");
  index++; // Index at rlp[8] payload
  this->s = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], sLength)
  );
  index += sLength; // Index at rlp[9] size

  // Get chainId - calculated from v
  if (this->v > 36) {
    this->chainId = static_cast<uint64_t>((this->v - 35) / 2);
    if (this->chainId > std::numeric_limits<uint64_t>::max()) {
      throw std::runtime_error("chainId too high");
    }
  } else if (this->v != 27 && this->v != 28) {
    throw std::runtime_error("Invalid tx signature - v is not 27 or 28");
  }

  if (!fromDB) {
    // Verify signature and derive sender address (from) if not coming from DB
    uint8_t recoveryId = uint8_t{this->v - (uint256_t(this->chainId) * 2 + 35)};
    if (!Secp256k1::verifySig(this->r, this->s, recoveryId)) {
      throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
    }
    Signature sig = Secp256k1::makeSig(this->r, this->s, recoveryId);
    Hash msgHash = this->hash(false); // Do not include signature
    UPubkey key = Secp256k1::recover(sig, msgHash);
    if (!Secp256k1::verify(msgHash, key, sig)) {
      throw std::runtime_error("Invalid transaction signature");
    }
    this->from = Secp256k1::toAddress(key);
  } else {
    /**
     * Skip signature verification and simply read sender address (from) if coming from DB.
     * Txs coming from DB are included in a block, which means they're already
     * verified, so we don't have to redo the expensive secp256k1 calculation above.
     */
    appendedBytes = bytes.substr(bytes.size() - 25);
    this->from = appendedBytes.substr(4, 20);
  }
}

std::string TxBlock::rlpSerialize(bool includeSig, bool includeFrom) const {
  dev::RLPStream rlpStrm;
  rlpStrm.appendList(9);
  rlpStrm << this->nonce << this->gasPrice << this->gas <<
    this->to.toHash() << this->value << this->data;
  if (includeSig) {
    rlpStrm << (this->recoverId() + (this->chainId * 2 + 35)) << this->r << this->s;
  } else {
    rlpStrm << this->chainId << 0 << 0;
  }
  std::string ret;
  rlpStrm.exportBytesString(ret);
  if (includeFrom) ret += this->from.get();
  return ret;
}

TxValidator::TxValidator(const std::string_view& bytes, bool fromDB) {
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
    this->data = bytes[index];
    index++; // Index at rlp[1] size
  } else if (uint8_t(bytes[index]) < 0xb7) {
    uint8_t dataLength = bytes[index] - 0x80;
    index++; // Index at rlp[0] payload
    if (dataLength > 0) {
      this->data = bytes.substr(index, dataLength);
      index += dataLength; // Index at rlp[1] size
    }
  } else {
    uint8_t dataLengthSize = bytes[index] - 0xb7;
    index++; // Index at rlp[0] payload size
    uint64_t dataLength = Utils::fromBigEndian<uint64_t>(
      std::string_view(&bytes[index], dataLengthSize)
    );
    index += dataLengthSize; // Index at rlp[0] payload
    this->data = bytes.substr(index, dataLength);
    index += dataLength; // Index at rlp[1] size
  }

  // Get nHeight - can be a small string or the byte itself
  const uint8_t nHeightLength = (uint8_t(bytes[index]) >= 0x80) ? uint8_t(bytes[index]) - 0x80 : 0;
  if (nHeightLength != 0) {
    index++; // Index at rlp[1] payload
    this->nHeight = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], nHeightLength)
    );
    index += nHeightLength; // Index at rlp[2] size
  } else {
    this->nHeight = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], 1)
      - (uint8_t(bytes[index]) == 0x80) ? 0x80 : 0
    );
    index++; // Index at rlp[2] size
  }

  // Get v - small string or the byte itself
  uint8_t vLength = (uint8_t(bytes[index]) >= 0x80) ? uint8_t(bytes[index]) - 0x80 : 0;
  if (vLength != 0) {
    if (vLength > 0x37) throw std::runtime_error("V is not a small string");
    index++; // Index at rlp[2] payload
    this->v = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], vLength)
    );
    index += vLength; // Index at rlp[3] size
  } else {
    this->v = Utils::fromBigEndian<uint256_t>(
      std::string_view(&bytes[index], 1)
      - (uint8_t(bytes[index]) == 0x80) ? 0x80 : 0
    );
    index++; // Index at rlp[3] size
  }

  // Get r - small string
  uint8_t rLength = bytes[index] - 0x80;
  if (rLength > 0x37) throw std::runtime_error("R is not a small string");
  index++; // Index at rlp[3] payload
  this->r = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], rLength)
  );
  index += rLength; // Index at rlp[4] size

  // Get s - small string
  uint8_t sLength = bytes[index] - 0x80;
  if (sLength > 0x37) throw std::runtime_error("S is not a small string");
  index++; // Index at rlp[4] payload
  this->s = Utils::fromBigEndian<uint256_t>(
    std::string_view(&bytes[index], sLength)
  );
  index += sLength; // Index at rlp[5] size. rlp[5] doesn't exist on Validator txs

  // Get chainId - calculated from v
  if (this->v > 36) {
    this->chainId = static_cast<uint64_t>((this->v - 35) / 2);
    if (this->chainId > std::numeric_limits<uint64_t>::max()) {
      throw std::runtime_error("chainId too high");
    }
  } else if (this->v != 27 && this->v != 28) {
    throw std::runtime_error("Invalid tx signature - v is not 27 or 28, v is "
      + boost::lexical_cast<std::string>(this->v));
  }

  // Get recoveryId, verify the signature and derive sender address (from)
  uint8_t recoveryId = uint8_t{this->v - (uint256_t(this->chainId) * 2 + 35)};
  if (!Secp256k1::verifySig(this->r, this->s, recoveryId)) {
    throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  Signature sig = Secp256k1::makeSig(this->r, this->s, recoveryId);
  Hash msgHash = this->hash(false); // Do not include signature
  UPubkey key = Secp256k1::recover(sig, msgHash);
  if (!Secp256k1::verify(msgHash, key, sig)) throw std::runtime_error("Invalid tx signature");
  this->from = Secp256k1::toAddress(key);
}

std::string TxValidator::rlpSerialize(bool includeSig, bool includeFrom) const {
  dev::RLPStream rlpStrm;
  rlpStrm.appendList(5);
  rlpStrm << this->data << this->nHeight;
  if (includeSig) {
    rlpStrm << (this->recoverId() + (this->chainId * 2 + 35)) << this->r << this->s;
  } else {
    rlpStrm << this->chainId << 0 << 0;
  }
  std::string ret;
  rlpStrm.exportBytesString(ret);
  if (includeFrom) ret += this->from.get();
  return ret;
}

