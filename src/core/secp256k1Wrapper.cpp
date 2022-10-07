#include "secp256k1Wrapper.h"

UncompressedPubkey Secp256k1::recover(const Signature& sig, const Hash& messageHash) {
  int v = sig[64];
  if (v > 3) return UncompressedPubkey();
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
    ctx, &rawSig, reinterpret_cast<const unsigned char*>(sig.data()), v)
  ) return UncompressedPubkey();
  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ecdsa_recover(
    ctx, &rawPubkey, &rawSig, reinterpret_cast<const unsigned char*>(messageHash.data()))
  ) return UncompressedPubkey();
  UncompressedPubkey serializedPubkey;
  size_t serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(ctx, reinterpret_cast<unsigned char*>(&serializedPubkey[0]), &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
  secp256k1_context_destroy(ctx);
  assert (serializedPubkeySize == serializedPubkey.size());
  // Expect single byte header of value 0x04 -- uncompressed pubkey.
  assert(serializedPubkey[0] == 0x04);
  // return pubkey without the 0x04 header.
  return serializedPubkey;
}

Signature Secp256k1::appendSignature(const uint256_t &r, const uint256_t &s, const uint8_t &v) {
  std::string signature(65, 0x00);  // r = [0, 32], s = [32, 64], v = 65
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
  Signature sig(std::move(signature));
  return sig;
}

bool Secp256k1::verify(const UncompressedPubkey& pubkey, const Signature& sig, const Hash& msghash) {
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

UncompressedPubkey Secp256k1::toPub(const PrivKey &privKey) {
  if (privKey.size() != 32) { return UncompressedPubkey(); }
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_create(ctx, &rawPubkey, reinterpret_cast<const unsigned char*>(privKey.data()))) {
    return UncompressedPubkey();
  }

  UncompressedPubkey serializedPubkey;
  auto serializedPubkeySize = serializedPubkey.size();

  secp256k1_ec_pubkey_serialize(ctx, reinterpret_cast<unsigned char*>(&serializedPubkey[0]), &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
  assert (serializedPubkey.size() == serializedPubkeySize);
  // Expect single byte header of value 0x04 -- uncompressed pubkey.
  assert(serializedPubkey[0] == 0x04);
  return serializedPubkey;
}

Address Secp256k1::toAddress(const UncompressedPubkey &pubKey) {
  return Address(Utils::sha3(std::string_view(&pubKey[1], 64)).get().substr(12), false); // Address = pubKeyHash[12..32], no "0x"
}

Signature Secp256k1::sign(const PrivKey &privKey, const Hash &hash) {
  if (privKey.size() != 32 && hash.size() != 32) { return Signature(); }
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_sign_recoverable(ctx, &rawSig, reinterpret_cast<const unsigned char*>(hash.data()), reinterpret_cast<const unsigned char*>(privKey.data()), nullptr, nullptr)) {
    return Signature();
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
  return Secp256k1::appendSignature(r, s, v);
}
