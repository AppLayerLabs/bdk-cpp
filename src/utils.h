#ifndef UTILS_H
#define UTILS_H

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/lexical_cast.hpp>
#include <include/web3cpp/devcore/CommonData.h>
#include "json.hpp"



using json = nlohmann::ordered_json;
using uint256_t = boost::multiprecision::uint256_t;

namespace Utils {
    std::vector<uint8_t> uint256ToBytes(const uint256_t &i);
    std::vector<uint8_t> uint64ToBytes(const uint64_t &i);
    std::vector<uint8_t> uint32ToBytes(const uint32_t &i);
    uint256_t bytesToUint256(const std::vector<uint8_t> &bytes);
    uint64_t bytesToUint64(const std::vector<uint8_t> &bytes);
    uint32_t bytesToUint32(const std::vector<uint8_t> &bytes);
}





#endif // UTILS_H