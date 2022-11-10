#include "solidity.h"

bool Solidity::checkType(std::string type, json value) {
  if (type == "function") {
    // Check both "funcName()" and every type inside the "()"
    std::string hdr = value.get<std::string>();
    if (hdr.find("(") == std::string::npos || hdr.find(")") == std::string::npos) {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid Function");
      throw std::runtime_error(std::string(__func__) + "ABI Invalid Function ");
    }
    hdr.erase(0, hdr.find("(") + 1);
    hdr.replace(hdr.find(")"), 1, ",");
    int pos;
    while ((pos = hdr.find(",")) != std::string::npos) {
      std::string hdrType = hdr.substr(0, pos);
      if (
        hdrType != "uint256" && hdrType != "address" &&
        hdrType != "bool" && hdrType != "bytes" &&
        hdrType != "string" && hdrType != "uint256[]" &&
        hdrType != "address[]" && hdrType != "bool[]" &&
        hdrType != "bytes[]" && hdrType != "string[]"
      ) {
        Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid Function");
        throw std::runtime_error(std::string(__func__) + "ABI Invalid Function ");
      }
      hdr.erase(0, pos + 1);
    }
    return true;
  } else if (type == "uint256") {
    std::string it = value.get<std::string>();
    if (!std::all_of(it.begin(), it.end(), ::isdigit)) {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid uint256");
      throw std::runtime_error(std::string(__func__) + "ABI Invalid uint256");
    }
    return true;
  } else if (type == "address") {
    std::string it = value.get<std::string>();
    if (!Utils::isAddress(it, true)) {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid address");
      throw std::runtime_error(std::string(__func__) + "ABI Invalid address");
    }
    return true;
  } else if (type == "bool") {
    std::string it = value.get<std::string>();
    if (it != "0" && it != "1" && it != "true" && it != "false") {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid boolean");
      throw std::runtime_error(std::string(__func__) + "ABI Invalid boolean");
    }
    return true;
  } else if (type == "bytes") {
    if (!Utils::isHex(value.get<std::string>())) {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid bytes");
      throw std::runtime_error(std::string(__func__) + "ABI Invalid bytes");
    }
    return true;
  } else if (type == "string") {
    std::string it = Utils::utf8ToHex(value.get<std::string>());
    if (!Utils::isHex(it)) {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid string");
      throw std::runtime_error(std::string(__func__) + "ABI Invalid string");
    }
    return true;
  } else if (type == "uint256[]") {
    for (json item : value) {
      std::string it = item.get<std::string>();
      if (!std::all_of(it.begin(), it.end(), ::isdigit)) {
        Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid uint256 array");
        throw std::runtime_error(std::string(__func__) + "ABI Invalid uint256 array");
      }
    } 
    return true;
  } else if (type == "address[]") {
    for (json item : value) {
      std::string it = item.get<std::string>();
      if (!Utils::isAddress(it, true)) {
        Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid address array");
        throw std::runtime_error(std::string(__func__) + "ABI Invalid address array");
      }
    }
    return true;
  } else if (type == "bool[]") {
    for (json item : value) {
      std::string it = item.get<std::string>();
      if (it != "0" && it != "1" && it != "true" && it != "false") {
        Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid boolean array");
        throw std::runtime_error(std::string(__func__) + "ABI Invalid boolean array");
      }
    }
    return true;
  } else if (type == "bytes[]") {
    for (json item : value) {
      if (!Utils::isHex(item.get<std::string>())) {
        Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid bytes array");
        throw std::runtime_error(std::string(__func__) + "ABI Invalid bytes array");
      }
    }
    return true;
  } else if (type == "string[]") {
    for (json item : value) {
      std::string it = Utils::utf8ToHex(item.get<std::string>());
      if (!Utils::isHex(it)) {
        Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid string array");
        throw std::runtime_error(std::string(__func__) + "ABI Invalid string array");
      }
    }
    return true;
  }
  Utils::LogPrint(Log::ABI, __func__, " Error: ABI Unsupported or invalid type");
  throw std::runtime_error(std::string(__func__) + "ABI Unsupported or invalid type");
  return false;  // ABI Unsupported Or Invalid Type
}

std::string Solidity::packFunction(std::string func) {
  return Utils::bytesToHex(Utils::sha3(func).get()).substr(0, 8);
}

std::string Solidity::packUint(std::string num) {
  return Hash(boost::lexical_cast<uint256_t>(num)).hex();
}

std::string Solidity::packAddress(std::string add) {
  Utils::toLowercaseAddress(add);
  Utils::stripHexPrefix(add);
  return Utils::padLeft(add,64);
}

std::string Solidity::packBool(std::string b) {
  if (b == "true") b = "1";
  else if (b == "false") b = "0";
  return Utils::padLeft(b, 64);
}

std::string Solidity::packBytes(std::string hex) {
  std::string hexStrip, hexOffset, hexLength, hexData = "";
  int padding = 0;
  hexStrip = hex;
  Utils::stripHexPrefix(hexStrip);
  hexOffset = Utils::padLeft(Utils::uintToHex(32), 64);
  hexLength = Utils::padLeft(Utils::uintToHex(hexStrip.length() / 2), 64);
  do { padding += 64; } while (padding < hexStrip.length());
  hexData = Utils::padRight(hexStrip, padding);
  return hexOffset + hexLength + hexData;
}

