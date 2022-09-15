#include "transaction.h"


// custom implementation (no RLP abstraction) 
Tx::Base::Base(const std::string_view &bytes, bool fromDB) {
  std::string_view appendedBytes;
  uint64_t index = 0;

  // Check if first byte is equal or higher than 0xf7, meaning it is a list
  if (uint8_t(bytes[0]) < 0xf7) {
    // tx is not a list.
    throw std::runtime_error("Transaction is not a list");
  }

  // Get list lenght.
  uint8_t listLenghtSize = bytes[index] - 0xf7;
  ++index;
  uint64_t listLenght = Utils::fromBigEndian<uint64_t>(std::string_view(&bytes[index], listLenghtSize));
  index += listLenghtSize; // Index is now at rlp[0] size.
  // Size sanity check.
  if (listLenght < ((fromDB) ? (bytes.size() - 25) : bytes.size()) - listLenghtSize - 1) {
    throw std::runtime_error("Transaction RLP reports a size, returns smaller.");
  }
  const uint8_t nonceLenght = bytes[index] - 0x80; // Nonce is a small string.

  // Nonce must be a small string.
  if (nonceLenght > 0x37) { throw std::runtime_error("Nonce is not a small string"); }
  if (nonceLenght != 0) {
    ++index; // Index at rlp[0] payload.
    uint64_t nonce = 0;
    this->_nonce = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], nonceLenght));
    index += nonceLenght; // Index at rlp[1] size.
  } else {
    this->_nonce = 0;
    ++index; // Index at rlp[1] size.
  }

  // Get gas price. small string.
  uint8_t gasPriceLenght = bytes[index] - 0x80;
  if (gasPriceLenght > 0x37) { throw std::runtime_error("Gas price is not a small string"); }
  ++index; // Index at rlp[1] payload.
  this->_gasPrice = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], gasPriceLenght));
  index += gasPriceLenght; // Index at rlp[2] size.
  
  // Get gas limit. small string.
  uint8_t gasLimitLenght = bytes[index] - 0x80;
  if (gasLimitLenght > 0x37) { throw std::runtime_error("Gas limit is not a small string"); }
  ++index; // Index at rlp[2] payload.
  this->_gas = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], gasLimitLenght));
  index += gasLimitLenght; // Index at rlp[3] size.

  // Get to. small string
  // We don't actually need to get the size, because to/from has a size of 20
  if (uint8_t(bytes[index]) != 0x94) { throw std::runtime_error("To is not a 20 byte string (address)"); }
  ++index; // Index at rlp[3] payload.
  this->_to = Utils::fromBigEndian<uint160_t>(std::string_view(&bytes[index], 20));
  index += 20; // Index at rlp[4] size.

  // Get value. small string
  uint8_t valueLenght = bytes[index] - 0x80;
  if (valueLenght > 0x37) { throw std::runtime_error("Value is not a small string"); }
  ++index; // Index at rlp[4] payload.
  this->_value = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], valueLenght));
  index += valueLenght; // Index at rlp[5] size.

  // Get data, it can be anything really, from nothing (0x80) to a big string (0xb7);
  if (uint8_t(bytes[index]) < 0x80) {
    this->_data = bytes[index];
    ++index; // Index at rlp[6] size
  } else if (uint8_t(bytes[index]) < 0xb7) {
    uint8_t dataLenght = bytes[index] - 0x80;
    ++index; // Index at rlp[5] payload.
    if (dataLenght > 0) {
      this->_data = bytes.substr(index, dataLenght);
      index += dataLenght; // Index at rlp[6] size.
    }
  } else {
    uint8_t dataLenghtSize = bytes[index] - 0xb7;
    ++index; // Index at rlp[5] payload size.
    uint64_t dataLenght = Utils::fromBigEndian<uint64_t>(std::string_view(&bytes[index], dataLenghtSize));
    index += dataLenghtSize; // Index at rlp[5] payload.
    this->_data = bytes.substr(index, dataLenght);
    index += dataLenght; // Index at rlp[6] size
  }

  // Get v, small string.

  uint8_t vLenght = bytes[index] - 0x80;
  if (vLenght > 0x37) { throw std::runtime_error("V is not a small string"); }
  ++index; // Index at rlp[6] payload.
  this->_v = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], vLenght));
  index += vLenght; // Index at rlp[7] size.

  // Get r, small string, 32 in size.
  if (uint8_t(bytes[index]) != 0xa0) { throw std::runtime_error("R is not a 32 byte string"); }
  ++index; // Index at rlp[7] payload.
  this->_r = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], 32));
  index += 32; // Index at rlp[8] size.

  // get s, small string
  if (uint8_t(bytes[index]) != 0xa0) { throw std::runtime_error("S is not a 32 byte string"); }
  ++index; // Index at rlp[8] payload.
  this->_s = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], 32));
  index += 32; // Index at rlp[9] size.

  if (_v > 36) {
    this->_chainId = static_cast<uint64_t>((this->_v - 35) / 2);
    if (this->_chainId > std::numeric_limits<uint64_t>::max()) {
      throw std::runtime_error(std::string(__func__) + ": " +
        std::string("RLP: Transaction chainId too high")
      );
    }
  } else if (this->_v != 27 && this->_v != 28) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("RLP: Invalid transaction signature - v is not 27 or 28")
    );
  }

  if (!fromDB) {
    // If tx is not coming from DB, we have to verify its signature
    uint8_t recoveryId = uint8_t{this->_v - (uint256_t(this->_chainId) * 2 + 35)};
    if (!Utils::verifySignature(recoveryId, this->_r, this->_s)) {
      throw std::runtime_error(std::string(__func__) + ": " +
        std::string("RLP: Invalid transaction signature - doesn't fit elliptic curve verification")
      );
    }
    std::string sig;
    Secp256k1::appendSignature(this->_r, this->_s, recoveryId, sig);
    this->_hasSig = true;
    std::string messageHash = Utils::sha3(this->rlpSerialize(false));
    auto pubKey = Secp256k1::recover(sig, messageHash);
    if (!Secp256k1::verify(pubKey, sig, messageHash)) {
      throw std::runtime_error(std::string(__func__) + ": " +
        std::string("RLP: Invalid transaction signature")
      );
    }
    this->_from = Secp256k1::toAddress(pubKey);
    this->_verified = true;
    return;
  }
  appendedBytes = bytes.substr(bytes.size() - 25);

  // If tx is coming from DB, we simply read the information from the extra bytes.
  // Txs that come from DB are included in a block, which means they are
  // already verified, so we don't have to redo the expensive secp256k1 calculation
  // to verify their signature.
  this->_blockIndex = Utils::bytesToUint32(appendedBytes.substr(0, 4));
  this->_from = appendedBytes.substr(4, 20);
  this->_callsContract = static_cast<bool>(static_cast<char>(appendedBytes[24]));
  this->_hasSig = true;
  this->_inBlock = true;
  this->_verified = true;

}

