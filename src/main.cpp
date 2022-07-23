#include <iostream>
#include <vector>

#include "subnet.h"

std::unique_ptr<Subnet> subnet;

/**
 * As of July 5th 2022, the AvalancheGo daemon simply kills the subnet,
 * not letting it know through the grpcserver or any other means...
 * This stop function is a workaround to allow our subnet to be stopped
 * without the need of AvalancheGo letting it know.
 */
void shutdown_handler(int sig) { subnet->stop(); }

// Let that good boi run
int main() {
  std::signal(SIGINT, SIG_IGN);
  std::signal(SIGTERM, shutdown_handler);
  subnet = std::make_unique<Subnet>();
  subnet->start();
  return 0;
}

