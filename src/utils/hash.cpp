#include "hash.h"
#include "utils.h"

Hash::Hash(const uint256_t& value) : Hash(UintConv::uint256ToBytes(value)) {}

Hash::operator uint256_t() const {
  return UintConv::bytesToUint256(*this);
}

View<Hash>::operator uint256_t() const {
  return UintConv::bytesToUint256(*this);
}