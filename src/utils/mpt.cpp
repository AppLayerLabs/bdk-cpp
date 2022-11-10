#include "mpt.h"

MPT::MPT(std::vector<std::string> list) {
  // Create leaf nodes
  for (int i = 0; i < list.size(); i++) {
    this->merkle.push_back(Utils::sha3(list[i]).hex());
  }

  // Create the tree from the bottom up to the root
  unsigned int nextLayerCt = this->merkle.size();
  while (nextLayerCt > 1) {
    std::vector<std::string> layer;
    for (int i = 0; i < nextLayerCt; i += 2) {
      if (nextLayerCt % 2 != 0 && (i + 1) == nextLayerCt) { // Odd leaf number
        layer.push_back(this->merkle[i]);
      } else {  // Even leaf number
        layer.push_back(Utils::sha3(this->merkle[i] + this->merkle[i+1]).hex());
      }
    }
    nextLayerCt = layer.size();
    this->merkle.insert(this->merkle.begin(), layer.begin(), layer.end());
  }
}

bool MPT::verify(std::string data) {
  std::string hashedData = Utils::sha3(data).hex();
  std::vector<std::string>::iterator it;
  it = std::find(this->merkle.begin(), this->merkle.end(), hashedData);
  if (it == this->merkle.end()) return false;

  std::string side, hash;
  unsigned int pos = it - this->merkle.begin();
  while (pos > 0) {
    if (pos % 2 == 0) { // Right child
      hash = Utils::sha3(this->merkle[pos-1] + this->merkle[pos]).hex();
      pos = floor((pos - 1) / 2);
    } else {  // Left child
      hash = Utils::sha3(this->merkle[pos] + this->merkle[pos+1]).hex();
      pos = floor(pos / 2);
    }
  }
  return (hash == this->merkle[0]);
}

