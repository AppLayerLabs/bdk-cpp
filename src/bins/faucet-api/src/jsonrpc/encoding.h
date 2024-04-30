/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_ENCODING_H
#define JSONRPC_ENCODING_H

#include "utils/utils.h"

// Forward declarations.

namespace Faucet {
 /// Namespace for encoding JSON-RPC data.
 namespace JsonRPC::Encoding {
   json dripToAddress();
 }
}

#endif  // JSONRPC_ENCODING_H
