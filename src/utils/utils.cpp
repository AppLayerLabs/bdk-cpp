/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "utils.h"

#include "dynamicexception.h"
#include "uintconv.h"

#include <openssl/sha.h>

std::mutex log_lock;
std::mutex debug_mutex;

void fail(const std::string& cl, std::string&& func, boost::beast::error_code ec, const char* what) {
  Logger::logToDebug(LogType::ERROR, cl, std::move(func), std::string("HTTP Fail ") + what + " : " + ec.message());
}

std::string Utils::getTestDumpPath() { return std::string("testdump"); }

void Utils::logToFile(std::string_view str) {
  // Lock to prevent multiple memory writes
  std::lock_guard lock(log_lock);
  std::ofstream log("log.txt", std::ios::app);
  log << str << std::endl;
  log.close();
}

Functor Utils::makeFunctor(std::string_view functionSignature) {
  Functor ret;
  // Create the hash
  Hash hash = Utils::sha3(View<Bytes>(reinterpret_cast<const uint8_t*>(functionSignature.data()), functionSignature.size()));
  // Copy the first 4 bytes of the hash to the value
  ret.value = UintConv::bytesToUint32(hash.view(0,4));
  return ret;
}

void Utils::safePrint(std::string_view str) {
  Log::safePrint(str);
}

void Utils::safePrintTest(std::string_view str) {
  Log::safePrintTest(str);
}

Hash Utils::sha3(const View<Bytes> input) {
  ethash_hash256 h = ethash_keccak256(input.data(), input.size());
  Hash ret;
  std::copy(reinterpret_cast<const Byte*>(h.bytes), reinterpret_cast<const Byte*>(h.bytes + 32), ret.begin());
  return ret;
}

Account::Account(const View<Bytes>& bytes) {
  if (bytes.size() < 73) throw DynamicException(std::string(__func__) + ": Invalid bytes size");
  this->balance = UintConv::bytesToUint256(bytes.subspan(0,32));
  this->nonce = UintConv::bytesToUint64(bytes.subspan(32,8));
  this->codeHash = Hash(bytes.subspan(40,32));
  if (bytes[72] > 2) throw DynamicException(std::string(__func__) + ": Invalid contract type");
  this->contractType = ContractType(bytes[72]);
  if (bytes.size() > 73) this->code = Bytes(bytes.begin() + 73, bytes.end());
}

Bytes Account::serialize() const {
  return Utils::makeBytes(bytes::join(
    UintConv::uint256ToBytes(this->balance),
    UintConv::uint64ToBytes(this->nonce),
    this->codeHash,
    UintConv::uint8ToBytes(char(this->contractType)),
    this->code
  ));
}

Bytes Utils::randBytes(const int& size) {
  Bytes bytes(size, 0x00);
  RAND_bytes(bytes.data(), size);
  return bytes;
}

json Utils::readConfigFile() {
  if (!std::filesystem::exists("config.json")) {
    SLOGINFO("No config file found, generating default");
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

void Utils::sha256(const Bytes& input, Hash& output) {
  SHA256(input.data(), input.size(), output.data());
}

Hash Utils::sha256(const Bytes& input) {
  Hash hash;
  SHA256(input.data(), input.size(), hash.data());
  return hash;
}

std::string Utils::getSignalName(int signum) {
  std::string n;
  if (signum == SIGINT) n = "SIGINT";
  else if (signum == SIGHUP) n = "SIGHUP";
  else n = "Unknown signal";
  n += " (" + std::to_string(signum) + ")";
  return n;
}

View<Bytes> Utils::create_view_span(const Bytes& vec) {
  return View<Bytes>(vec.data(), vec.size());
}

View<Bytes> Utils::create_view_span(const Bytes& vec, size_t start, size_t size) {
  if (start + size > vec.size()) throw DynamicException("Invalid range for span");
  return View<Bytes>(vec.data() + start, size);
}

View<Bytes> Utils::create_view_span(const std::string_view str) {
  return View<Bytes>(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

View<Bytes> Utils::create_view_span(const std::string_view str, size_t start, size_t size) {
  if (start + size > str.size()) throw DynamicException("Invalid range for span");
  return View<Bytes>(reinterpret_cast<const uint8_t*>(str.data()) + start, size);
}

uint64_t Utils::stringToNanos(const std::string& timestamp) {
  // Converts an ISO 8601 timestamp string (with nanosecond precision) into
  // nanoseconds since the epoch.
  std::string timeStr = timestamp;
  if (!timeStr.empty() && timeStr.back() == 'Z') {
    timeStr.pop_back(); // Remove trailing 'Z'
  }
  // Split the string into the part without fraction and the fractional part.
  auto dotPos = timeStr.find('.');
  std::string timeWithoutFraction = (dotPos == std::string::npos)
    ? timeStr : timeStr.substr(0, dotPos);
  std::string fraction = (dotPos == std::string::npos)
    ? "0" : timeStr.substr(dotPos + 1);
  // Parse the non-fractional date/time.
  std::tm tm = {};
  std::istringstream ss(timeWithoutFraction);
  ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
  if (ss.fail()) {
    throw DynamicException("Failed to parse time without fraction: " + timeWithoutFraction);
  }
  // Convert to seconds since epoch.
  std::time_t seconds = timegm(&tm);  // timegm interprets tm as UTC
  uint64_t nanosecondsSinceEpoch = static_cast<uint64_t>(seconds) * 1000000000ULL;
  // Process the fractional part:
  // Pad the fractional part to 9 digits (nanosecond resolution)
  if (fraction.size() > 9)
    fraction = fraction.substr(0, 9);
  else if (fraction.size() < 9)
    fraction.append(9 - fraction.size(), '0');
  nanosecondsSinceEpoch += std::stoull(fraction);
  return nanosecondsSinceEpoch;
}

std::string Utils::nanosToString(uint64_t nanosecondsSinceEpoch) {
  // Converts nanoseconds since epoch to an ISO 8601 timestamp string
  // with nanosecond precision.
  // Extract whole seconds and the remaining nanoseconds.
  std::time_t seconds = nanosecondsSinceEpoch / 1000000000ULL;
  uint64_t ns = nanosecondsSinceEpoch % 1000000000ULL;
  // Convert seconds to UTC time.
  std::tm* utc_tm = std::gmtime(&seconds);
  if (!utc_tm) {
    throw DynamicException("gmtime failed");
  }
  // Format the date and time.
  std::ostringstream oss;
  oss << std::put_time(utc_tm, "%Y-%m-%dT%H:%M:%S");
  // Append the fractional part with 9 digits.
  oss << "." << std::setw(9) << std::setfill('0') << ns << "Z";
  return oss.str();
}
