#include <iostream>
#include <vector>

#include "core/subnet.h"
#include "core/transaction.h"

#include "net/p2p.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  subnet->p2p->connect("127.0.0.1", 8080);
  subnet->p2p->c_send("info");
  subnet->p2p->s_send("info");
  return 0;
}

