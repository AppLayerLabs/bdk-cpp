#include "transaction.h"

// TODO: I believe there is multiple unecessary copies with
// It might repeat the same code inside the conditions, but I believe it is better organized this way.
Tx::Base::Base(std::string &bytes, bool fromDB) {
  if (!fromDB) {
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
  } else {
    // Simply read the information from the extra bytes.
    // There is no need to redo the expensive secp256k1 calculation,
    // since the tx is already included in a block, thus already valid and verified.
    std::string rlpBytes = bytes.substr(0,bytes.size() - 40);
    std::string appendedBytes = bytes.substr(bytes.size() - 40, 40);
    dev::RLP rlp(bytes);
  }
}

std::string Tx::Base::rlpSerialize(bool includeSig) {
  // EIP-155 Compatible.
  // instead of hashing six rlp encoded elements (nonce, gasprice, startgas, to, value, data)
  // hash nine rlp encoded elements (nonce, gasprice, startgas, to, value, data, chainid, 0, 0) before signing.
  dev::RLPStream rlpStrm;
  rlpStrm.appendList(9);
  rlpStrm << this->_nonce << this->_gasPrice << this->_gas <<
    this->_to.toHash() << this->_value << this->_data;
  if (includeSig) {
    rlpStrm << (this->_v + (this->_chainId * 2 + 35)) << this->_r << this->_s;
  } else {
    rlpStrm << this->_chainId << 0 << 0;
  }
  std::string ret;
  rlpStrm.exportBytesString(ret);
  return ret;
}

