#include "common.h"
  
Address generateContractAddress(uint64_t nonce, View<Address> address) {
  uint8_t rlpSize = 0xc0;
  rlpSize += 20;
  rlpSize += (nonce < 0x80) ? 1 : 1 + Utils::bytesRequired(nonce);
  Bytes rlp;
  rlp.insert(rlp.end(), rlpSize);
  rlp.insert(rlp.end(), address.begin(), address.end());
  rlp.insert(rlp.end(), (nonce < 0x80) ? (char)nonce : (char)0x80 + Utils::bytesRequired(nonce));

  return Address(Utils::sha3(rlp).view(12));
}

Address generateContractAddress(View<Address> from, View<Hash> salt, View<Bytes> code) {
  const Hash codeHash = Utils::sha3(code);
  Bytes buffer(from.size() + salt.size() + codeHash.size() + 1); // 85
  buffer[0] = 0xFF;
  bytes::join(from, salt, codeHash).to(buffer | std::views::drop(1));
  return Address(Utils::sha3(buffer).view(12));
}
