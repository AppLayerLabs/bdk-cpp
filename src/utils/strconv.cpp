/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "strconv.h"

#include <algorithm> // std::transform

Bytes StrConv::cArrayToBytes(const uint8_t* arr, size_t size) {
  Bytes ret;
  ret.reserve(size);
  for (size_t i = 0; i < size; i++) ret.push_back(arr[i]);
  return ret;
}

Bytes StrConv::padLeftBytes(const View<Bytes> bytes, unsigned int charAmount, uint8_t sign) {
  size_t padding = (charAmount > bytes.size()) ? (charAmount - bytes.size()) : 0;
  Bytes padded = (padding != 0) ? Bytes(padding, sign) : Bytes(0, 0x00);
  padded.reserve(bytes.size() + padded.size());
  padded.insert(padded.end(), bytes.begin(), bytes.end());
  return padded;
}

Bytes StrConv::padRightBytes(const View<Bytes> bytes, unsigned int charAmount, uint8_t sign) {
  size_t padding = (charAmount > bytes.size()) ? (charAmount - bytes.size()) : 0;
  Bytes padded = (padding != 0) ? Bytes(padding, sign) : Bytes(0, 0x00);
  Bytes ret;
  ret.reserve(bytes.size() + padded.size());
  ret.insert(ret.end(), bytes.begin(), bytes.end());
  ret.insert(ret.end(), padded.begin(), padded.end());
  return ret;
}

std::string StrConv::padLeft(std::string str, unsigned int charAmount, char sign) {
  bool hasPrefix = (str.starts_with("0x") || str.starts_with("0X"));
  if (hasPrefix) str = str.substr(2);
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + padded + str;
}

std::string StrConv::padRight(std::string str, unsigned int charAmount, char sign) {
  bool hasPrefix = (str.starts_with("0x") || str.starts_with("0X"));
  if (hasPrefix) str = str.substr(2);
  size_t padding = (charAmount > str.length()) ? (charAmount - str.length()) : 0;
  std::string padded = (padding != 0) ? std::string(padding, sign) : "";
  return (hasPrefix ? "0x" : "") + str + padded;
}

void StrConv::toLower(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void StrConv::toUpper(std::string& str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

