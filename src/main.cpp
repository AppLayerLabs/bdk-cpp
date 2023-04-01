#include <iostream>
#include "utils/block.h"
#include "utils/strings.h"
#include "utils/utils.h"
#include "net/http/jsonrpc/methods.h"


// Dummy main.cpp file
int main() {
  PrivKey privKey(Hex::toBytes("ce974dad85cf9593db9d5c3e89ca8c67ca0f841dc97f2c58c6ea2038e4fa6d8d"));

  TxBlock tx(
    Address(std::string_view("0x13b5c424686de186bc5268d5cfe6aa4200ca9aee"), false),
    Address(std::string_view("0x31Af43C5E5924610a9c02B669c7980D9eBdB9719"), false),
    Hex::toBytes("0xe426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e"),
    uint64_t(8080),
    uint256_t("42968208492763873"),
    uint256_t("166903214424643"),
    uint256_t("769147246"),
    uint256_t("61182866117425671"),
    privKey
  );

  std::cout << Hex::fromBytes(tx.rlpSerialize()) << std::endl;
  return 0;
}