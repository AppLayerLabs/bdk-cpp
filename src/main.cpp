#include <iostream>
#include <vector>

#include "core/subnet.h"
#include "core/transaction.h"

#include "net/p2p.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  std::shared_ptr<P2PNode> p2p = std::make_shared<P2PNode>("127.0.0.1", 8080);
  p2p->connect("127.0.0.1", 8080);
  p2p->c_send("info");
  p2p->s_send("info");
  while (true) {}
  return 0;

  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

