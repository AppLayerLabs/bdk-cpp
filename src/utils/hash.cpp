#include "hash.h"
#include "utils.h"

Hash::Hash(const uint256_t& value) : Hash(Utils::uint256ToBytes(value)) {}

Hash::operator uint256_t() const {
  return Utils::bytesToUint256(*this);
}

View<Hash>::operator uint256_t() const {
  return Utils::bytesToUint256(*this);
}