std::string Solidity::packString(std::string str) {
  std::string strStrip, strOffset, strLength, strData = "";
  int padding = 0;
  strStrip = str;
  strStrip = Utils::utf8ToHex(str);
  Utils::stripHexPrefix(str);
  strOffset = Utils::padLeft(Utils::uintToHex(32), 64);
  strLength = Utils::padLeft(Utils::uintToHex(strStrip.length() / 2), 64);
  do { padding += 64; } while (padding < strStrip.length());
  strData = Utils::padRight(strStrip, padding);
  return strOffset + strLength + strData;
}

std::string Solidity::packUintArray(std::vector<std::string> numV) {
  std::string arrOffset, arrSize, arrData = "";
  arrOffset = Utils::padLeft(Utils::uintToHex(32), 64);
  arrSize = Utils::padLeft(Utils::uintToHex(numV.size()), 64);
  for (std::string num : numV) {
    arrData += Solidity::packUint(num);
  }
  return arrOffset + arrSize + arrData;
}

std::string Solidity::packAddressArray(std::vector<std::string> addV) {
  std::string arrOffset, arrSize, arrData = "";
  arrOffset = Utils::padLeft(Utils::uintToHex(32), 64);
  arrSize = Utils::padLeft(Utils::uintToHex(addV.size()), 64);
  for (std::string add : addV) {
    Utils::stripHexPrefix(add);
    Utils::toLowercaseAddress(add);
    arrData += Utils::padLeft(add,64);
  }
  return arrOffset + arrSize + arrData;
}

std::string Solidity::packBoolArray(std::vector<std::string> bV) {
  std::string arrOffset, arrSize, arrData = "";
  arrOffset = Utils::padLeft(Utils::uintToHex(32), 64);
  arrSize = Utils::padLeft(Utils::uintToHex(bV.size()), 64);
  for (std::string b : bV) {
    if (b == "true") b = "1";
    else if (b == "false") b = "0";
    arrData += Utils::padLeft(b, 64);
  }
  return arrOffset + arrSize + arrData;
}

std::string Solidity::packBytesArray(std::vector<std::string> hexV) {
  std::string arrOffset, arrSize = "";
  std::vector<std::string> hexStrip, hexOffset, hexLength, hexData = {};
  arrOffset = Utils::padLeft(Utils::uintToHex(32), 64);
  arrSize = Utils::padLeft(Utils::uintToHex(hexV.size()), 64);
  int totalPaddings = 0;
  for (int i = 0; i < hexV.size(); i++) {
    std::string hS, hO, hL, hD = "";
    int padding = 0;
    hS = hexV[i]; 
    Utils::stripHexPrefix(hS);
    if (hS.length() % 2 != 0) hS.insert(0, "0");  // Complete odd bytes ("aaa" = "0aaa")
    hL = Utils::uintToHex(hS.length() / 2); // Get length first so we can get the right offset
    hO = Utils::uintToHex((32 * hexV.size()) + (32 * i) + (32 * totalPaddings)); // (offsets) + (lengths) + (datas)
    do { padding += 64; } while (padding < hS.length());
    totalPaddings += padding / 64;
    hD = Utils::padRight(hS, padding);
    hexStrip.push_back(Utils::padLeft(hS, 64));
    hexOffset.push_back(Utils::padLeft(hO, 64));
    hexLength.push_back(Utils::padLeft(hL, 64));
    hexData.push_back(Utils::padRight(hD, 64));
  }
  std::string ret = arrOffset + arrSize;
  for (std::string offset : hexOffset) ret += offset;
  for (int i = 0; i < hexV.size(); i++) {
    ret += hexLength[i] + hexData[i];
  }
  return ret;
}

std::string Solidity::packStringArray(std::vector<std::string> strV) {
  std::string arrOffset, arrSize = "";
  std::vector<std::string> strStrip, strOffset, strLength, strData = {};
  arrOffset = Utils::padLeft(Utils::uintToHex(32), 64);
  arrSize = Utils::padLeft(Utils::uintToHex(strV.size()), 64);
  int totalPaddings = 0;
  for (int i = 0; i < strV.size(); i++) {
    std::string sS, sO, sL, sD = "";
    int padding = 0;
    sS = strV[i];
    Utils::stripHexPrefix(sS);
    sS = Utils::utf8ToHex(sS);
    sL = Utils::uintToHex(sS.length() / 2); // Get length first so we can get the right offset
    sO = Utils::uintToHex((32 * strV.size()) + (32 * i) + (32 * totalPaddings)); // (offsets) + (lengths) + (datas)
    do { padding += 64; } while (padding < sS.length());
    totalPaddings += padding / 64;
    sD = Utils::padRight(sS, padding);
    strStrip.push_back(Utils::padLeft(sS, 64));
    strOffset.push_back(Utils::padLeft(sO, 64));
    strLength.push_back(Utils::padLeft(sL, 64));
    strData.push_back(Utils::padRight(sD, 64));
  }
  std::string ret = arrOffset + arrSize;
  for (std::string offset : strOffset) ret += offset;
  for (int i = 0; i < strV.size(); i++) {
    ret += strLength[i] + strData[i];
  }
  return ret;
}

