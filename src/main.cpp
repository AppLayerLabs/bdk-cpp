#include "core/subnet.h"

#include <iostream>
#include <vector>
#include "utils/utils.h"
#include "utils/merkle.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  //Patricia p;
  //Hash h = Utils::hexToBytes(
  //  "0x3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d"
  //);
  //p.add(h, "Transaction data here");
  //std::cout << p.get(h) << std::endl;
  //std::cout << p.remove(h) << std::endl;
  //std::cout << p.get(h) << std::endl;
  //return 0;
//
//
  //std::vector<Hash> hashList {
  //  Utils::hexToBytes("0x3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d"),
  //  Utils::hexToBytes("0x3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d"),
  //  Utils::hexToBytes("0x2e7d2c03a9507ae265ecf5b5356885a53393a2029d241394997265a1a25aefc6"),
  //  Utils::hexToBytes("0x3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d"),
  //  Utils::hexToBytes("0x3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d"),
  //  Utils::hexToBytes("0x2e7d2c03a9507ae265ecf5b5356885a53393a2029d241394997265a1a25aefc6"),
  //  Utils::hexToBytes("0x3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d"),
  //  Utils::hexToBytes("0x3e23e8160039594a33894f6564e1b1348bbd7a0088d42c4acb73eeaed59c009d"),
  //  Utils::hexToBytes("0x2e7d2c03a9507ae265ecf5b5356885a53393a2029d241394997265a1a25aefc6")
  //};
  //Merkle merkleRoot(hashList);
  //for (const auto &layer : merkleRoot.layers()) {
  //  for (const auto &hash : layer) {
  //    std::cout << hash.hex() << std::endl;
  //  }
  //  std::cout << std::endl;
  //}
  //std::cout << std::endl << std::endl << std::endl;
  //std::vector<Hash> res = merkleRoot.getProof(2);
  //for (Hash h : res) std::cout << h.hex() << std::endl;
  //return 0;

  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

