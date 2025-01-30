#ifndef BDK_CONTRACT_PRECOMPILES_H
#define BDK_CONTRACT_PRECOMPILES_H

#include "utils/address.h"
#include "utils/hash.h"
#include "utils/fixedbytes.h"

Address ecrecover(View<Hash> hash, uint8_t v, View<Hash> r, View<Hash> s);

Hash sha256(View<Bytes> input);

Bytes20 ripemd160(View<Bytes> input);

#endif // BDK_CONTRACT_PRECOMPILES_H
