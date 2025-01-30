#include "precompiles.h"
#include "utils/signature.h"
#include "utils/ecdsa.h"
#include <openssl/sha.h>
#include <openssl/ripemd.h>

Address ecrecover(View<Hash> hash, uint8_t v, View<Hash> r, View<Hash> s) {
  if (v == 27) {
    v = 0;
  } else if (v == 28) {
    v = 1;
  } else {
    return Address{};
  }

  Signature sig;
  *std::ranges::copy(s, std::ranges::copy(r, sig.begin()).out).out = v;

  const auto pubkey = Secp256k1::recover(sig, Hash(hash));

  if (!pubkey) {
    return Address{};
  }

  return Secp256k1::toAddress(pubkey);
}

Hash sha256(View<Bytes> input) {
  Hash output;
  SHA256(input.data(), input.size(), output.data());
  return output;
}

Bytes20 ripemd160(View<Bytes> input) {
  Bytes20 output;
  RIPEMD160(input.data(), input.size(), output.data()); // TODO: this is deprecated
  return output;
}
