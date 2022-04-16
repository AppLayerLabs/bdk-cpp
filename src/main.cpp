#include "main.h"

int main() {
  std::signal(SIGINT, SIG_IGN);
  Subnet subnet;
  subnet.start();
  Utils::logToFile("returned");
  return 0;
}