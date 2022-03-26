#include "utils.h"


dev::bytes Utils::u256toBytes(dev::u256 value) {
    dev::bytes ret;
    // u256 = 32 bytes;
    for (int i = 0, i < 32; ++i) {
        ret.push_back((value >> (i * 8)));
    }
    return ret;
};