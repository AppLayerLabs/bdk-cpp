#include <iostream>
#include "utils/tx.h"
// Dummy main.cpp file
int main() {

  TxBlock tx(Hex::toBytes("0xf8ae82d3548505d21dba008305cf809418df1967e5cc30ee53d399a8bbf71c3e60b44beb80b8430d079f8876b0de07c78a02254986f3473fabbb6b4aca5396627ec5c60480d05fa135405e021373121c55ca5bc2a2490000000000000000000000000000000000000000830150f7a0566e1e6e301e72698e948fee5ca0cd32eab301d66ba2fb4496809fb8cb5b3663a039ef7219cb5c105024f0f586d468f9d41d4ce431c4d3cb6824738ff50a9a0b32"));



  std::cout << "nonce: " << tx.getNonce() << std::endl;
  std::cout << "gasprice: " << tx.getGasPrice() << std::endl;
  std::cout << "gas: " << tx.getGas() << std::endl;
  std::cout << "to: " << tx.getTo().hex() << std::endl;
  std::cout << "Value: " << tx.getValue() << std::endl;
  std::cout << "data: " << Hex::fromBytes(tx.getData()) << std::endl;
  std::cout << "from: " << tx.getFrom().hex() << std::endl;
  std::cout << "r: " << tx.getR() << std::endl;
  std::cout << "s: " << tx.getS() << std::endl;
  std::cout << "v: " << tx.getV() << std::endl;
  return 0;
}