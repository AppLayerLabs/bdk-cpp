#include "ecdsa.h"

const secp256k1_context* Secp256k1::getCtx() {
  static std::unique_ptr<secp256k1_context, ContextDeleter> s_ctx{
    secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY)
  };
  return s_ctx.get();
}

UPubKey Secp256k1::recover(const Signature& sig, const Hash& msg) {
  uint8_t v = sig[64];
  if (v > 3) return UPubKey();
  auto* ctx = Secp256k1::getCtx();

  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
    ctx, &rawSig, sig.raw(), v
  )) return UPubKey();

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ecdsa_recover(ctx, &rawPubkey, &rawSig,
                               msg.raw()
  )) return UPubKey();

  BytesArr<65> serializedPubkey;
  size_t serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(
    ctx, serializedPubkey.data(),
    &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED
  );

  assert(serializedPubkeySize == serializedPubkey.size());
  assert(serializedPubkey[0] == 0x04); // Expect single byte header of value 0x04 = uncompressed pubkey

  return serializedPubkey;  // Return pubkey without the 0x04 header
}

Signature Secp256k1::makeSig(const uint256_t& r, const uint256_t& s, const uint8_t& v) {
  BytesArr<65> sig;  // r = [0, 32], s = [32, 64], v = 65
  auto tmpR = Utils::uint256ToBytes(r);
  for (uint16_t i = 0; i < tmpR.size(); i++) {
    // Replace bytes from tmp to ret to make it 32 bytes in size
    sig[31 - i] = tmpR[tmpR.size() - i - 1];
  }
  auto tmpS = Utils::uint256ToBytes(s);
  for (uint16_t i = 0; i < tmpS.size(); i++) {
    // Replace bytes from tmp to ret to make it 32 bytes in size
    sig[63 - i] = tmpS[tmpS.size() - i - 1];
  }
  std::memcpy(&sig[64], &v, 1);
  return Signature { std::move(sig) };
}

bool Secp256k1::verifySig(const uint256_t& r, const uint256_t& s, const uint8_t& v) {
  return (v <= 1 && r > 0 && s > 0 && r < Secp256k1::ecConst && s < Secp256k1::ecConst);
}

UPubKey Secp256k1::toUPub(const PrivKey& key) {
  auto* ctx = Secp256k1::getCtx();

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_create(
    ctx, &rawPubkey, key.raw()
  )) return UPubKey();

  BytesArr<65> serializedPubkey;
  auto serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(
    ctx, serializedPubkey.data(),
    &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED
  );

  // Expect single byte header of value 0x04 = uncompressed pubkey
  assert(serializedPubkey.size() == serializedPubkeySize);
  assert(serializedPubkey[0] == 0x04);
  return UPubKey(std::move(serializedPubkey));
}

UPubKey Secp256k1::toUPub(const PubKey& key) {
  auto* ctx = Secp256k1::getCtx();

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_parse(
    ctx, &rawPubkey, key.raw(), key.size()
  )) return UPubKey();

  BytesArr<65> serializedPubkey;
  auto serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(
    ctx, serializedPubkey.data(),
    &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED
  );

  // Expect single byte header of value 0x04 = uncompressed pubkey
  assert(serializedPubkey.size() == serializedPubkeySize);
  assert(serializedPubkey[0] == 0x04);
  return UPubKey(std::move(serializedPubkey));
}

PubKey Secp256k1::toPub(const PrivKey& key) {
  auto* ctx = Secp256k1::getCtx();

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_create(
    ctx, &rawPubkey, key.raw()
  )) return PubKey();

  BytesArr<33> serializedPubkey;
  auto serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(
    ctx, serializedPubkey.data(),
    &serializedPubkeySize, &rawPubkey, SECP256K1_EC_COMPRESSED
  );

  // Expect single byte header of value 0x02 or 0x03 == compressed pubkey
  assert(serializedPubkey.size() == serializedPubkeySize);
  assert(serializedPubkey[0] == 0x02 || serializedPubkey[0] == 0x03);
  return serializedPubkey;
}

Address Secp256k1::toAddress(const UPubKey& key) {
  // Address = pubKeyHash[12..32], no "0x"
  return Address(Utils::sha3(key.view_const(1, 64)).view_const(12));
}

Address Secp256k1::toAddress(const PubKey& key) {
  return Secp256k1::toAddress(Secp256k1::toUPub(key));
}

Signature Secp256k1::sign(const Hash& msg, const PrivKey& key) {
  auto* ctx = Secp256k1::getCtx();

  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_sign_recoverable(ctx, &rawSig,
    msg.raw(),
    key.raw(),
    nullptr, nullptr
  )) return Signature();

  int v = 0;


  BytesArr<64> sig;
  secp256k1_ecdsa_recoverable_signature_serialize_compact(
    ctx, sig.data(), &v, &rawSig
  );

  auto sigView = Utils::create_view_span(sig);
  uint8_t rawV = v;
  uint256_t r = Utils::bytesToUint256(sigView.subspan(0, 32));
  uint256_t s = Utils::bytesToUint256(sigView.subspan(32, 32));

  if (s > (Secp256k1::ecConst / 2)) {
    rawV = static_cast<uint8_t>(rawV ^ 1);
    s = uint256_t(Secp256k1::ecConst - uint256_t(s));
  }

  assert (s <= (Secp256k1::ecConst / 2));
  return Secp256k1::makeSig(r, s, rawV);
}

bool Secp256k1::verify(const Hash& msg, const UPubKey& key, const Signature& sig) {
  auto* ctx = Secp256k1::getCtx();

  secp256k1_ecdsa_signature rawSig;
  if (!secp256k1_ecdsa_signature_parse_compact(
    ctx, &rawSig, reinterpret_cast<const unsigned char*>(sig.raw())
  )) {
    return false;
  }

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_parse(ctx, &rawPubkey,
                                 reinterpret_cast<const unsigned char*>(key.raw()), key.size()
  )) {
    return false;
  }

  int ret = secp256k1_ecdsa_verify(ctx, &rawSig,
                                   reinterpret_cast<const unsigned char*>(msg.raw()), &rawPubkey
  );

  return ret;
}

