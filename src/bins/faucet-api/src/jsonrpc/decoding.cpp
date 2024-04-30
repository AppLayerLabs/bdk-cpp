/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "decoding.h"
#include "../faucetmanager.h"

namespace Faucet {
namespace JsonRPC::Decoding {

  bool checkJsonRPCSpec(const json& request) {
    try {
      // "jsonrpc": "2.0" is a MUST
      if (!request.contains("jsonrpc")) return false;
      if (request["jsonrpc"].get<std::string>() != "2.0") return false;

      // "method" is a MUST
      if (!request.contains("method")) return false;

      // Params MUST be Object or Array.
      if (
        request.contains("params") && (!request["params"].is_object() && !request["params"].is_array())
      ) return false;

      return true;
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while checking json RPC spec: ") + e.what()
      );
      throw DynamicException("Error while checking json RPC spec: " + std::string(e.what()));
    }
  }

  Methods getMethod(const json& request) {
    try {
      const std::string& method = request["method"].get<std::string>();
      auto it = methodsLookupTable.find(method);
      if (it == methodsLookupTable.end()) return Methods::invalid;
      return it->second;
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while getting method: ") + e.what()
      );
      throw DynamicException("Error while checking json RPC spec: " + std::string(e.what()));
    }
  }
  // https://www.jsonrpc.org/specification
  void dripToAddress(const json& request, Manager& faucet) {
    static const std::regex addFilter("^0x[0-9,a-f,A-F]{40}$");
    try {
      const auto address = request["params"].at(0).get<std::string>();
      if (!std::regex_match(address, addFilter)) throw DynamicException("Invalid address hex");
      faucet.dripToAddress(Address(Hex::toBytes(address)));
    } catch (std::exception& e) {
      Logger::logToDebug(LogType::ERROR, Log::JsonRPCDecoding, __func__,
        std::string("Error while decoding dripToAddress: ") + e.what()
      );
      throw DynamicException("Error while decoding dripToAddress: " + std::string(e.what()));
    }
  }
}
}
