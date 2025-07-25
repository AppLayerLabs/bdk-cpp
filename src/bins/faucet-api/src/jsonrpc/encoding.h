/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_ENCODING_H
#define JSONRPC_ENCODING_H

#include "utils/utils.h"

namespace Faucet {
  /// Namespace for encoding JSON-RPC data.
  namespace JsonRPC::Encoding {
    /**
     * Encode a "dripToAddress" request.
     * @return The formed request.
     */
    json dripToAddress();
  }
}

#endif  // JSONRPC_ENCODING_H
