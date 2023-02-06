#include "ecdsa.h"

UPubKey Secp256k1::recover(const Signature& sig, const Hash& msg) {
  int v = sig[64];
  if (v > 3) { return UPubKey(); }
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
    ctx, &rawSig, reinterpret_cast<const unsigned char*>(sig.view().data()), v
  )) return UPubKey();

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ecdsa_recover(ctx, &rawPubkey, &rawSig,
    reinterpret_cast<const unsigned char*>(msg.view().data())
  )) return UPubKey();

  std::string serializedPubkey(65, 0x00);
  size_t serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(
    ctx, reinterpret_cast<unsigned char*>(&serializedPubkey[0]),
    &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED
  );

  secp256k1_context_destroy(ctx);
  assert(serializedPubkeySize == serializedPubkey.size());
  assert(serializedPubkey[0] == 0x04); // Expect single byte header of value 0x04 = uncompressed pubkey

  return UPubKey(std::move(serializedPubkey));  // Return pubkey without the 0x04 header
}

Signature Secp256k1::makeSig(const uint256_t& r, const uint256_t& s, const uint8_t& v) {
  std::string sig(65, 0x00);  // r = [0, 32], s = [32, 64], v = 65
  std::string tmpR;
  boost::multiprecision::export_bits(r, std::back_inserter(tmpR), 8);
  for (uint16_t i = 0; i < tmpR.size(); i++) {
    // Replace bytes from tmp to ret to make it 32 bytes in size
    sig[31 - i] = tmpR[tmpR.size() - i - 1];
  }
  std::string tmpS;
  boost::multiprecision::export_bits(s, std::back_inserter(tmpS), 8);
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
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_create(
    ctx, &rawPubkey, reinterpret_cast<const unsigned char*>(&key[0])
  )) return UPubKey();

  std::string serializedPubkey (65, 0x00);
  auto serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(
    ctx, reinterpret_cast<unsigned char*>(&serializedPubkey[0]),
    &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED
  );

  // Expect single byte header of value 0x04 = uncompressed pubkey
  assert(serializedPubkey.size() == serializedPubkeySize);
  assert(serializedPubkey[0] == 0x04);
  return UPubKey(std::move(serializedPubkey));
}

UPubKey Secp256k1::toUPub(const PubKey& key) {
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_parse(
    ctx, &rawPubkey, reinterpret_cast<const unsigned char*>(key.view().data()), 33
  )) return UPubKey();

  std::string serializedPubkey (65, 0x00);
  auto serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(
    ctx, reinterpret_cast<unsigned char*>(&serializedPubkey[0]),
    &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED
  );

  // Expect single byte header of value 0x04 = uncompressed pubkey
  assert(serializedPubkey.size() == serializedPubkeySize);
  assert(serializedPubkey[0] == 0x04);
  return UPubKey(std::move(serializedPubkey));
}

PubKey Secp256k1::toPub(const PrivKey& key) {
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_create(
    ctx, &rawPubkey, reinterpret_cast<const unsigned char*>(key.view().data())
  )) return PubKey();

  std::string serializedPubkey(33, 0x00);
  auto serializedPubkeySize = serializedPubkey.size();
  secp256k1_ec_pubkey_serialize(
    ctx, reinterpret_cast<unsigned char*>(&serializedPubkey[0]),
    &serializedPubkeySize, &rawPubkey, SECP256K1_EC_COMPRESSED
  );

  // Expect single byte header of value 0x02 or 0x03 == compressed pubkey
  assert(serializedPubkey.size() == serializedPubkeySize);
  assert(serializedPubkey[0] == 0x02 || serializedPubkey[0] == 0x03);
  return serializedPubkey;
}

Address Secp256k1::toAddress(const UPubKey& key) {
  // Address = pubKeyHash[12..32], no "0x"
  return Address(Utils::sha3(std::string_view(&key[1], 64)).get().substr(12), false);
}

Address Secp256k1::toAddress(const PubKey& key) {
  return Secp256k1::toAddress(Secp256k1::toUPub(key));
}

Signature Secp256k1::sign(const Hash& msg, const PrivKey& key) {
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_sign_recoverable(ctx, &rawSig,
    reinterpret_cast<const unsigned char*>(msg.view().data()),
    reinterpret_cast<const unsigned char*>(key.view().data()),
    nullptr, nullptr
  )) return Signature();

  int v = 0;
  std::string sig(65, 0x00);
  secp256k1_ecdsa_recoverable_signature_serialize_compact(
    ctx, reinterpret_cast<unsigned char*>(&sig[0]), &v, &rawSig
  );

  uint8_t rawV = static_cast<uint8_t>(v);
  uint256_t r = Utils::bytesToUint256(sig.substr(0,32));
  uint256_t s = Utils::bytesToUint256(sig.substr(32,32));

  if (s > (Secp256k1::ecConst / 2)) {
    // TODO: unused variable rawV
    rawV = static_cast<uint8_t>(rawV ^ 1);
    s = uint256_t(Secp256k1::ecConst - uint256_t(s));
  }

  assert (s <= (Secp256k1::ecConst / 2));
  return Secp256k1::makeSig(r, s, v);
}

bool Secp256k1::verify(const Hash& msg, const UPubKey& key, const Signature& sig) {
  auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  secp256k1_ecdsa_signature rawSig;
  if (!secp256k1_ecdsa_signature_parse_compact(
    ctx, &rawSig, reinterpret_cast<const unsigned char*>(sig.view().data())
  )) {
    secp256k1_context_destroy(ctx);
    return false;
  }

  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ec_pubkey_parse(ctx, &rawPubkey,
    reinterpret_cast<const unsigned char*>(key.view().data()), key.size()
  )) {
    secp256k1_context_destroy(ctx);
    return false;
  }

  secp256k1_ecdsa_signature_normalize(ctx, &rawSig, &rawSig);
  int ret = secp256k1_ecdsa_verify(ctx, &rawSig,
    reinterpret_cast<const unsigned char*>(msg.view().data()), &rawPubkey
  );

  secp256k1_context_destroy(ctx);
  return ret;
}

