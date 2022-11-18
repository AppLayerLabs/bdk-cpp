#include "core/subnet.h"

#include <iostream>
#include <vector>
#include "utils/utils.h"
#include "utils/merkle.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

