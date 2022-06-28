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

namespace Log {
    const std::string subnet = "Subnet::";
    const std::string chainHead = "ChainHead::";
    const std::string block = "Block::";
    const std::string dbService = "DBService::";
    const std::string state = "State::";
    const std::string grpcServer = "VMServiceImplementation::";
    const std::string grpcClient = "VMCommClient::";
    const std::string utils = "Utils::";
};

namespace Utils {
    void logToFile(std::string str);
    void LogPrint(std::string prefix, std::string function, std::string data);
    std::string uint256ToBytes(const uint256_t &i);
    std::string uint64ToBytes(const uint64_t &i);
    std::string uint32ToBytes(const uint32_t &i);
    uint256_t bytesToUint256(const std::string &bytes);
    uint64_t bytesToUint64(const std::string &bytes);
    uint32_t bytesToUint32(const std::string &bytes);
    // These functions are literal copies of input into output.
    // In usage to convert bytes into gRPC strings.
    std::string bytesToByteString(const std::string &bytes);
    std::string stringToBytes(const std::string &str);
}





#endif // UTILS_H