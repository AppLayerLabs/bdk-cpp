#ifndef BDK_CONTRACT_PRECOMPILES_H
#define BDK_CONTRACT_PRECOMPILES_H

#include "utils/address.h"
#include "utils/hash.h"
#include "utils/fixedbytes.h"

Address ecrecover(View<Hash> hash, uint8_t v, View<Hash> r, View<Hash> s);

Hash sha256(View<Bytes> input);

Bytes20 ripemd160(View<Bytes> input);

void blake2f(std::span<uint64_t, 8> h, std::span<const uint64_t, 16> m, 
             uint64_t c0, uint64_t c1, bool flag, uint32_t rounds);

#endif // BDK_CONTRACT_PRECOMPILES_H
