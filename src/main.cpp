#include "main.h"

int main() {
  std::signal(SIGINT, SIG_IGN);
  RunServer();
  Utils::logToFile("returned");
  return 0;
}