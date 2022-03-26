#include "utils.h"

std::mutex log_lock;

dev::bytes Utils::u256toBytes(dev::u256 value) {
    dev::bytes ret(sizeof(value));
    unsigned char bytes[sizeof(value)];


    std::memcpy(&bytes, &value, sizeof(value)); 

    for(uint64_t i = 0; i < sizeof(value); ++i) {
        ret[i] = bytes[i];
    }
    
    return ret;
};

void Utils::logToFile(std::string str) {
  log_lock.lock();
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
  log.close();
  log_lock.unlock();
}

dev::u256 Utils::bytesTou256(dev::bytes value) {
    dev::u256 ret;
    // 
    unsigned char bytes[sizeof(value)];

    for (uint64_t i = 0; i < sizeof(value); ++i) {
        bytes[i] = value[i];
    }

    std::memcpy(&ret, &bytes, sizeof(bytes));
    return ret;
}