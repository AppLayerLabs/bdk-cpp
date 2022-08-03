#include <iostream>
#include <vector>

#include "subnet.h"
#include "transaction.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {

  uint256_t keyI = uint256_t("172381824912738179852131");
  std::string privKey = Utils::uint256ToBytes(keyI);

  std::string pubkey = Secp256k1::toPub(privKey);
  std::string pubkeyHash;
  Utils::sha3(pubkey, pubkeyHash);
  Address address(pubkeyHash.substr(12), false);


// Base(Address &from, Address &to, uint256_t &value, std::string &data, uint64_t &chainId, uint256_t &nonce, uint256_t &gas, uint256_t &gasPrice) 

  Address to("0x1544920afDc2D6de7BbAc245170789D498320498", true);
  uint256_t value("1000000000000000000");
  std::string data = "";
  uint64_t chainId = 8848;
  uint256_t nonce = 0;
  uint256_t gas = 21000;
  uint256_t gasPrice("5000000000");
  Tx::Base transaction(
    address,
    to,
    value,
    data,
    chainId,
    nonce,
    gas,
    gasPrice
  );


  transaction.sign(privKey);


  std::cout << Utils::bytesToHex(transaction.rlpSerialize(true)) << std::endl;
  return 0;


  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

