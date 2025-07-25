/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "utils.h"

#include "dynamicexception.h"
#include "uintconv.h"

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
  // TODO: this could be optimized with bytes::join()?
  Bytes ret;
  Utils::appendBytes(ret, UintConv::uint256ToBytes(this->balance));
  Utils::appendBytes(ret, UintConv::uint64ToBytes(this->nonce));
  Utils::appendBytes(ret, this->codeHash);
  ret.insert(ret.end(), char(this->contractType));
  Utils::appendBytes(ret, this->code);
  return ret;
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

std::string Utils::getSignalName(int signum) {
  std::string n;
  if (signum == SIGINT) n = "SIGINT";
  else if (signum == SIGHUP) n = "SIGHUP";
  else n = "Unknown signal";
  n += " (" + std::to_string(signum) + ")";
  return n;
}

View<Bytes> Utils::create_view_span(const Bytes& vec, size_t start, size_t size) {
  if (start + size > vec.size()) throw DynamicException("Invalid range for span");
  return View<Bytes>(vec.data() + start, size);
}

View<Bytes> Utils::create_view_span(const std::string_view str, size_t start, size_t size) {
  if (start + size > str.size()) throw DynamicException("Invalid range for span");
  return View<Bytes>(reinterpret_cast<const uint8_t*>(str.data()) + start, size);
}

