#ifndef BDK_CONTRACT_PRECOMPILES_H
#define BDK_CONTRACT_PRECOMPILES_H

#include "utils/address.h"
#include "utils/hash.h"

Address ecrecover(View<Hash> hash, uint8_t v, View<Hash> r, View<Hash> s);

#endif // BDK_CONTRACT_PRECOMPILES_H
