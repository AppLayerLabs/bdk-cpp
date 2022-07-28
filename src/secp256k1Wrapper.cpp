#include "secp256k1Wrapper.h"

secp256k1_context const* Secp256k1::getCtx() {
  static std::unique_ptr<secp256k1_context, decltype(&secp256k1_context_destroy)> s_ctx {
    secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY),
    &secp256k1_context_destroy
  };
  return s_ctx.get();
}

std::string Secp256k1::recover(std::string& sig, std::string& messageHash) {
  int v = sig[64];
  if (v > 3) return "";
  auto* ctx = getCtx();
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
  assert (serializedPubkeySize == serliazedPubkey.size());
  // Expect single byte header of value 0x04 -- uncompressed pubkey.
  assert(serializedPubkey[0] == "0x04");
  // return pubkey without the 0x04 header.
  return std::string(serializedPubkey.begin() + 1, serializedPubkey.end());
}

void Secp256k1::appendSignature(const uint256_t &r, const uint256_t &s, const uint8_t &v, std::string &signature) {
  signature = std::string(65, 0x00);  // r = [0, 32], s = [32, 64], v = 65
  std::string tmpR;
  boost::multiprecision::export_bits(r, std::back_inserter(tmpR), 8);
  for (uint16_t i = 0; i < tmpR.size(); ++i) {
    signature[31-i] = tmpR[31-i];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  std::string tmpS;
  boost::multiprecision::export_bits(s, std::back_inserter(tmpS), 8);
  for (uint16_t i = 0; i < tmpS.size(); ++i) {
    signature[63-i] = tmpS[31-i];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  signature[64] = v;
  return;
}

