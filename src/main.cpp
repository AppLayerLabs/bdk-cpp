#include "main.h"


int main() {
  dev::u256 i = dev::u256(100);

  std::cout << "i: " << i << std::endl;

  auto iBytes = Utils::u256toBytes(i);

  for (auto c : iBytes) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << c;
  }
  std::cout << std::endl;
  dev::u256 ii = Utils::bytesTou256(iBytes);

  std::cout << "ii: " << ii << std::endl;
  if (i == ii) {
    std::cout << "Equal! nice" << std::endl;
  }

  return 0;
  RunServer();

  return 0;
}