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
    unsigned char bytes[sizeof(value)];

    for (uint64_t i = 0; i < sizeof(value); ++i) {
        bytes[i] = value[i];
    }

    std::memcpy(&ret, &bytes, sizeof(bytes));
    return ret;
}

std::string Utils::secondsToGoTimeStamp(uint64_t seconds) {
    std::string ret;
    ret.resize(15);
    // First byte is version. 0x01
    ret[0] = 0x01;
    // Next 8 bytes is timestamp in seconds.   
    seconds += 62135596800; // Sum year 1 to 1970, stupid Go lol
    uint8_t timestamp[8];
    std::memcpy(&timestamp, &seconds, sizeof(seconds));
    ret[1] = timestamp[7];
    ret[2] = timestamp[6];
    ret[3] = timestamp[5];
    ret[4] = timestamp[4];
    ret[5] = timestamp[3];
    ret[6] = timestamp[2];
    ret[7] = timestamp[1];
    ret[8] = timestamp[0];
    // Remaining timestamp is nanoseconds, we ignore it for ease of implementation. I only have 18 hours to "finish" this
    ret[9] = 0x00;
    ret[10] = 0x00;
    ret[11] = 0x00;
    ret[12] = 0x00;
    ret[13] = 0x00;
    ret[14] = 0x00;

    return ret;
}

std::string Utils::hashToBytes(std::string hash) {
    std::string ret;
    ret.resize(32);
    uint64_t index = 0;

    for (int i = 0; i < 32; ++i) {
        // fkin C++ uint16_t and them get the first byte, completely pepega
        std::stringstream strm;
        std::string byteStr = std::string("") + hash[index] + hash[index+1];
        uint16_t byteInt;
        strm << std::hex << byteStr;
        strm >> byteInt;
        uint8_t byte = byteInt >> 0;
        ret[i] = byte;
        index += 2;
    }
    return ret;
}