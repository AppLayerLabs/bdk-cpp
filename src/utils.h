#ifndef UTILS_H
#define UTILS_H

#include <include/web3cpp/devcore/Common.h>

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <sstream>
#include <fstream>
#include <chrono>

namespace Utils {
    
   dev::bytes u256toBytes(dev::u256 value);
   dev::u256 bytesTou256(dev::bytes value);

   void logToFile(std::string str);

}

#endif // UTILS_H