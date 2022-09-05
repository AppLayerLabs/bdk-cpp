#include "transaction.h"

Tx::Base::Base(const std::string_view &bytes, bool fromDB) {
  std::string appendedBytes;
  std::string substr;
  // Copy from bytes if necessary.
  if (fromDB) {
    appendedBytes = bytes.substr(bytes.size() - 25);
    substr = bytes.substr(0, bytes.size() - 25);
  }

  dev::RLP rlp((fromDB) ? substr : bytes);
  if (!rlp.isList()) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("RLP: Transaction is not a list")
    );
  }
  rlp[0].toIntRef<uint256_t>(this->_nonce);
  rlp[1].toIntRef<uint256_t>(this->_gasPrice);
  rlp[2].toIntRef<uint256_t>(this->_gas);
  if (!rlp[3].isData()) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("RLP: Receiver must be a byte array")
    );
  }
  this->_to = rlp[3].toInt<uint160_t>();
  rlp[4].toIntRef<uint256_t>(this->_value);
  if (!rlp[5].isData()) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("RLP: Transaction data must be a byte array")
    );
  }
  this->_data = rlp[5].toString();
  rlp[6].toIntRef<uint256_t>(this->_v);
  rlp[7].toIntRef<uint256_t>(this->_r);
  rlp[8].toIntRef<uint256_t>(this->_s);
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
    std::string messageHash;
    Utils::sha3(this->rlpSerialize(false), messageHash);
    auto pubKey = Secp256k1::recover(sig, messageHash);
    std::string pubKeyHash;
    Utils::sha3(pubKey, pubKeyHash);
    this->_from = pubKeyHash.substr(12);  // Address = pubkey[12...32]
    this->_verified = true;
    if (rlp.itemCount() > 9) {
      throw std::runtime_error(std::string(__func__) + ": " +
        std::string("RLP: Too many fields")
      );
    }
    return;
  }
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
  std::string pubkeyHash;
  Utils::sha3(pubkey, pubkeyHash);
  Address address(pubkeyHash.substr(12), false); // Address = hash(pubkey)[12]...[32]
  if (address != this->_from) {
    throw std::runtime_error(std::string(__func__) + ": " +
      std::string("Private key does not match sender address")
    );
  }
  std::string messageHash;
  Utils::sha3(this->rlpSerialize(false), messageHash);
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

