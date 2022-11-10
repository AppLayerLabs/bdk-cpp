#include "core/subnet.h"

#include <iostream>
#include <vector>
#include "utils/mpt.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  std::vector<std::string> v = {"a", "b", "c", "d", "e"};
  std::vector<std::string> v2 = {"a", "b", "c", "d", "e"};
  MPT mpt(v);
  MPT mpt2(v2);
  std::cout << mpt.verify("a") << std::endl;
  std::cout << mpt2.verify("a") << std::endl;
  return 0;

  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

