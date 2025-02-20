#include "precompiles.h"
#include "utils/signature.h"
#include "utils/ecdsa.h"

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
