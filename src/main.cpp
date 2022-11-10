#include "core/subnet.h"

#include <iostream>
#include <vector>
#include "utils/mpt.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  std::vector<std::string> v = {"a", "b", "c", "d", "e", "f", "g", "h"};
  MPT mpt(v);
  std::cout << mpt.verify("a") << std::endl;
  std::cout << mpt.verify("b") << std::endl;
  std::cout << mpt.verify("c") << std::endl;
  std::cout << mpt.verify("d") << std::endl;
  std::cout << mpt.verify("e") << std::endl;
  std::cout << mpt.verify("f") << std::endl;
  std::cout << mpt.verify("g") << std::endl;
  std::cout << mpt.verify("h") << std::endl;
  std::cout << mpt.verify("1") << std::endl;
  std::cout << mpt.verify("2") << std::endl;
  std::cout << mpt.verify("3") << std::endl;
  std::cout << mpt.verify("4") << std::endl;
  std::cout << mpt.verify("5") << std::endl;
  std::cout << mpt.verify("6") << std::endl;
  std::cout << mpt.verify("7") << std::endl;
  std::cout << mpt.verify("8") << std::endl;
  return 0;

  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

