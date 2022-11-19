#include "net/p2p.h"
#include <iostream>

#include "core/subnet.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  P2PMsg a("info");
  std::cout << a.dump() << std::endl;
  return 0;

  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

