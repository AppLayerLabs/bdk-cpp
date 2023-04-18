#include "tx.h"

TxBlock::TxBlock(const std::string_view& bytes, const uint64_t& requiredChainId) {
  uint64_t index = 0;
  const auto txData = bytes.substr(1);

  // Check if Tx is type 2
  if (uint8_t(bytes[0]) != 0x02) throw std::runtime_error("Tx is not type 2");

  // Check if first byte is equal or higher than 0xf7, meaning it is a list
  if (uint8_t(txData[0]) < 0xf7) throw std::runtime_error("Tx is not a list");

  // Get list length
  uint8_t listLengthSize = uint8_t(txData[index]) - 0xf7;
  index++;
  uint64_t listLength = Utils::fromBigEndian<uint64_t>(
    std::string_view(&txData[index], listLengthSize)
  );
  index += listLengthSize; // Index is now at rlp[0] size

  // Size sanity check
  if (listLength < txData.size() - listLengthSize - 1) {
    throw std::runtime_error("Tx RLP returns smaller size than reported");
  } else if (listLength > txData.size() - listLengthSize - 1) {
    throw std::runtime_error("Tx RLP returns larger size than reported");
  }

  // If chainId > 0, get chainId from string.
  // chainId can be a small string or the byte itself
  const uint8_t chainIdLength = (uint8_t(txData[index]) >= 0x80)
                              ? uint8_t(txData[index]) - 0x80 : 0;
  if (chainIdLength != 0) {
    if (chainIdLength > 0x37) throw std::runtime_error("ChainId is too large");
    index++; // Index at rlp[0] payload
    uint64_t chainId = 0;
    this->chainId = Utils::fromBigEndian<uint64_t>(
      std::string_view(&txData[index], chainIdLength)
    );
    index += chainIdLength; // Index at rlp[1] size
  } else {
    this->chainId = (uint8_t(txData[index]) == 0x80)
                  ? 0 : Utils::fromBigEndian<uint64_t>(std::string_view(&txData[index], 1));
    index++; // Index at rlp[1] size
  }

  // If nonce > 0, get nonce from string.
  // nonce can be a small string or the byte itself
  const uint8_t nonceLength = (uint8_t(txData[index]) >= 0x80)
                                ? uint8_t(txData[index]) - 0x80 : 0;
  if (nonceLength != 0) {
    if (nonceLength > 0x37) throw std::runtime_error("Nonce is too large");
    index++; // Index at rlp[1] payload
    uint64_t nonce = 0;
    this->nonce = Utils::fromBigEndian<uint64_t>(
      std::string_view(&txData[index], nonceLength)
    );
    index += nonceLength; // Index at rlp[2] size
  } else {
    this->nonce = (uint8_t(txData[index]) == 0x80)
                    ? 0 : Utils::fromBigEndian<uint64_t>(std::string_view(&txData[index], 1));
    index++; // Index at rlp[2] size
  }

  // If maxPriorityFeePerGas > 0, get maxPriorityFeePerGas from string.
  // maxPriorityFeePerGas can be a small string or the byte itself
  const uint8_t maxPriorityFeePerGasLength = (uint8_t(txData[index]) >= 0x80)
                              ? uint8_t(txData[index]) - 0x80 : 0;
  if (maxPriorityFeePerGasLength != 0) {
    if (maxPriorityFeePerGasLength > 0x37) throw std::runtime_error("MaxPriorityFeePerGas is too large");
    index++; // Index at rlp[2] payload
    uint64_t maxPriorityFeePerGas = 0;
    this->maxPriorityFeePerGas = Utils::fromBigEndian<uint256_t>(
      std::string_view(&txData[index], maxPriorityFeePerGasLength)
    );
    index += maxPriorityFeePerGasLength; // Index at rlp[3] size
  } else {
    this->maxPriorityFeePerGas = (uint8_t(txData[index]) == 0x80)
                  ? 0 : Utils::fromBigEndian<uint256_t>(std::string_view(&txData[index], 1));
    index++; // Index at rlp[3] size
  }

  // If maxFeePerGas > 0, get nonce from string.
  // maxFeePerGas can be a small string or the byte itself
  const uint8_t maxFeePerGasLength = (uint8_t(txData[index]) >= 0x80)
                                             ? uint8_t(txData[index]) - 0x80 : 0;
  if (maxFeePerGasLength != 0) {
    if (maxFeePerGasLength > 0x37) throw std::runtime_error("MaxFeePerGas is too large");
    index++; // Index at rlp[3] payload
    uint64_t maxFeePerGas = 0;
    this->maxFeePerGas = Utils::fromBigEndian<uint256_t>(
      std::string_view(&txData[index], maxFeePerGasLength)
    );
    index += maxFeePerGasLength; // Index at rlp[4] size
  } else {
    this->maxFeePerGas = (uint8_t(txData[index]) == 0x80)
                                 ? 0 : Utils::fromBigEndian<uint256_t>(std::string_view(&txData[index], 1));
    index++; // Index at rlp[4] size
  }

  // If gasLimit > 0, get gasLimit from string.
  // gasLimit can be a small string or the byte itself
  const uint8_t gasLimitLength = (uint8_t(txData[index]) >= 0x80)
                                ? uint8_t(txData[index]) - 0x80 : 0;
  if (gasLimitLength != 0) {
    if (gasLimitLength > 0x37) throw std::runtime_error("GasLimit is too large");
    index++; // Index at rlp[0] payload
    uint64_t gasLimit = 0;
    this->gasLimit = Utils::fromBigEndian<uint64_t>(
      std::string_view(&txData[index], gasLimitLength)
    );
    index += gasLimitLength; // Index at rlp[1] size
  } else {
    this->gasLimit = (uint8_t(txData[index]) == 0x80)
                    ? 0 : Utils::fromBigEndian<uint64_t>(std::string_view(&txData[index], 1));
    index++; // Index at rlp[1] size
  }

  // Get receiver addess (to) - small string.
  // We don't actually need to get the size, because to/from has a size of 20
  if (uint8_t(txData[index]) != 0x94) throw std::runtime_error(
    "Receiver address (to) is not a 20 byte string (address)"
  );
  index++; // Index at rlp[5] payload
  this->to = Address(std::string_view(&txData[index], 20), true);
  index += 20; // Index at rlp[6] size

  // Get value - small string or byte itself.
  uint8_t valueLength = (uint8_t(txData[index]) >= 0x80 ? uint8_t(txData[index]) - 0x80 : 0);
  if (valueLength != 0) {
    if (valueLength > 0x37) throw std::runtime_error("Value is not a small string");
    index++; // Index at rlp[6] payload
    this->value = Utils::fromBigEndian<uint256_t>(
      std::string_view(&txData[index], valueLength)
    );
    index += valueLength; // Index at rlp[7] size
  } else {
    this->value = (uint8_t(txData[index]) == 0x80)
      ? 0 : Utils::fromBigEndian<uint256_t>(std::string_view(&txData[index], 1));
    index++; // Index at rlp[7] size
  }

  // Get data - it can be anything really, from nothing (0x80) to a big string (0xb7)
  if (uint8_t(txData[index]) < 0x80) {
    this->data = txData[index];
    index++; // Index at rlp[8] size
  } else if (uint8_t(txData[index]) < 0xb7) {
    uint8_t dataLength = txData[index] - 0x80;
    index++; // Index at rlp[8] payload
    if (dataLength > 0) {
      this->data = txData.substr(index, dataLength);
      index += dataLength; // Index at rlp[8] size
    }
  } else if (uint8_t(txData[index]) < 0xc0) {
    uint8_t dataLengthSize = txData[index] - 0xb7;
    index++; // Index at rlp[8] payload size
    uint64_t dataLength = Utils::fromBigEndian<uint64_t>(
      std::string_view(&txData[index], dataLengthSize)
    );
    index += dataLengthSize; // Index at rlp[8] payload
    this->data = txData.substr(index, dataLength);
    index += dataLength; // Index at rlp[8] size
  } else {
    throw std::runtime_error("Data is too large");
  }

  // Get access list
  // ALWAYS 0xc0 (empty list)
  if (uint8_t(txData[index]) != 0xc0) throw std::runtime_error("Access list is not empty");
  index++; // Index at rlp[9] size

  // Get v - always byte itself (1 byte)
  if (uint8_t(txData[index]) == 0x80) {
    this->v = 0;
  } else {
    this->v = Utils::fromBigEndian<uint8_t>(std::string_view(&txData[index], 1));
    if (this->v > 0x01) throw std::runtime_error("V is not 0 or 1");
  }
  ++index; // Index at rlp[10] size

  // Get r - small string
  uint8_t rLength = txData[index] - 0x80;
  if (rLength > 0x20) throw std::runtime_error("R is bigger than 32 bytes");
  index++; // Index at rlp[10] payload
  this->r = Utils::fromBigEndian<uint256_t>(std::string_view(&txData[index], rLength));
  index += rLength; // Index at rlp[11] size

  // Get s - small string
  uint8_t sLength = txData[index] - 0x80;
  if (sLength > 0x20) throw std::runtime_error("S is bigger than 32 bytes");
  index++; // Index at rlp[11] payload
  this->s = Utils::fromBigEndian<uint256_t>(std::string_view(&txData[index], sLength));
  index += sLength; // Index at rlp[12] size

  if (!Secp256k1::verifySig(this->r, this->s, this->v)) {
    throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  Signature sig = Secp256k1::makeSig(this->r, this->s, this->v);
  std::string txMessage = this->rlpSerialize(false);
  Hash msgHash = Utils::sha3(txMessage); // Do not include signature
  UPubKey key = Secp256k1::recover(sig, msgHash);
  if (!key) throw std::runtime_error("Invalid tx signature - cannot recover public key");

  this->from = Secp256k1::toAddress(key);
}

TxBlock::TxBlock(
  const Address to, const Address from, const std::string data,
  const uint64_t chainId, const uint256_t nonce, const uint256_t value,
  const uint256_t maxPriorityFeePerGas, const uint256_t maxFeePerGas, const uint256_t gasLimit, const PrivKey privKey
) : to(to), from(from), data(data), chainId(chainId), nonce(nonce),
  value(value), maxPriorityFeePerGas(maxPriorityFeePerGas), maxFeePerGas(maxFeePerGas), gasLimit(gasLimit)
{
  UPubKey pubKey = Secp256k1::toUPub(privKey);
  Address add = Secp256k1::toAddress(pubKey);
  if (add != this->from) throw std::runtime_error("Private key does not match sender address (from)");

  Hash hash = this->hash(false);
  Signature sig = Secp256k1::sign(hash, privKey);
  this->r = Utils::bytesToUint256(sig.view(0, 32));
  this->s = Utils::bytesToUint256(sig.view(32,32));
  this->v = sig[64];

  if (pubKey != Secp256k1::recover(sig, hash)) {
    throw std::runtime_error("Invalid tx signature - derived key doesn't match public key");
  }
  if (!Secp256k1::verifySig(this->r, this->s, this->v)) {
    throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
  }
}

std::string TxBlock::rlpSerialize(bool includeSig) const {
  std::string ret("\x02");
  uint64_t total_size = 0;
  uint64_t reqBytesChainId = Utils::bytesRequired(this->chainId);
  uint64_t reqBytesNonce = Utils::bytesRequired(this->nonce);
  uint64_t reqBytesMaxPriorityFeePerGas = Utils::bytesRequired(this->maxPriorityFeePerGas);
  uint64_t reqBytesMaxFeePerGas = Utils::bytesRequired(this->maxFeePerGas);
  uint64_t reqBytesGasLimit = Utils::bytesRequired(this->gasLimit);
  uint64_t reqBytesValue = Utils::bytesRequired(this->value);
  uint64_t reqBytesData = this->data.size();
  uint64_t reqBytesR = Utils::bytesRequired(this->r);
  uint64_t reqBytesS = Utils::bytesRequired(this->s);

  // Calculate total sizes
  total_size += (this->chainId < 0x80) ? 1 : 1 + reqBytesChainId;
  total_size += (this->nonce < 0x80) ? 1 : 1 + reqBytesNonce;
  total_size += (this->maxPriorityFeePerGas < 0x80) ? 1 : 1 + reqBytesMaxPriorityFeePerGas;
  total_size += (this->maxFeePerGas < 0x80) ? 1 : 1 + reqBytesMaxFeePerGas;
  total_size += (this->gasLimit < 0x80) ? 1 : 1 + reqBytesGasLimit;
  total_size += 1 + 20; // To
  total_size += (this->value < 0x80) ? 1 : 1 + reqBytesValue;
  total_size += 1; // Access List

  if (this->data.size() == 0) {
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
    ret += char(total_size + 0xc0);
  } else {
    uint64_t sizeBytes = Utils::bytesRequired(total_size);
    ret.reserve(total_size + sizeBytes + 1);
    ret += char(sizeBytes + 0xf7);
    ret += Utils::uintToBytes(total_size);
  }

  // chainId
  if (this->chainId == 0) {
    ret += char(0x80);
  } else if (this->chainId < 0x80) {
    ret += char(this->chainId);
  } else {
    ret += char(reqBytesChainId + 0x80);
    ret += Utils::uintToBytes(this->chainId);
  }
  // Nonce
  if (this->nonce == 0) {
    ret += char(0x80);
  } else if (this->nonce < 0x80) {
    ret += char(this->nonce);
  } else {
    ret += char(reqBytesNonce + 0x80);
    ret += Utils::uintToBytes(this->nonce);
  }

  // max Priority Fee Per Gas
  if (this->maxPriorityFeePerGas == 0) {
    ret += char(0x80);
  } else if (this->maxPriorityFeePerGas < 0x80) {
    ret += char(this->maxPriorityFeePerGas);
  } else {
    ret += char(reqBytesMaxPriorityFeePerGas + 0x80);
    ret += Utils::uintToBytes(this->maxPriorityFeePerGas);
  }

  // max Fee Per Gas
  if (this->maxFeePerGas == 0) {
    ret += char(0x80);
  } else if (this->maxFeePerGas < 0x80) {
    ret += char(this->maxFeePerGas);
  } else {
    ret += char(reqBytesMaxFeePerGas + 0x80);
    ret += Utils::uintToBytes(this->maxFeePerGas);
  }

  // Gas Limit
  if (this->gasLimit == 0) {
    ret += char(0x80);
  } else if (this->gasLimit < 0x80) {
    ret += char(this->gasLimit);
  } else {
    ret += char(reqBytesGasLimit + 0x80);
    ret += Utils::uintToBytes(this->gasLimit);
  }

  // To
  ret += char(0x94);
  ret += this->to.get();

  // Value
  if (this->value == 0) {
    ret += char(0x80);
  } else if (this->value < 0x80) {
    ret += char(this->value);
  } else {
    ret += char(reqBytesValue + 0x80);
    ret += Utils::uintToBytes(this->value);
  }

  // Data
  if (this->data.size() == 0) {
    ret += char(0x80);
  } else if (reqBytesData <= 55) {
    ret += char(reqBytesData + 0x80);
    ret += this->data;
  } else {
    ret += char(Utils::bytesRequired(reqBytesData) + 0xb7);
    ret += Utils::uintToBytes(reqBytesData);
    ret += this->data;
  }

  // Access List
  ret += char(0xc0);

  // V/chainId
  if (includeSig) {
    ret += (this->v == 0x00) ? char(0x80) : char(this->v);
  }

  // R
  if (includeSig) {
    ret += char(reqBytesR + 0x80);
    ret += Utils::uintToBytes(this->r);
  }

  // S
  if (includeSig) {
    ret += char(reqBytesS + 0x80);
    ret += Utils::uintToBytes(this->s);
  }
  return ret;
}

ethCallInfo TxBlock::txToCallInfo() const {
  // ethCallInfo: tuple of (from, to, gasLimit, gasPrice, value, data)
  ethCallInfo ret;
  auto& [from, to, gasLimit, gasPrice, value, data] = ret;
  from = this->getFrom();
  to = this->getTo();
  gasLimit = this->getGasLimit();
  gasPrice = this->getMaxFeePerGas();
  value = this->getValue();
  data = this->getData();
  return ret;
}

TxValidator::TxValidator(const std::string_view& bytes, const uint64_t& requiredChainId) {
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
  } else if (listLength > bytes.size() - listLengthSize - 1) {
    throw std::runtime_error("Tx RLP returns larger size than reported");
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
    this->nHeight = Utils::fromBigEndian<uint64_t>(
      std::string_view(&bytes[index], nHeightLength)
    );
    index += nHeightLength; // Index at rlp[2] size
  } else {
    this->nHeight = (uint8_t(bytes[index]) == 0x80)
      ? 0 : Utils::fromBigEndian<uint64_t>(std::string_view(&bytes[index], 1));
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
    this->v = (uint8_t(bytes[index]) == 0x80)
      ? 0 : Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], 1));
    index++; // Index at rlp[3] size
  }

  // Get r - small string
  uint8_t rLength = bytes[index] - 0x80;
  if (rLength > 0x37) throw std::runtime_error("R is not a small string");
  index++; // Index at rlp[3] payload
  this->r = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], rLength));
  index += rLength; // Index at rlp[4] size

  // Get s - small string
  uint8_t sLength = bytes[index] - 0x80;
  if (sLength > 0x37) throw std::runtime_error("S is not a small string");
  index++; // Index at rlp[4] payload
  this->s = Utils::fromBigEndian<uint256_t>(std::string_view(&bytes[index], sLength));
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
  UPubKey key = Secp256k1::recover(sig, msgHash);
  if (key == UPubKey()) throw std::runtime_error("Invalid tx signature - cannot recover public key");
  this->from = Secp256k1::toAddress(key);
}

