#include "utils.h"
#include "hex.h"
#include "strings.h"

std::mutex log_lock;
std::mutex debug_mutex;

void fail(std::string_view cl, std::string_view func, boost::beast::error_code ec, const char* what) {
  Utils::logToDebug(cl, func, std::string("P2P Fail ") + what + " : " + ec.message());
}

void Utils::logToFile(std::string_view str)
{
  /// Lock to prevent multiple memory write
  log_lock.lock();
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
  log.close();
  log_lock.unlock();
}

void Utils::logToDebug(std::string_view pfx, std::string_view func, std::string_view data)
{
  debug_mutex.lock();
  std::ofstream log("debug.txt", std::ios::app);
  log << pfx << func << " - " << data << std::endl;
  log.close();
  debug_mutex.unlock();
}

Hash Utils::sha3(const std::string_view& input)
{
  std::string ret;
  ethash_hash256 h = ethash_keccak256(reinterpret_cast<const unsigned char*>(&input[0]), input.size());
  for(unsigned char byte : h.bytes) {
    ///Unsigned char to char warning?
    ret.push_back(byte);
  }
  Hash retH(std::move(ret));
  return retH;
}

// Deprecated
//  bool Utils::isHex(const std::string_view input, bool strict)
//  {
//    uint16_t i = (strict) ? 2 : 0;
//    if (strict && input.substr(0, 2) != "0x" && input.substr(0, 2) != "0X")
//    {
//      return false;
//    }
//    std::string_view temp(&input[i], input.size() - i);
//    return temp.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
//  }

//  Deprecated
//  std::string Utils::utf8ToHex(const std::string_view &str) {
//    std::stringstream ss;
//    for (char i : str) {
//      // You need two casts in order to properly cast char to uint.
//      ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint>(static_cast<uint8_t>(i));
//    }
//    return ss.str();
//  }

// Deprecated
// std::string Utils::bytesToHex(const std::string_view& b)
// {
//   auto _it = b.begin();
//   auto _end = b.end();
// 	static char const* hexdigits = "0123456789abcdef";
// 	size_t off = 0;
// 	std::string hex(std::distance(_it, _end)*2, '0');
// 	for (; _it != _end; _it++)
// 	{
// 		hex[off++] = hexdigits[(*_it >> 4) & 0x0f];
// 		hex[off++] = hexdigits[*_it & 0x0f];
// 	}
// 	return hex;
// }

// std::string Utils::hexToBytes(std::string_view content) {
//   std::string hex = patchHex(content.data());
//   std::string ret;
//   uint32_t index = 0;
// 
//   if (hex.size() % 2 != 0) {
//     int byteHex = hexCharToInt(hex[index]);
//     if (byteHex != -1) {
//       ret += (char) uint8_t(byteHex);
//     } else {
//       Utils::logToDebug(Log::utils ,__func__, "Invalid Hex");
//       throw std::runtime_error(std::string(__func__) + ": " +
//                                std::string("Invalid hex char: ") + hex[index]
//       );
//     }
//     index++;
//   }
// 
//   // Parse two by two chars until the end
//   while (index < hex.size()) {
//     int h = hexCharToInt(hex[index]);
//     int l = hexCharToInt(hex[index+1]);
//     if (h != -1 && l != -1) {
//       ret += (char) uint8_t(h * 16 + l);
//     } else {
//       throw std::runtime_error(
//               std::string(__func__) + ": " +
//               std::string("One or more invalid hex chars: ") +
//               hex[index] + hex[index + 1]
//       );
//     }
//     index += 2;
//   }
//   return ret;
// }

// bool Utils::verifySig(uint256_t const &r, uint256_t const &s, uint8_t const &v) {
//   // s_max = 0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141
//   static const uint256_t s_max("115792089237316195423570985008687907852837564279074904382605163141518161494337");
//   static const uint256_t s_zero = 0;
//   return (v <= 1 && r > s_zero && s > s_zero && r < s_max && s < s_max);
// }

// TODO: This function is identical in CommonData.h, is for the better a re-write of commonly used functions at CommonData.h

int Utils::hexCharToInt(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')	return c - 'A' + 10;
  return -1;
}

// std::string Utils::patchHex(const std::string& str)
// {
//   std::string ret;
//   uint64_t index = 0;
//   if (str[0] == '0' && str[1] == 'x') index = 2;
//   for (;index < str.size(); ++index) {
//     if (std::isupper(str[index])) {
//       ret += std::tolower(str[index]);
//     } else {
//       ret += str[index];
//     }
//   }
//   return ret;
// }

