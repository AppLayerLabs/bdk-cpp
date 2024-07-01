/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_METHODS_H
#define JSONRPC_METHODS_H

#include <string>

#include "../../../../libs/unordered_dense.h"

/**
 * Namespace for Ethereum's JSON-RPC Specification.
 *
 * See the following links for more info:
 *
 * https://ethereum.org/pt/developers/docs/apis/json-rpc/ (Most updated)
 *
 * https://ethereum.github.io/execution-apis/api-documentation/ (Has regex for the methods)
 *
 * https://eips.ethereum.org/EIPS/eip-1474#error-codes (Respective error codes)
 */

namespace Faucet {
  namespace JsonRPC {
    /**
     * Enum with all known methods for Ethereum's JSON-RPC.
     *
     * Check the following list for reference (`COMMAND === IMPLEMENTATION STATUS`):
     *
     * ```
     * invalid ==================================== N/A
     * dripToAddress ============================== N/A
     * ```
     */
    enum Methods {
      invalid,
      dripToAddress
    };

    /// Lookup table for the implemented methods.
    inline extern const boost::unordered_flat_map<std::string, Methods> methodsLookupTable = {
      { "dripToAddress", dripToAddress },
    };
  }
}
#endif // JSONRPC_METHODS_H
