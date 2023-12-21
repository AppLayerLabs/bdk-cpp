/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "jsonabi.h"

void JsonAbi::methodToJson(json& obj, const ABI::MethodDescription& desc) {
  obj["name"] = desc.name;
  obj["stateMutability"] = desc.stateMutability;
  obj["type"] = desc.type;
  for (auto& input : desc.inputs) {
    obj["inputs"].push_back({
      {"internalType", input.second},
      {"name", input.first},
      {"type", input.second}
    });
  }
  for (auto& output : desc.outputs) {
    obj["outputs"].push_back({
      {"internalType", output.second},
      {"name", output.first},
      {"type", output.second}
    });
  }
}

void JsonAbi::eventToJson(json& obj, const ABI::EventDescription& desc) {
  obj["name"] = desc.name;
  obj["type"] = "event";
  obj["anonymous"] = desc.anonymous;
  for (auto& arg : desc.args) {
    obj["inputs"].push_back({
      {"name", std::get<0>(arg)},
      {"type", std::get<1>(arg)},
      {"indexed", std::get<2>(arg)}
    });
  }
}

