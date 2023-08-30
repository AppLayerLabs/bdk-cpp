/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "jsonabi.h"

void JsonAbi::to_json(json &jsonObject,
                    const ABI::MethodDescription &description) {
  jsonObject["name"] = description.name;
  jsonObject["stateMutability"] = description.stateMutability;
  jsonObject["type"] = description.type;

  for (auto &input : description.inputs) {
    jsonObject["inputs"].push_back({{"internalType", input.second},
                                    {"name", input.first},
                                    {"type", input.second}});
  }

  for (auto &output : description.outputs) {
    jsonObject["outputs"].push_back({{"internalType", output.second},
                                     {"name", output.first},
                                     {"type", output.second}});
  }
}

