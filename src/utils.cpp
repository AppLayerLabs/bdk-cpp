#include "utils.h"

std::mutex log_lock;

dev::bytes Utils::u256toBytes(dev::u256 value) {
    dev::bytes ret(sizeof(value));
    // u256 = 32 bytes;
    for (int i = 0; i < sizeof(value); ++i) {
        ret[(sizeof(value)-1) - i] = (unsigned char)(value >> (i * 8));
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
    if (value.size() != 48) {
        Utils::logToFile("Utils::bytesTou256: invalid size!");
        return ret;
    }

    for (uint i = 0; i < 48; ++i) {
        ret << (8 * i) = value[i];
    }
    return ret;
}