// RLP Based Decoding
//Tx::Base::Base(const std::string_view &bytes, bool fromDB) {
//  std::string appendedBytes;
//  std::string substr;
//  // Copy from bytes if necessary.
//  if (fromDB) {
//    appendedBytes = bytes.substr(bytes.size() - 25);
//    substr = bytes.substr(0, bytes.size() - 25);
//  }
//
//  dev::RLP rlp((fromDB) ? substr : bytes);
//  if (!rlp.isList()) {
//    throw std::runtime_error(std::string(__func__) + ": " +
//      std::string("RLP: Transaction is not a list")
//    );
//  }
//  rlp[0].toIntRef<uint256_t>(this->_nonce);
//  rlp[1].toIntRef<uint256_t>(this->_gasPrice);
//  rlp[2].toIntRef<uint256_t>(this->_gas);
//  if (!rlp[3].isData()) {
//    throw std::runtime_error(std::string(__func__) + ": " +
//      std::string("RLP: Receiver must be a byte array")
//    );
//  }
//  this->_to = rlp[3].toInt<uint160_t>();
//  rlp[4].toIntRef<uint256_t>(this->_value);
//  if (!rlp[5].isData()) {
//    throw std::runtime_error(std::string(__func__) + ": " +
//      std::string("RLP: Transaction data must be a byte array")
//    );
//  }
//  this->_data = rlp[5].toString();
//  rlp[6].toIntRef<uint256_t>(this->_v);
//  rlp[7].toIntRef<uint256_t>(this->_r);
//  rlp[8].toIntRef<uint256_t>(this->_s);
//  if (_v > 36) {
//    this->_chainId = static_cast<uint64_t>((this->_v - 35) / 2);
//    if (this->_chainId > std::numeric_limits<uint64_t>::max()) {
//      throw std::runtime_error(std::string(__func__) + ": " +
//        std::string("RLP: Transaction chainId too high")
//      );
//    }
//  } else if (this->_v != 27 && this->_v != 28) {
//    throw std::runtime_error(std::string(__func__) + ": " +
//      std::string("RLP: Invalid transaction signature - v is not 27 or 28")
//    );
//  }
//  if (!fromDB) {
//    // If tx is not coming from DB, we have to verify its signature
//    uint8_t recoveryId = uint8_t{this->_v - (uint256_t(this->_chainId) * 2 + 35)};
//    if (!Utils::verifySignature(recoveryId, this->_r, this->_s)) {
//      throw std::runtime_error(std::string(__func__) + ": " +
//        std::string("RLP: Invalid transaction signature - doesn't fit elliptic curve verification")
//      );
//    }
//    std::string sig;
//    Secp256k1::appendSignature(this->_r, this->_s, recoveryId, sig);
//    this->_hasSig = true;
//    std::string messageHash;
//    Utils::sha3(this->rlpSerialize(false), messageHash);
//    auto pubKey = Secp256k1::recover(sig, messageHash);
//    std::string pubKeyHash;
//    Utils::sha3(pubKey, pubKeyHash);
//    this->_from = pubKeyHash.substr(12);  // Address = pubkey[12...32]
//    this->_verified = true;
//    if (rlp.itemCount() > 9) {
//      throw std::runtime_error(std::string(__func__) + ": " +
//        std::string("RLP: Too many fields")
//      );
//    }
//    return;
//  }
//  // If tx is coming from DB, we simply read the information from the extra bytes.
//  // Txs that come from DB are included in a block, which means they are
//  // already verified, so we don't have to redo the expensive secp256k1 calculation
//  // to verify their signature.
//  this->_blockIndex = Utils::bytesToUint32(appendedBytes.substr(0, 4));
//  this->_from = appendedBytes.substr(4, 20);
//  this->_callsContract = static_cast<bool>(static_cast<char>(appendedBytes[24]));
//  this->_hasSig = true;
//  this->_inBlock = true;
//  this->_verified = true;
//
//}

