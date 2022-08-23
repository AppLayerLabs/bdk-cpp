#ifndef ABI_H
#define ABI_H

#include <string>
#include "../core/utils.h"

#include "../libs/json.hpp"

enum ABITypes {
  uint256, uint256Arr,
  address, addressArr,
  boolean, booleanArr,
  bytes, bytesArr,
  string, stringArr
};

// ABI Encoder.
class ABI {
  private:
    // TODO
  public:
    // TODO
};

class ABIDecoder {
  private:
    // All currently compatible solidity types:
    // uint256_t, uint256_t[]
    // address, address[]
    // bool, bool[]
    // bytes, bytes[]
    // string, string[]
    // Both bytes and string are stored as std::string.


    std::vector<std::variant<uint256_t, std::vector<uint256_t>,
                  Address, std::vector<Address>,
                  bool, std::vector<bool>,
                  std::string, std::vector<std::string>>> data;


  public:

  // Data as *bytes*
  ABIDecoder(std::vector<ABITypes> const &types, std::string const &abiData);


  template <typename T>
  T get(uint64_t const &index) {
    if (index >= data.size()) {
      throw std::out_of_range("Index out of range");
    }
    if (std::holds_alternative<T>(data[index])) {
      return std::get<T>(data[index]);
    }
    throw std::runtime_error("Type mismatch");
  }
  
  size_t size() { return data.size(); }

};

#endif  // ABI_H
