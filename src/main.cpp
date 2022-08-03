#include <iostream>
#include <vector>

#include "subnet.h"
#include "transaction.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

