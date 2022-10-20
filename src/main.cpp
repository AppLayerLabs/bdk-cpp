#include <iostream>
#include <vector>

#include "core/subnet.h"
#include "core/transaction.h"

#include "net/p2p.h"

std::unique_ptr<Subnet> subnet;

// Let that good boi run
int main() {
  std::shared_ptr<P2PListener> p2pl;
  std::shared_ptr<P2PClient> p2pc;
  p2pl = std::make_shared<P2PListener>(tcp::endpoint{
    net::ip::make_address("127.0.0.1"), 8080
  });
  p2pc = std::make_shared<P2PClient>(P2PInfo(
    "0.0.1c", std::time(NULL), 10000, 94378248324932
  ));
  p2pl->start();
  p2pc->resolve("127.0.0.1", "8080");

  while (true) {}

  return 0;

  std::signal(SIGINT, SIG_IGN);
  //std::signal(SIGTERM, SIG_IGN);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

