#include <iostream>
#include <vector>

#include "core/subnet.h"
#include "core/transaction.h"

#include "net/p2p.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  std::shared_ptr<P2PListener> p2pl1;
  std::shared_ptr<P2PListener> p2pl2;
  std::shared_ptr<P2PClient> p2pc1;
  std::shared_ptr<P2PClient> p2pc2;
  p2pl1 = std::make_shared<P2PListener>(tcp::endpoint{
    net::ip::make_address("127.0.0.1"), 8080
  });
  p2pl2 = std::make_shared<P2PListener>(tcp::endpoint{
    net::ip::make_address("127.0.0.2"), 8080
  });
  p2pc1 = std::make_shared<P2PClient>();
  p2pc2 = std::make_shared<P2PClient>();
  p2pl1->start();
  p2pl2->start();
  p2pc1->resolve("127.0.0.2", "8080");
  p2pc2->resolve("127.0.0.1", "8080");

  while (true) {}

  return 0;

  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

