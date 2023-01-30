#include "safehash.h"

uint64_t SafeHash::splitmix(uint64_t i) {
  i += 0x9e3779b97f4a7c15;
  i = (i ^ (i >> 30)) * 0xbf58476d1ce4e5b9;
  i = (i ^ (i >> 27)) * 0x94d049bb133111eb;
  return i ^ (i >> 31);
}

size_t SafeHash::operator()(uint64_t i) {
  static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
  return Utils::splitmix(i + FIXED_RANDOM);
}

size_t SafeHash::operator()(const Address& add) {
  static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
  return Utils::splitmix(std::hash<std::string>()(add.get()) + FIXED_RANDOM);
}

size_t SafeHash::operator()(const std::string& str) {
  static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
  return Utils::splitmix(std::hash<std::string>()(str) + FIXED_RANDOM);
}

size_t SafeHash::operator()(const boost::asio::ip::address add) {
  static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
  return Utils::splitmix(std::hash<std::string>()(add.to_string()) + FIXED_RANDOM);
}

template <typename T> size_t SafeHash::operator()(const std::shared_ptr<T>& ptr) {
  static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
  return Utils::splitmix(std::hash<std::shared_ptr<T>>()(ptr) + FIXED_RANDOM);
}

template <unsigned N> size_t SafeHash::operator()(const FixedStr<N>& str) {
  static const uint64_t FIXED_RANDOM = std::chrono::steady_clock::now().time_since_epoch().count();
  return Utils::splitmix(std::hash<std::string>()(str.get()) + FIXED_RANDOM);
}