std::string Tx::Base::rlpSerialize(const bool &includeSig) const {
  // EIP-155 Compatible.
  // instead of hashing six rlp encoded elements (nonce, gasprice, startgas, to, value, data)
  // hash nine rlp encoded elements (nonce, gasprice, startgas, to, value, data, chainid, 0, 0) before signing.
  dev::RLPStream rlpStrm;
  rlpStrm.appendList(9);
  rlpStrm << this->_nonce << this->_gasPrice << this->_gas <<
    this->_to.toHash() << this->_value << this->_data;
  if (includeSig) {
    rlpStrm << (this->recoverId() + (this->_chainId * 2 + 35)) << this->_r << this->_s;
  } else {
    rlpStrm << this->_chainId << 0 << 0;
  }
  std::string ret;
  rlpStrm.exportBytesString(ret);
  return ret;
}

std::string Tx::Base::serialize() const {
  if (!this->_hasSig && !this->_verified) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Transaction has no signature and/or is not verified")
    );
  }
  std::string ret = this->rlpSerialize(true);
  ret += Utils::uint32ToBytes(this->_blockIndex) + _from.get() + static_cast<char>(this->_callsContract);
  return ret;
}

void Tx::Base::sign(std::string &privKey) {
  if (privKey.size() != 32) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Invalid private key size - expected 32, got ") + std::to_string(privKey.size())
    );
  }
  auto pubkey = Secp256k1::toPub(privKey);
  std::string pubkeyHash = Utils::sha3(pubkey);
  Address address(pubkeyHash.substr(12), false); // Address = hash(pubkey)[12]...[32]
  if (address != this->_from) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Private key does not match sender address")
    );
  }
  std::string messageHash = Utils::sha3(this->rlpSerialize(false));
  std::string signature = Secp256k1::sign(privKey, messageHash);
  this->_r = Utils::bytesToUint256(signature.substr(0,32));
  this->_s = Utils::bytesToUint256(signature.substr(32,32));
  uint8_t recoveryIds = signature[64];
  this->_v = recoveryIds + (this->_chainId * 2 + 35);
  if (!Utils::verifySignature(recoveryIds, this->_r, this->_s)) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Invalid transaction signature - doesn't fit elliptic curve verification")
    );
  }
  this->_verified = true;
  this->_hasSig = true;
  return;
}

