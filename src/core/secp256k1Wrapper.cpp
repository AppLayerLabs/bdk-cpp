#include "secp256k1Wrapper.h"

std::string Secp256k1::recover(const std::string& sig, const std::string& messageHash) {
  int v = sig[64];
  if (v > 3) return "";
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
    ctx, &rawSig, reinterpret_cast<const unsigned char*>(sig.data()), v)
  ) return "";
  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ecdsa_recover(
    ctx, &rawPubkey, &rawSig, reinterpret_cast<const unsigned char*>(messageHash.data()))
  ) return "";
  std::array<uint8_t,65> serializedPubkey;
  size_t serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(ctx, serializedPubkey.data(), &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
  secp256k1_context_destroy(ctx);
  assert (serializedPubkeySize == serializedPubkey.size());
  // Expect single byte header of value 0x04 -- uncompressed pubkey.
  assert(serializedPubkey[0] == 0x04);
  // return pubkey without the 0x04 header.
  return { serializedPubkey.begin(), serializedPubkey.end() };
}

void Secp256k1::appendSignature(const uint256_t &r, const uint256_t &s, const uint8_t &v, std::string &signature) {
  signature = std::string(65, 0x00);  // r = [0, 32], s = [32, 64], v = 65
  std::string tmpR;
  boost::multiprecision::export_bits(r, std::back_inserter(tmpR), 8);
  for (uint16_t i = 0; i < tmpR.size(); ++i) {
    signature[31-i] = tmpR[tmpR.size()-i-1];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  std::string tmpS;
  boost::multiprecision::export_bits(s, std::back_inserter(tmpS), 8);
  for (uint16_t i = 0; i < tmpS.size(); ++i) {
    signature[63-i] = tmpS[tmpS.size()-i-1];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  std::memcpy(&signature[64], &v, 1);
  return;
}

bool Secp256k1::verify(const std::string& pubkey, const std::string& sig, const std::string msghash) {
    auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    secp256k1_ecdsa_signature rawSig;
    if (!secp256k1_ecdsa_signature_parse_compact(ctx, &rawSig, reinterpret_cast<const unsigned char*>(sig.data()))) {
        secp256k1_context_destroy(ctx);
        return false;
    }
    secp256k1_pubkey rawPubkey;
    if (!secp256k1_ec_pubkey_parse(ctx, &rawPubkey, reinterpret_cast<const unsigned char*>(pubkey.data()), pubkey.size())) {
        secp256k1_context_destroy(ctx);
        return false;
    }
    secp256k1_ecdsa_signature_normalize(ctx, &rawSig, &rawSig);
    int ret = secp256k1_ecdsa_verify(ctx, &rawSig, reinterpret_cast<const unsigned char*>(msghash.data()), &rawPubkey);
    secp256k1_context_destroy(ctx);
    return ret;
}

std::string Secp256k1::toPub(const std::string &privKey) {
  Utils::logToFile(std::string("Privkey: ") + Utils::bytesToHex(privKey) + std::to_string(privKey.size()));
  if (privKey.size() != 32) { return ""; }
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_create(ctx, &rawPubkey, reinterpret_cast<const unsigned char*>(privKey.data()))) {
    return "";
  }

  std::array<byte, 65> serializedPubkey;
  auto serializedPubkeySize = serializedPubkey.size();

  secp256k1_ec_pubkey_serialize(ctx, serializedPubkey.data(), &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
  assert (serializedPubkey.size() == serializedPubkeySize);
  // Expect single byte header of value 0x04 -- uncompressed pubkey.
  assert(serializedPubkey[0] == 0x04);
  // return pubkey without the 0x04 header.
  return { serializedPubkey.begin() + 1, serializedPubkey.end() };
}

std::string Secp256k1::toAddress(const std::string &pubKey) {
  return Utils::sha3(std::string_view(&pubKey[1], 64)).get().substr(12); // Address = pubKeyHash[12..32], no "0x"
}

std::string Secp256k1::sign(const std::string &privKey, const std::string &hash) {
  if (privKey.size() != 32 && hash.size() != 32) { return ""; }
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_sign_recoverable(ctx, &rawSig, reinterpret_cast<const unsigned char*>(hash.data()), reinterpret_cast<const unsigned char*>(privKey.data()), nullptr, nullptr)) {
    return "";
  }

  int v = 0;
  std::string signature(65, 0x00);
  secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, reinterpret_cast<unsigned char*>(&signature[0]), &v, &rawSig);

  uint8_t rawV = static_cast<uint8_t>(v);
  uint256_t r = Utils::bytesToUint256(signature.substr(0,32));
  uint256_t s = Utils::bytesToUint256(signature.substr(32,32));
  
  if (s > c_secp256k1n / 2 ) {
    rawV = static_cast<uint8_t>(rawV ^ 1);
    s = uint256_t(c_secp256k1n - uint256_t(s));
  }

  assert (s <= c_secp256k1n / 2);
  Secp256k1::appendSignature(r, s, v, signature);
  return signature;
}

