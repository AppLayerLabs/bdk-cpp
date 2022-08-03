#include "tests.h"
#include "../src/db.h"
#include "../src/chainHead.h"
#include "../src/state.h"
#include "../src/block.h"
#include "../src/secp256k1Wrapper.h"

void Tests::testBlockchain() {
  // Start DB.
  auto dbServer = std::make_shared<DBService>("tests-db");

  // Start State

  auto headState = std::make_unique<State>(dbServer);
  auto chainHead = std::make_unique<ChainHead>(dbServer);

  // Test simply running the blockchain for 1000 blocks.
  Tests::doBlocks(1000, chainHead, headState);
  assert(chainHead->latest().nHeight() == 1000);

  // Create multiple accounts in order to do transactions inside the network.

  auto accounts = Tests::generateAddresses(1000);

  assert(accounts.size() == 1000);

  Tests::addBalance(accounts, headState);

};

std::vector<std::pair<Address,std::string>> Tests::generateAddresses(uint64_t quantity) {
  std::vector<std::pair<Address,std::string>> addresses;
  uint256_t start = 10000;
  for (uint64_t i = 0; i < quantity; ++i) {
    uint256_t r = start + i;
    std::string privKey = Utils::uint256ToBytes(r);
    std::string pubkey = Secp256k1::toPub(privKey);
    std::string pubkeyHash;
    Utils::sha3(pubkey, pubkeyHash);
    Address address(pubkeyHash.substr(12), false);  // Address = pubkeyHash[12...32]
    addresses.emplace_back(std::make_pair(address, privKey));
  }

  // Compare it with pregenerated address.
  assert(addresses[0].first.hex() == "1544920afdc2d6de7bbac245170789d498320498");
  std::cout << __func__ << " with " << quantity << " addresses OK" << std::endl;
  return addresses;
}

void Tests::addBalance(std::vector<std::pair<Address,std::string>> &accounts, std::unique_ptr<State> &state) {
  for (auto &item : accounts ) {
    state->addBalance(item.first);
  }
  // Check all added balances.
  for (auto &item : accounts) {
    assert(state->getNativeBalance(item.first) == uint256_t("1000000000000000000"));
  }
  std::cout << __func__ << " with " << accounts.size() << " addresses OK" << std::endl;
}