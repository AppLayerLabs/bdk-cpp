#include "common.h"

#ifdef BUILD_TESTNET
Address deprecatedGenerateContractAddress(uint64_t nonce, View<Address> address) {
  // TODO: Make forkable
  uint8_t rlpSize = 0xc0;
  rlpSize += 20;
  rlpSize += (nonce < 0x80) ? 1 : 1 + Utils::bytesRequired(nonce);
  Bytes rlp;
  rlp.insert(rlp.end(), rlpSize);
  rlp.insert(rlp.end(), address.cbegin(), address.cend());
  if (nonce < 0x80) {
    rlp.insert(rlp.end(), static_cast<char>(nonce));
  } else {
    rlp.insert(rlp.end(), 0x80 + Utils::bytesRequired(nonce));
    Utils::appendBytes(rlp, Utils::uintToBytes(nonce));
  }

  return Address(Utils::sha3(rlp).view(12));
}
#endif

Address generateContractAddress(uint64_t nonce, View<Address> address) {
  Bytes payload;
  payload.push_back(0x80 + 20);
  payload.insert(payload.end(), address.cbegin(), address.cend());

  // 1b) RLP‑encode the nonce
  if (nonce == 0) {
    // zero is encoded as empty string → 0x80
    payload.push_back(0x80);
  } else if (nonce < 0x80) {
    // single byte [0x00..0x7f]
    payload.push_back(static_cast<uint8_t>(nonce));
  } else {
    // longer nonce: prefix + raw bytes
    auto nb = Utils::uintToBytes(nonce);
    payload.push_back(0x80 + nb.size());
    Utils::appendBytes(payload, nb);
  }

  Bytes rlp;
  rlp.push_back(0xc0 + static_cast<uint8_t>(payload.size()));
  rlp.insert(rlp.end(), payload.begin(), payload.end());

  auto hash = Utils::sha3(rlp);
  return Address(hash.view(12));  // skip first 12 bytes, keep last 20
}


Address generateContractAddress(View<Address> from, View<Hash> salt, View<Bytes> code) {
  const Hash codeHash = Utils::sha3(code);
  Bytes buffer(from.size() + salt.size() + codeHash.size() + 1); // 85
  buffer[0] = 0xFF;
  bytes::join(from, salt, codeHash).to(buffer | std::views::drop(1));
  return Address(Utils::sha3(buffer).view(12));
}
