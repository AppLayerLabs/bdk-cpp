/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "utils.h"

#include <csignal>

std::mutex log_lock;
std::mutex debug_mutex;
std::mutex cout_mutex;

std::atomic<bool> Utils::logToCout = false;

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

Functor Utils::getFunctor(const evmc_message& msg) {
  Functor ret;
  if (msg.input_size < 4) return ret;
  // Memcpy the first 4 bytes from the input data to the function signature
  ret.value = Utils::bytesToUint32(BytesArrView(msg.input_data, 4));
  return ret;
}

Functor Utils::makeFunctor(const std::string& functionSignature) {
  Functor ret;
  // Create the hash
  Hash hash = Utils::sha3(BytesArrView(reinterpret_cast<const uint8_t*>(functionSignature.data()), functionSignature.size()));
  // Copy the first 4 bytes of the hash to the value
  ret.value = Utils::bytesToUint32(hash.view(0,4));
  return ret;
}

BytesArrView Utils::getFunctionArgs(const evmc_message& msg) {
  if (msg.input_size < 4) return BytesArrView();
  return BytesArrView(msg.input_data + 4, msg.input_size - 4);
}

void Utils::safePrint(std::string_view str) {
  if (!Utils::logToCout) return; // Never print if we are in a test
  std::lock_guard lock(cout_mutex);
  std::cout << str << std::endl;
}

void Utils::safePrintTest(std::string_view str) {
  std::lock_guard lock(cout_mutex);
  std::cout << str << std::endl;
}

Hash Utils::sha3(const BytesArrView input) {
  ethash_hash256 h = ethash_keccak256(input.data(), input.size());
  BytesArr<32> ret;
  std::copy(reinterpret_cast<const Byte*>(h.bytes), reinterpret_cast<const Byte*>(h.bytes + 32), ret.begin());
  return std::move(ret);
}

uint256_t Utils::evmcUint256ToUint256(const evmc::uint256be& i) {
  // We can use the uint256ToBytes directly as it is std::span and we can create a span from an array
  return Utils::bytesToUint256(BytesArrView(reinterpret_cast<const uint8_t*>(&i.bytes[0]), 32));
}
evmc::uint256be Utils::uint256ToEvmcUint256(const uint256_t& i) {
  // Convert the uint256_t to BytesArr<32> then copy it to evmc::uint256be
  // evmc::uint256be is a struct with a single member, bytes, which holds a uint256 value in *big-endian* order
  evmc::uint256be ret;
  BytesArr<32> bytes = Utils::uint256ToBytes(i);
  std::copy(bytes.begin(), bytes.end(), ret.bytes);
  return ret;
}
BytesArr<32> Utils::evmcUint256ToBytes(const evmc::uint256be& i) {
  BytesArr<32> ret;
  std::copy(i.bytes, i.bytes + 32, ret.begin());
  return ret;
}
evmc::uint256be Utils::bytesToEvmcUint256(const BytesArrView b) {
  evmc::uint256be ret;
  std::copy(b.begin(), b.end(), ret.bytes);
  return ret;
}

Account::Account(const BytesArrView& bytes) {
  if (bytes.size() < 73) throw DynamicException(std::string(__func__) + ": Invalid bytes size");
  this->balance = Utils::bytesToUint256(bytes.subspan(0,32));
  this->nonce = Utils::bytesToUint64(bytes.subspan(32,8));
  this->codeHash = Hash(bytes.subspan(40,32));
  if (bytes[72] > 2) throw DynamicException(std::string(__func__) + ": Invalid contract type");
  this->contractType = ContractType(bytes[72]);
  if (bytes.size() > 73) this->code = Bytes(bytes.begin() + 73, bytes.end());
}

Bytes Account::serialize() const {
  Bytes ret;
  Utils::appendBytes(ret, Utils::uint256ToBytes(this->balance));
  Utils::appendBytes(ret, Utils::uint64ToBytes(this->nonce));
  Utils::appendBytes(ret, this->codeHash);
  ret.insert(ret.end(), char(this->contractType));
  Utils::appendBytes(ret, this->code);
  return ret;
}

