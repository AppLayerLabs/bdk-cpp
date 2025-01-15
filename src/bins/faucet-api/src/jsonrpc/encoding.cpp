/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "encoding.h"

namespace Faucet {
  namespace JsonRPC::Encoding {
    json dripToAddress() {
      json ret;
      ret["jsonrpc"] = 2.0;
      ret["result"] = "0x1";
      return ret;
    }
  }
}

