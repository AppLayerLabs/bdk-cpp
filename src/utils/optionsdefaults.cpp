/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "options.h"

Options Options::binaryDefaultOptions(const std::string& rootPath) {
  return {
    rootPath,
    "BDK/cpp/linux_x86-64/0.2.0",
    2,
    8080,
    Address(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6")),
    8081,
    2000,
    10000,
    1000,
    IndexingMode::RPC,
    json()
  };
}

