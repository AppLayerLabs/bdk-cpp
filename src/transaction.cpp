#include "transaction.h"

// TODO: I believe there is multiple unecessary copies with
// It might repeat the same code inside the conditions, but I believe it is better organized this way.
Tx::Base::Base(std::string &bytes, bool fromDB) {
  // Parse RLP
  std::string appendedBytes;
  if (fromDB) {
    appendedBytes = bytes.substr(bytes.size() - 25);
    bytes = bytes.substr(0, bytes.size() - 25);
  }

  dev::RLP rlp(bytes);
  if (!rlp.isList()) {
    throw std::runtime_error("transaction RLP is not a list");
  }
  rlp[0].toIntRef<uint256_t>(this->_nonce);
  rlp[1].toIntRef<uint256_t>(this->_gasPrice);
  rlp[2].toIntRef<uint256_t>(this->_gas);
  if (!rlp[3].isData()) {
    throw std::runtime_error("recepient RLP must be a byte array");
  }
  this->_to = rlp[3].toInt<uint160_t>();
  rlp[4].toIntRef<uint256_t>(this->_value);
  if (!rlp[5].isData()) {
    throw std::runtime_error("transaction data RLP must be a byte array");
  }
  this->_data = rlp[5].toString();
  rlp[6].toIntRef<uint256_t>(this->_v);
  rlp[7].toIntRef<uint256_t>(this->_r);
  rlp[8].toIntRef<uint256_t>(this->_s);
  if (_v > 36) {
    this->_chainId = static_cast<uint64_t>((this->_v - 35) / 2);
    if (this->_chainId > std::numeric_limits<uint64_t>::max()) {
        throw std::runtime_error("transaction chainId too high.");
    }
  } else if (this->_v != 27 && this->_v != 28 ) {
    throw std::runtime_error("Transaction signature invalid, v is not 27 or 28");
  }
  // Not from DB? Has to have it's signature verified.
  if (!fromDB) {
    uint8_t recoveryId = uint8_t{this->_v - (uint256_t(this->_chainId) * 2 + 35)};
    if (!Utils::verifySignature(recoveryId, this->_r, this->_s)) {
      throw std::runtime_error("Transaction Signature invalid, signature doesn't fit elliptic curve");;
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
      throw std::runtime_error("too many fields in the transaction RLP");
    }
    return;
  } else {
    // Simply read the information from the extra bytes.
    // There is no need to redo the expensive secp256k1 calculation. and tx is also included in a block.
    // FROM DB == TX IN BLOCK (maybe dangerous?)
    this->_blockIndex = Utils::bytesToUint32(appendedBytes.substr(0, 4));
    this->_from = appendedBytes.substr(4, 20);
    this->_callsContract = bool(char(appendedBytes[24]));
    this->_hasSig = true;
    this->_inBlock = true;
    this->_verified = true;
  }
}

std::string Tx::Base::rlpSerialize(bool includeSig) const {
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
    throw std::runtime_error("Transaction has no signature/not verified to serialize");
  }
  std::string ret = this->rlpSerialize(true);
  ret += Utils::uint32ToBytes(this->_blockIndex) + _from.get() + char(this->_callsContract);
  return ret;
}

void Tx::Base::sign(std::string &privKey) {
  if (privKey.size() != 32) { throw std::runtime_error("Tx::Base::sign privateKey invalid size"); }
  auto pubkey = Secp256k1::toPub(privKey);
  std::string pubkeyHash;
  Utils::sha3(pubkey, pubkeyHash);
  Address address(pubkeyHash.substr(12), false); // Address = hash(pubkey)[12] ... [32]
  std::cout << address.hex() << std::endl;
  std::cout << this->_from.hex() << std::endl;
  if (address != this->_from) { throw std::runtime_error("Tx::Base::sign different privateKey"); }
  std::string messageHash;
  Utils::sha3(this->rlpSerialize(false), messageHash);
  std::string signature = Secp256k1::sign(privKey, messageHash);
  std::cout << Utils::bytesToHex(signature) << std::endl;
  this->_r = Utils::bytesToUint256(signature.substr(0,32));
  this->_s = Utils::bytesToUint256(signature.substr(32,32));
  uint8_t recoveryIds = signature[64];
  this->_v = recoveryIds + (this->_chainId * 2 + 35);
  if (!Utils::verifySignature(recoveryIds, this->_r, this->_s)) {
    throw std::runtime_error("Transaction Signature invalid, signature doesn't fit elliptic curve");
  }

  this->_verified = true;
  this->_hasSig = true;

  return;
}