std::string Utils::uint256ToBytes(const uint256_t& integer)
{
  std::string ret(32, 0x00);
  std::string tmp;
  boost::multiprecision::export_bits(integer, std::back_inserter(tmp), 8);
  for (unsigned i = 0; i < tmp.size(); ++i) {
    ret[31-i] = tmp[tmp.size()-i-1];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  return ret;
}

uint256_t Utils::bytesToUint256(const std::string_view &b)
{
  if (b.size() != 32) {
    throw std::runtime_error(
            std::string(__func__) + ": Invalid bytes size - expected 32, got " + std::to_string(b.size())
    );
  }
  uint256_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

std::string Utils::uint160ToBytes(const uint160_t& integer)
{
  std::string ret(20, 0x00);
  std::string tmp;
  boost::multiprecision::export_bits(integer, std::back_inserter(tmp), 8);
  for (unsigned i = 0; i < tmp.size(); ++i) {
    ret[19-i] = tmp[tmp.size()-i-1];  // Replace bytes from tmp to ret to make it 32 bytes in size.
  }
  return ret;
}

uint160_t Utils::bytesToUint160(const std::string_view& b)
{
  if (b.size() != 20) {
    throw std::runtime_error(std::string(__func__) + ": " +
                             std::string("Invalid bytes size - expected 20, got ") + std::to_string(b.size())
    );
  }
  uint160_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

std::string Utils::uint64ToBytes(const uint64_t& integer)
{
  std::string ret(8, 0x00);
  std::memcpy(&ret[0], &integer, 8);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  std::reverse(ret.begin(), ret.end());
#endif
  return ret;
}

uint64_t Utils::bytesToUint64(const std::string_view& b)
{
  if (b.size() != 8) {
    throw std::runtime_error(std::string(__func__) + ": " +
                             std::string("Invalid bytes size - expected 8, got ") + std::to_string(b.size())
    );
  }
  uint64_t ret = 0;
  std::memcpy(&ret, b.data(), 8);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return __builtin_bswap64(ret);
#endif
  return ret;
}

std::string Utils::uint32ToBytes(const uint32_t& integer)
{
  std::string ret(4, 0x00);
  std::memcpy(&ret[0], &integer, 4);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  std::reverse(ret.begin(), ret.end());
#endif
  return ret;
}

uint32_t Utils::bytesToUint32(const std::string_view& b)
{
  if (b.size() != 4) {
    throw std::runtime_error(std::string(__func__) + ": " +
                             std::string("Invalid bytes size - expected 4, got ") + std::to_string(b.size())
    );
  }
  uint32_t ret = 0;
  std::memcpy(&ret, b.data(), 4);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return __builtin_bswap32(ret);
#endif
  return ret;
}

std::string Utils::uint16ToBytes(const uint16_t& i)
{
  std::string ret(2, 0x00);
  std::memcpy(&ret[0], &i, 2);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  std::reverse(ret.begin(), ret.end());
#endif
  return ret;
}

uint16_t Utils::bytesToUint16(const std::string_view &b)
{
  if (b.size() != 2) {
    throw std::runtime_error(std::string(__func__) + ": " +
                             std::string("Invalid bytes size - expected 2, got ") + std::to_string(b.size())
    );
  }
  uint16_t ret = 0;
  std::memcpy(&ret, b.data(), 2);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return __builtin_bswap16(ret);
#endif
  return ret;
}

std::string Utils::uint8ToBytes(const uint8_t& i)
{
  std::string ret(1, 0x00);
  std::memcpy(&ret[0], &i, 1);
  return ret;
}

uint8_t Utils::bytesToUint8(const std::string_view& b)
{
  if (b.size() != 1) {
    throw std::runtime_error(std::string(__func__) + ": " +
                             std::string("Invalid bytes size - expected 1, got ") + std::to_string(b.size())
    );
  }
  uint8_t ret;
  ret = b[0];
  return ret;
}

std::string Utils::randBytes(const int& size)
{
  std::string bytes(size, 0x00);
  RAND_bytes((unsigned char*)bytes.data(), size);
  return bytes;
}

std::string Utils::padLeft(std::string str, unsigned int charAmount, char sign)
{
  bool hasPrefix = (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X");
  if (hasPrefix) { str = str.substr(2); }
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + padded + str;
}

std::string Utils::padRight(std::string str, unsigned int charAmount, char sign)
{
  bool hasPrefix = (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X");
  if (hasPrefix) { str = str.substr(2); }
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + str + padded;
}


void Utils::toLower(std::string& str)
{
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void Utils::toUpper(std::string& str)
{
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

// void Utils::stripHexPrefix(std::string& str)
// {
//   if (str[0] == '0' && str[1] == 'x') str = str.substr(2);
// }

// uint256_t Utils::hexToUint(std::string &hex) {
//   patchHex(hex);
//   return boost::lexical_cast<HexTo<uint256_t>>(hex);
// }

void Utils::toChksum(std::string& str) {
// Hash requires lowercase address without "0x"
  if (str.substr(0, 2) == "0x" || str.substr(0, 2) == "0X") {
    str = str.substr(2);
  }
  Utils::toLower(str);
  Hex hash = Utils::sha3(str).hex();
  for (int i = 0; i < str.length(); i++) {
    if (!std::isdigit(str[i])) {  // Only check letters (A-F)
      // If character hash is 8-F then make it uppercase
      int nibble = std::stoi(hash.substr(i, 1), nullptr, 16);
      str[i] = (nibble >= 8) ? std::toupper(str[i]) : std::tolower(str[i]);
    }
  }
  str.insert(0, "0x");
}

bool Utils::isChksum(const std::string& str) {
  std::string addCpy = str;
  Utils::toChksum(addCpy);
  return (str == addCpy);
}

bool Utils::isAddress(const std::string& add, bool fromRPC) {
  if(fromRPC)
  {
    if (add[0] == '0' && (add[1] == 'x' || add[1] == 'X')) {
      if(add.size () != 42) { return false; }
      if(add.substr(2).find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) { return false; }
      return true;
    } else {
      if (add.size() != 40) { return false; }
      if (add.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos) { return false; }
      return true;
    }
  }
  return (add.size() == 20);
}

json Utils::readConfigFile() {
  if (!std::filesystem::exists("config.json")) {
    Utils::logToDebug(Log::utils, __func__, "No config file found, generating default");
    json config;
    config["rpcport"] = 8080;
    config["p2pport"] = 8081;
    config["seedNodes"] = {
            "127.0.0.1:8086", "127.0.0.1:8087", "127.0.0.1:8088", "127.0.0.1:8089"
    };
    std::ofstream configFile("config.json");
    configFile << config.dump(2);
    configFile.close();
  }
  std::ifstream configFile("config.json");
  json config = json::parse(configFile);
  return config;
}