TxValidator::TxValidator(
  const Address from, const std::string data, const uint64_t chainId,
  const uint64_t nHeight, const PrivKey privKey
) : from(from), data(data), chainId(chainId), nHeight(nHeight) {
  UPubKey pubKey = Secp256k1::toUPub(privKey);
  Address add = Secp256k1::toAddress(pubKey);
  Hash hash = this->hash(false);
  if (add != this->from) throw std::runtime_error("Private key does not match sender address (from)");

  Signature sig = Secp256k1::sign(hash, privKey);
  this->r = Utils::bytesToUint256(sig.view(0, 32));
  this->s = Utils::bytesToUint256(sig.view(32,32));
  uint8_t recoveryIds = sig[64];
  this->v = recoveryIds + (this->chainId * 2 + 35);

  if (!Secp256k1::verifySig(this->r, this->s, recoveryIds)) {
    throw std::runtime_error("Invalid tx signature - doesn't fit elliptic curve verification");
  }
  if (pubKey != Secp256k1::recover(sig, hash)) {
    throw std::runtime_error("Invalid transaction signature, signature derived key doens't match public key");
  }
}

std::string TxValidator::rlpSerialize(bool includeSig) const {
  std::string ret;
  uint64_t total_size = 0;
  uint64_t reqBytesData = this->data.size();
  uint64_t reqBytesnHeight = Utils::bytesRequired(this->nHeight);
  uint64_t reqBytesV = Utils::bytesRequired((includeSig) ? this->v : this->chainId);
  uint64_t reqBytesR = Utils::bytesRequired(this->r);
  uint64_t reqBytesS = Utils::bytesRequired(this->s);

  // Calculate total sizes
  if (this->data.size() == 0) {
    total_size += 1;
  } else if (reqBytesData <= 55) {
    total_size += 1 + reqBytesData;
  } else {
    total_size += 1 + Utils::bytesRequired(reqBytesData) + reqBytesData;
  }

  total_size += (this->nHeight < 0x80) ? 1 : 1 + reqBytesnHeight;

  if (includeSig) {
    total_size += (this->v < 0x80) ? 1 : 1 + reqBytesV;
  } else {
    total_size += (this->chainId < 0x80) ? 1 : 1 + reqBytesV;
  }

  total_size += (!includeSig) ? 1 : 1 + reqBytesR;
  total_size += (!includeSig) ? 1 : 1 + reqBytesS;

  // Serialize everything
  if (total_size <= 55) {
    ret.reserve(total_size + 1);
    ret += char(total_size + 0xc0);
  } else {
    uint64_t sizeBytes = Utils::bytesRequired(total_size);
    ret.reserve(total_size + sizeBytes + 1);
    ret += char(sizeBytes + 0xf7);
    ret += Utils::uintToBytes(total_size);
  }

  // Data
  if (this->data.size() == 0) {
    ret += char(0x80);
  } else if (reqBytesData <= 55) {
    ret += char(reqBytesData + 0x80);
    ret += this->data;
  } else {
    ret += char(Utils::bytesRequired(reqBytesData) + 0xb7);
    ret += Utils::uintToBytes(reqBytesData);
    ret += this->data;
  }

  // nHeight
  if (this->nHeight == 0) {
    ret += char(0x80);
  } else if (this->nHeight < 0x80) {
    ret += char(this->nHeight);
  } else {
    ret += char(reqBytesnHeight + 0x80);
    ret += Utils::uintToBytes(this->nHeight);
  }

  // V/chainId
  if (includeSig) {
    if (this->v < 0x80) {
      ret += char(this->v);
    } else {
      ret += char(reqBytesV + 0x80);
      ret += Utils::uintToBytes(this->v);
    }
  } else {
    if (this->chainId < 0x80) {
      ret += char(this->chainId);
    } else {
      ret += char(reqBytesV + 0x80);
      ret += Utils::uintToBytes(this->chainId);
    }
  }

  // R
  if (!includeSig) {
    ret += char(0x80);
  } else {
    ret += char(reqBytesR + 0x80);
    ret += Utils::uintToBytes(this->r);
  }

  // S
  if (!includeSig) {
    ret += char(0x80);
  } else {
    ret += char(reqBytesS + 0x80);
    ret += Utils::uintToBytes(this->s);
  }

  return ret;
}

