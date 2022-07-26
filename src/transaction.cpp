#include "transaction.h"


// It might repeat the same code inside the conditions, but I believe it is better oganized this way.
Tx::Base::Base(std::string &bytes, bool fromDB) {
  if (!fromDB) {
    dev::RLP rlp(bytes);

    if (!rlp.isList()) {
      throw std::runtime_error("transaction RLP is not a list");
    }
    
    this->_nonce = rlp[0].toInt<uint256_t>();
    this->_gasPrice = rlp[1].toInt<uint256_t>();
    this->_gas = rlp[2].toInt<uint256_t>();
    if (!rlp[3].isData()) {
      throw std::runtime_error("recepient RLP must be a byte array");
    }

    this->_to = rlp[3].toHash<dev::h160>(dev::RLP::VeryStrict);
    this->_value = rlp[4].toInt<uint256_t>();


    if (!rlp[5].isData()) {

    }

    this->_data = rlp[5].toString();
    uint256_t const v = rlp[6].toInt<uint256_t>();
    uint256_t const r = rlp[7].toInt<uint256_t>();
    uint256_t const s = rlp[8].toInt<uint256_t>();

    if (v > 36) {
      auto const chainId = (v - 35) / 2;
      if (chainId > std::numeric_limits<uint64_t>::max()) {
        throw std::runtime_error("transaction chainId too high.");
      }
      this->_chainId = static_cast<uint64_t>(chainId);
    } else if ( v != 27 && v != 28 ) {
      throw std::runtime_error("Transaction signature invalid");
    }

    auto const recoveryId = uint8_t{v - (uint256_t(this->_chainId) * 2 + 35)};
    
    if (Utils::verifySignature(recoveryId, r, s)) {
      throw std::runtime_error("Transaction Signature invalid");;
    }

    // A signature: 65 bytes: r: [0, 32), s: [32, 64), v: 64.
    std::string sig = Utils::uint256ToBytes(r) + Utils::uint256ToBytes(s) + Utils::uint8ToBytes(recoveryId);
    this->hasSig = true;
    std::string messageHash = dev::sha3(this->rlpSerialize(false), false);

    auto pubKey = Secp256k1::recover(sig, messageHash);
    this->_from = dev::sha3(pubKey, false).substr(12,32);

    if (rlp.itemCount() > 9) {
      throw std::runtime_error("too many fields in the transaction RLP");
    }

  } else {
    std::string rlpBytes = bytes.substr(0,bytes.size() - 40);
    dev::RLP rlp(bytes);

  }
}

std::string Tx::Base::rlpSerialize(bool includeSig) {
  dev::RLPStream rlpStrm;

  // EIP-155 Compatible.
  // instead of hashing six rlp encoded elements (nonce, gasprice, startgas, to, value, data)
  // hash nine rlp encoded elements (nonce, gasprice, startgas, to, value, data, chainid, 0, 0) before signing.
  rlpStrm.appendList(9);
  rlpStrm << this->_nonce << this->_gasPrice << this->_gas 
          << this->_to.toHash() << this->_value << this->_data;
  if (includeSig) {
    rlpStrm << (this->_v + (this->_chainId * 2 + 35)) << this->_r << this->_s;
  } else {
    rlpStrm << this->_chainId << 0 << 0;
  }

  auto rlpBytes = rlpStrm.out();
  return [&]() -> std::string {
    std::string ret;
    std::copy(rlpBytes.begin(), rlpBytes.end(), std::back_inserter(ret));
    return ret;
  }();
}