#include "secp256k1Wrapper.h"



secp256k1_context const* getCtx()
{
    static std::unique_ptr<secp256k1_context, decltype(&secp256k1_context_destroy)> s_ctx{
        secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY),
        &secp256k1_context_destroy
    };
    return s_ctx.get();
}

std::string Secp256k1::recover(std::string sig, std::string messageHash) {
  int v = sig[64];
  if (v > 3)
    return "";

  auto* ctx = getCtx();

  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &rawSig, reinterpret_cast<const unsigned char*>(sig.data()), v))
    return "";

  secp256k1_pubkey rawPubkey;
  if(!secp256k1_ecdsa_recover(ctx, &rawPubkey, &rawSig, reinterpret_cast<const unsigned char*>(messageHash.data())))
    return "";

  std::array<uint8_t,65> serializedPubkey;
  size_t serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(ctx, serializedPubkey.data(), &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
  assert (serializedPubkeySize == serliazedPubkey.size());
  // Expect single byte header of value 0x04 -- uncompressed pubkey.
  assert(serializedPubkey[0] == "0x04");
  // return pubkey without the 0x04 header.
  return std::string(serializedPubkey.begin() + 1, serializedPubkey.end());
}