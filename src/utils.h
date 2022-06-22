#ifndef UTILS_H
#define UTILS_H

#include <fstream>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/lexical_cast.hpp>
#include <include/web3cpp/devcore/CommonData.h>
#include "json.hpp"

using json = nlohmann::ordered_json;
using uint256_t = boost::multiprecision::uint256_t;
using bytes = std::vector<uint8_t>;

namespace Utils {
    void logToFile(std::string str);
    std::vector<uint8_t> uint256ToBytes(const uint256_t &i);
    std::vector<uint8_t> uint64ToBytes(const uint64_t &i);
    std::vector<uint8_t> uint32ToBytes(const uint32_t &i);
    uint256_t bytesToUint256(const std::vector<uint8_t> &bytes);
    uint64_t bytesToUint64(const std::vector<uint8_t> &bytes);
    uint32_t bytesToUint32(const std::vector<uint8_t> &bytes);
    // These functions are literal copies of input into output.
    // In usage to convert bytes into gRPC strings.
    std::string bytesToByteString(const std::vector<uint8_t> &bytes);
    std::vector<uint8_t> stringToBytes(const std::string &str);
}





#endif // UTILS_H