std::string Solidity::packMulti(json args, std::string func) { // err, std::string func) {
  // Handle function ID first if it exists
  std::string ret = "0x";
  if (!func.empty()) {
    if (!checkType("function", func)) {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid type");
      throw std::runtime_error(std::string(__func__) + "ABI Invalid type");
    }
    ret += packFunction(func);
  }
  uint64_t nextOffset = 32 * args.size();
  std::string arrToAppend = "";

  // Treat singular and multiple types differently
  // (one is a single object, the other is an array)
  if (!args.is_array()) {
    // Get type and value, and check if both are valid
    std::string type;
    json value;
    if (
      (args.contains("type") && args.contains("value")) ||
      (args.contains("t") && args.contains("v"))
    ) {
      type = (args.contains("t") ? args["t"] : args["type"]);
      value = (args.contains("v") ? args["v"] : args["value"]);
    } else {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Missing type or value");
      throw std::runtime_error(std::string(__func__) + "ABI Missing type or value");
    }
    if (!checkType(type, value)) {
      Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid type");
      throw std::runtime_error(std::string(__func__) + "ABI Invalid type");
    }

    // Parse value according to type
    if (type == "uint256[]") {
      ret += packUintArray(value.get<std::vector<std::string>>());
    } else if (type == "address[]") {
      ret += packAddressArray(value.get<std::vector<std::string>>());
    } else if (type == "bool[]") {
      ret += packBoolArray(value.get<std::vector<std::string>>());
    } else if (type == "bytes[]") {
      ret += packBytesArray(value.get<std::vector<std::string>>());
    } else if (type == "string[]") {
      ret += packStringArray(value.get<std::vector<std::string>>());
    } else if (type == "uint256") {
      ret += packUint(value.get<std::string>());
    } else if (type == "address") {
      ret += packAddress(value.get<std::string>());
    } else if (type == "bool") {
      ret += packBool(value.get<std::string>());
    } else if (type == "bytes") {
      ret += packBytes(value.get<std::string>());
    } else if (type == "string") {
      ret += packString(value.get<std::string>());
    }
  } else {
    for (json arg : args) {
      // Get type and value, and check if both are valid
      std::string type;
      json value;
      if (
        (arg.contains("type") && arg.contains("value")) ||
        (arg.contains("t") && arg.contains("v"))
      ) {
        type = (arg.contains("t") ? arg["t"] : arg["type"]);
        value = (arg.contains("v") ? arg["v"] : arg["value"]);
      } else {
        Utils::LogPrint(Log::ABI, __func__, " Error: ABI Missing type or value");
        throw std::runtime_error(std::string(__func__) + "ABI Missing type or value");
      }
      if (!checkType(type, value)) {
        Utils::LogPrint(Log::ABI, __func__, " Error: ABI Invalid type");
        throw std::runtime_error(std::string(__func__) + "ABI Invalid type");
      }

      // Parse value according to type
      if (type.find("[") != std::string::npos) {  // Type is array
        std::string arrType = type.substr(0, type.find("["));
        ret += Utils::padLeft(Utils::uintToHex(nextOffset), 64);  // Array offset
        if (arrType != "bytes" && arrType != "string") {
          nextOffset += 64 * arg.size();  // In chars
        }
        if (arrType == "uint256") {
          arrToAppend += packUintArray(value.get<std::vector<std::string>>()).substr(64);
        } else if (arrType == "address") {
          arrToAppend += packAddressArray(value.get<std::vector<std::string>>()).substr(64);
        } else if (arrType == "bool") {
          arrToAppend += packBoolArray(value.get<std::vector<std::string>>()).substr(64);
        } else if (arrType == "bytes" || arrType == "string") {
          std::string packed = (arrType == "bytes")
            ? packBytesArray(value.get<std::vector<std::string>>()).substr(64)
            : packStringArray(value.get<std::vector<std::string>>()).substr(64);
          nextOffset += 32 * (packed.length() / 64); // Offset in bytes, packed in chars
          arrToAppend += packed;
        }
      } else {  // Type is not array
        std::string val = value.get<std::string>();
        if (type == "uint256") {
          ret += packUint(val);
        } else if (type == "address") {
          ret += packAddress(val);
        } else if (type == "bool") {
          ret += packBool(val);
        } else if (type == "bytes" || type == "string") {
          ret += Utils::padLeft(Utils::uintToHex(nextOffset), 64);
          std::string packed = (type == "bytes")
            ? packBytes(value).substr(64) : packString(value).substr(64);
          nextOffset += 32 * (packed.length() / 64);
          arrToAppend += packed;
        }
      }
    }
  }

  ret += arrToAppend;
  return ret;
}

