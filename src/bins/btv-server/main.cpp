#include "src/manager.h"


std::unique_ptr<BTVServer::Manager> manager = nullptr;

void signalHandler(int signum) {
  BTVServer::Printer::safePrint("Signal caught: " + Utils::getSignalName(signum));
  manager.reset();
}

int main() {
  std::signal(SIGINT, signalHandler);
  std::signal(SIGHUP, signalHandler);
  Log::logToCout = true;
  manager = std::make_unique<BTVServer::Manager>();
  BTVServer::Printer::safePrint("Starting Build The Void Websocket Server...");
  manager->start();
  BTVServer::Printer::safePrint("Exitting Build The Void Websocket Server...");


  return 0;
}