Bytes Utils::cArrayToBytes(const uint8_t* arr, size_t size) {
  Bytes ret;
  ret.reserve(size);
  for (size_t i = 0; i < size; i++) ret.push_back(arr[i]);
  return ret;
}

BytesArr<31> Utils::uint248ToBytes(const uint248_t &i) {
  BytesArr<31> ret;
  Bytes tmp;
  tmp.reserve(31);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  // Replace bytes from tmp to ret to make it the right size. Applies to all similar functions.
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[30 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<30> Utils::uint240ToBytes(const uint240_t &i) {
  BytesArr<30> ret;
  Bytes tmp;
  tmp.reserve(30);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[29 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<29> Utils::uint232ToBytes(const uint232_t &i) {
  BytesArr<29> ret;
  Bytes tmp;
  tmp.reserve(29);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[28 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<28> Utils::uint224ToBytes(const uint224_t &i) {
  BytesArr<28> ret;
  Bytes tmp;
  tmp.reserve(28);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[27 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<27> Utils::uint216ToBytes(const uint216_t &i) {
  BytesArr<27> ret;
  Bytes tmp;
  tmp.reserve(27);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[26 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<26> Utils::uint208ToBytes(const uint208_t &i) {
  BytesArr<26> ret;
  Bytes tmp;
  tmp.reserve(26);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[25 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<25> Utils::uint200ToBytes(const uint200_t &i) {
  BytesArr<25> ret;
  Bytes tmp;
  tmp.reserve(25);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[24 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<24> Utils::uint192ToBytes(const uint192_t &i) {
  BytesArr<24> ret;
  Bytes tmp;
  tmp.reserve(24);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[23 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<23> Utils::uint184ToBytes(const uint184_t &i) {
  BytesArr<23> ret;
  Bytes tmp;
  tmp.reserve(23);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[22 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<22> Utils::uint176ToBytes(const uint176_t &i) {
  BytesArr<22> ret;
  Bytes tmp;
  tmp.reserve(22);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[21 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<21> Utils::uint168ToBytes(const uint168_t &i) {
  BytesArr<21> ret;
  Bytes tmp;
  tmp.reserve(21);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[20 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<20> Utils::uint160ToBytes(const uint160_t &i) {
  BytesArr<20> ret;
  Bytes tmp;
  tmp.reserve(20);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[19 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<19> Utils::uint152ToBytes(const uint152_t &i) {
  BytesArr<19> ret;
  Bytes tmp;
  tmp.reserve(19);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[18 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<18> Utils::uint144ToBytes(const uint144_t &i) {
  BytesArr<18> ret;
  Bytes tmp;
  tmp.reserve(18);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[17 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<17> Utils::uint136ToBytes(const uint136_t &i) {
  BytesArr<17> ret;
  Bytes tmp;
  tmp.reserve(17);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[16 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<16> Utils::uint128ToBytes(const uint128_t &i) {
  BytesArr<16> ret;
  Bytes tmp;
  tmp.reserve(16);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[15 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<15> Utils::uint120ToBytes(const uint120_t &i) {
  BytesArr<15> ret;
  Bytes tmp;
  tmp.reserve(15);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[14 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<14> Utils::uint112ToBytes(const uint112_t &i) {
  BytesArr<14> ret;
  Bytes tmp;
  tmp.reserve(14);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[13 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<13> Utils::uint104ToBytes(const uint104_t &i) {
  BytesArr<13> ret;
  Bytes tmp;
  tmp.reserve(13);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[12 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<12> Utils::uint96ToBytes(const uint96_t &i) {
  BytesArr<12> ret;
  Bytes tmp;
  tmp.reserve(12);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[11 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<11> Utils::uint88ToBytes(const uint88_t &i) {
  BytesArr<11> ret;
  Bytes tmp;
  tmp.reserve(11);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[10 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<10> Utils::uint80ToBytes(const uint80_t &i) {
  BytesArr<10> ret;
  Bytes tmp;
  tmp.reserve(10);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[9 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<9> Utils::uint72ToBytes(const uint72_t &i) {
  BytesArr<9> ret;
  Bytes tmp;
  tmp.reserve(9);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[8 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<7> Utils::uint56ToBytes(const uint56_t &i) {
  BytesArr<7> ret;
  Bytes tmp;
  tmp.reserve(7);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[6 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<6> Utils::uint48ToBytes(const uint48_t &i) {
  BytesArr<6> ret;
  Bytes tmp;
  tmp.reserve(6);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[5 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<5> Utils::uint40ToBytes(const uint40_t &i) {
  BytesArr<5> ret;
  Bytes tmp;
  tmp.reserve(5);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[4 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<3> Utils::uint24ToBytes(const uint24_t &i) {
  BytesArr<3> ret;
  Bytes tmp;
  tmp.reserve(3);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[2 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

BytesArr<32> Utils::uint256ToBytes(const uint256_t& i) {
  BytesArr<32> ret;
  Bytes tmp;
  tmp.reserve(32);
  ret.fill(0x00);
  boost::multiprecision::export_bits(i, std::back_inserter(tmp), 8);
  for (unsigned ii = 0; ii < tmp.size(); ii++) ret[31 - ii] = tmp[tmp.size() - ii - 1];
  return ret;
}

uint256_t Utils::bytesToUint256(const BytesArrView b) {
  if (b.size() != 32) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 32, got " + std::to_string(b.size())
  );
  uint256_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint248_t Utils::bytesToUint248(const BytesArrView b) {
  if (b.size() != 31) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 31, got " + std::to_string(b.size())
  );
  uint248_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint240_t Utils::bytesToUint240(const BytesArrView b) {
  if (b.size() != 30) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 30, got " + std::to_string(b.size())
  );
  uint240_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint232_t Utils::bytesToUint232(const BytesArrView b) {
  if (b.size() != 29) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 29, got " + std::to_string(b.size())
  );
  uint232_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint224_t Utils::bytesToUint224(const BytesArrView b) {
  if (b.size() != 28) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 28, got " + std::to_string(b.size())
  );
  uint224_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint216_t Utils::bytesToUint216(const BytesArrView b) {
  if (b.size() != 27) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 27, got " + std::to_string(b.size())
  );
  uint216_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint208_t Utils::bytesToUint208(const BytesArrView b) {
  if (b.size() != 26) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 26, got " + std::to_string(b.size())
  );
  uint208_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint200_t Utils::bytesToUint200(const BytesArrView b) {
  if (b.size() != 25) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 25, got " + std::to_string(b.size())
  );
  uint200_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint192_t Utils::bytesToUint192(const BytesArrView b) {
  if (b.size() != 24) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 24, got " + std::to_string(b.size())
  );
  uint192_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint184_t Utils::bytesToUint184(const BytesArrView b) {
  if (b.size() != 23) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 23, got " + std::to_string(b.size())
  );
  uint184_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint176_t Utils::bytesToUint176(const BytesArrView b) {
  if (b.size() != 22) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 22, got " + std::to_string(b.size())
  );
  uint176_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint168_t Utils::bytesToUint168(const BytesArrView b) {
  if (b.size() != 21) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 21, got " + std::to_string(b.size())
  );
  uint168_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint160_t Utils::bytesToUint160(const BytesArrView b) {
  if (b.size() != 20) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 20, got " + std::to_string(b.size())
  );
  uint160_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint152_t Utils::bytesToUint152(const BytesArrView b) {
  if (b.size() != 19) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 19, got " + std::to_string(b.size())
  );
  uint152_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint144_t Utils::bytesToUint144(const BytesArrView b) {
  if (b.size() != 18) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 18, got " + std::to_string(b.size())
  );
  uint144_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint136_t Utils::bytesToUint136(const BytesArrView b) {
  if (b.size() != 17) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 17, got " + std::to_string(b.size())
  );
  uint136_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint128_t Utils::bytesToUint128(const BytesArrView b) {
  if (b.size() != 16) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 16, got " + std::to_string(b.size())
  );
  uint128_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint120_t Utils::bytesToUint120(const BytesArrView b) {
  if (b.size() != 15) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 15, got " + std::to_string(b.size())
  );
  uint120_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint112_t Utils::bytesToUint112(const BytesArrView b) {
  if (b.size() != 14) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 16, got " + std::to_string(b.size())
  );
  uint112_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint104_t Utils::bytesToUint104(const BytesArrView b) {
  if (b.size() != 13) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 13, got " + std::to_string(b.size())
  );
  uint104_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint96_t Utils::bytesToUint96(const BytesArrView b) {
  if (b.size() != 12) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 12, got " + std::to_string(b.size())
  );
  uint96_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint88_t Utils::bytesToUint88(const BytesArrView b) {
  if (b.size() != 11) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 11, got " + std::to_string(b.size())
  );
  uint88_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint80_t Utils::bytesToUint80(const BytesArrView b) {
  if (b.size() != 10) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 10, got " + std::to_string(b.size())
  );
  uint80_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint72_t Utils::bytesToUint72(const BytesArrView b) {
  if (b.size() != 9) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 9, got " + std::to_string(b.size())
  );
  uint72_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint56_t Utils::bytesToUint56(const BytesArrView b) {
  if (b.size() != 7) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 7, got " + std::to_string(b.size())
  );
  uint56_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint48_t Utils::bytesToUint48(const BytesArrView b) {
  if (b.size() != 6) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 6, got " + std::to_string(b.size())
  );
  uint48_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint40_t Utils::bytesToUint40(const BytesArrView b) {
  if (b.size() != 5) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 5, got " + std::to_string(b.size())
  );
  uint40_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

uint24_t Utils::bytesToUint24(const BytesArrView b) {
  if (b.size() != 3) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 3, got " + std::to_string(b.size())
  );
  uint24_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);
  return ret;
}

BytesArr<8> Utils::uint64ToBytes(const uint64_t& i) {
  BytesArr<8> ret;
  std::memcpy(&ret[0], &i, 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

uint64_t Utils::bytesToUint64(const BytesArrView b) {
  if (b.size() != 8) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 8, got " + std::to_string(b.size())
  );
  uint64_t ret = 0;
  std::memcpy(&ret, b.data(), 8);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap64(ret);
  #endif
  return ret;
}

BytesArr<4> Utils::uint32ToBytes(const uint32_t& i) {
  BytesArr<4> ret;
  std::memcpy(&ret[0], &i, 4);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

uint32_t Utils::bytesToUint32(const BytesArrView b) {
  if (b.size() != 4) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 4, got " + std::to_string(b.size())
  );
  uint32_t ret = 0;
  std::memcpy(&ret, b.data(), 4);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap32(ret);
  #endif
  return ret;
}

BytesArr<2> Utils::uint16ToBytes(const uint16_t& i) {
  BytesArr<2> ret;
  std::memcpy(&ret[0], &i, 2);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    std::reverse(ret.begin(), ret.end());
  #endif
  return ret;
}

uint16_t Utils::bytesToUint16(const BytesArrView b) {
  if (b.size() != 2) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 2, got " + std::to_string(b.size())
  );
  uint16_t ret = 0;
  std::memcpy(&ret, b.data(), 2);
  #if __BYTE_ORDER == __LITTLE_ENDIAN
    return __builtin_bswap16(ret);
  #endif
  return ret;
}

BytesArr<1> Utils::uint8ToBytes(const uint8_t& i) {
  BytesArr<1> ret;
  std::memcpy(&ret[0], &i, 1);
  return ret;
}

uint8_t Utils::bytesToUint8(const BytesArrView b) {
  if (b.size() != 1) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 1, got " + std::to_string(b.size())
  );
  uint8_t ret;
  ret = b[0];
  return ret;
}

int256_t Utils::bytesToInt256(const BytesArrView b) {
  if (b.size() != 32) throw DynamicException(std::string(__func__)
    + ": Invalid bytes size - expected 32, got " + std::to_string(b.size())
  );
  uint256_t ret;
  boost::multiprecision::import_bits(ret, b.begin(), b.end(), 8);

  // Check the MSB to determine if the value is negative
  // Manually compute two's complement in reverse, since boost::multiprecision::cpp_int doesn't support it
  // Subtract one from the byte array
  if (b[0] & 0x80) {
    int borrow = 1;
    for (int i = 31; i >= 0 && borrow; i--) {
      borrow = (b[i] == 0);
      ret -= (uint256_t(1) << (8 * (31 - i)));
    }
    ret = ~ret;
    return -ret.convert_to<int256_t>();
  } else {
    return ret.convert_to<int256_t>();
  }
}

BytesArr<32> Utils::int256ToBytes(const int256_t& i) {
  BytesArr<32> ret;

  if (i < 0) {
    int256_t absValue = -i;
    Bytes tempBytes;
    boost::multiprecision::export_bits(absValue, std::back_inserter(tempBytes), 8);
    for (int j = 0; j < 32; j++) {
      if (j < tempBytes.size()) {
        ret[31 - j] = ~tempBytes[tempBytes.size() - j - 1];
      } else {
        ret[31 - j] = 0xFF;
      }
    }
    for (int j = 31; j >= 0; j--) {
      if (ret[j] != 0xFF) {
        ret[j]++;
        break;
      } else {
        ret[j] = 0x00;
      }
    }
  } else {
    Bytes tempBytes;
    boost::multiprecision::export_bits(i, std::back_inserter(tempBytes), 8);
    std::copy(tempBytes.rbegin(), tempBytes.rend(), ret.rbegin());
  }

  return ret;
}

Bytes Utils::randBytes(const int& size) {
  Bytes bytes(size, 0x00);
  RAND_bytes(bytes.data(), size);
  return bytes;
}

std::string Utils::padLeft(std::string str, unsigned int charAmount, char sign) {
  bool hasPrefix = (str.starts_with("0x") || str.starts_with("0X"));
  if (hasPrefix) str = str.substr(2);
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + padded + str;
}

std::string Utils::padRight(std::string str, unsigned int charAmount, char sign) {
  bool hasPrefix = (str.starts_with("0x") || str.starts_with("0X"));
  if (hasPrefix) str = str.substr(2);
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + str + padded;
}

Bytes Utils::padLeftBytes(const BytesArrView bytes, unsigned int charAmount, uint8_t sign) {
  size_t padding = (charAmount > bytes.size()) ? (charAmount - bytes.size()) : 0;
  Bytes padded = (padding != 0) ? Bytes(padding, sign) : Bytes(0, 0x00);
  padded.reserve(bytes.size() + padded.size());
  padded.insert(padded.end(), bytes.begin(), bytes.end());
  return padded;
}

Bytes Utils::padRightBytes(const BytesArrView bytes, unsigned int charAmount, uint8_t sign) {
  size_t padding = (charAmount > bytes.size()) ? (charAmount - bytes.size()) : 0;
  Bytes padded = (padding != 0) ? Bytes(padding, sign) : Bytes(0, 0x00);
  Bytes ret;
  ret.reserve(bytes.size() + padded.size());
  ret.insert(ret.end(), bytes.begin(), bytes.end());
  ret.insert(ret.end(), padded.begin(), padded.end());
  return ret;
}

json Utils::readConfigFile() {
  if (!std::filesystem::exists("config.json")) {
    Logger::logToDebug(LogType::INFO, Log::utils, __func__, "No config file found, generating default");
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

