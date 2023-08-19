#include <iostream>
#include "contract/abi.h"
#include "src/core/blockchain.h"
#include <filesystem>
#include "src/contract/abi.h"
#include "src/utils/utils.h"
#include "utils/utils.h"

int main() {

  ABI::Encoder::EncVar vars;
  int8_t a = -10;
  vars.push_back(static_cast<int256_t>(a));
  auto result = ABI::Encoder(vars).getData();
  std::cout << Hex::fromBytes(result).get() << std::endl;
  auto decodeResult = ABI::Decoder({ABI::Types::int8}, result).getData<int256_t>(0);
  std::cout << decodeResult << std::endl;
  return 0;
  
  
}