// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include "RLP.h"
#include "SHA3.h"

std::string uint64ToBytes(const uint64_t &i) {
  std::string ret(8, 0x00);
  ret[0] = i >> 56;
  ret[1] = i >> 48;
  ret[2] = i >> 40;
  ret[3] = i >> 32;
  ret[4] = i >> 24;
  ret[5] = i >> 16;
  ret[6] = i >> 8;
  ret[7] = i;
  return ret;
}

namespace dev {
  h256 const EmptySHA3 = sha3(bytesConstRef());
  h256 const EmptyListSHA3 = sha3(rlpList());
  bool sha3(bytesConstRef _input, bytesRef o_output) noexcept {
    if (o_output.size() != 32) return false;
    ethash::hash256 h = ethash::keccak256(_input.data(), _input.size());
    bytesConstRef{h.bytes, 32}.copyTo(o_output);
    return true;
  }
};

