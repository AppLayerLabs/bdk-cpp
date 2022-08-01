#include <iostream>
#include <vector>

#include "subnet.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {

  std::string myHex = "0xaaaaaa";

  std::string myBytes = Utils::hexToBytes(myHex);


  std::cout << dev::toHex(myBytes) << std::endl;;


  std::cout << "Trying 7..." << std::endl;;


  std::string myHex2  = "0x0aaaaaaa";

  std::string myBytes2 = Utils::hexToBytes(myHex2);

  std::cout << dev::toHex(myBytes2) << std::endl;


  return 0;
  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

