#include "tests.h"
#include "../src/utils.h"
#include "../src/transaction.h"

void Tests::transactions() {

  std::string txBytes = Utils::hexToBytes("0xf86e8085012a05f20082520894da3ef932f6c1dc41055f7c196c416be5c3c34fe788016345785d8a000080824544a00177cae5d48dcc8750f0647d18797b083ab5ab8ac1ea6e6c5c5ebdc3692533eea008574e6559f910a452af89db94a336d2c402bd27a03404c7f1e2ca5172fb4dec");

  try {
    Tx::Base tx(txBytes, false);

    assert(tx.nonce() == 0);
    assert(tx.gasPrice == uint256_t("5000000000"));
    assert(tx.gas() == uint256_t("21000"));
    assert(tx.to().hex() == "0xda3ef932f6c1dc41055f7c196c416be5c3c34fe7");
    assert(tx.value() == uint256_t("1000000000000000000"));
    assert(tx.from().hex() == "0x0c43aa7b1abc9355f4c6ad1c6c0881bc28f765f1");
    assert(tx.data() == std::string(""));
    assert(tx.r() == uint256_t("663967998266271129109389889062598836125705429943091786001113539763235009518"));
    assert(tx.v() == uint256_t("17732"));
    assert(tx.s() == uint256_t("3772759551946766949526054174207937981965374818020153740855208465901463227884"));


  } catch (std::exception &e) {
    std::cout << __func__ << " FAILED " << std::endl;
    std::cout << e.what() << std::endl;
    throw "";
  }

  std::cout << __func__ << " OK" << std::endl;

}