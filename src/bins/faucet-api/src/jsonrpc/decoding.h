/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef JSONRPC_DECODING_H
#define JSONRPC_DECODING_H
#include <regex>

#include "utils/utils.h"

#include "methods.h"

// Forward declarations.
class Storage;

namespace Faucet {
  class Manager;
  /**
   * Namespace for decoding JSON-RPC data.
   * All functions require a JSON object that is the request itself to be operated on.
   */
  namespace JsonRPC::Decoding {
    /**
     * Helper function to get the method of the JSON-RPC request.
     * @param request The request object.
     * @return The method inside the request, or `invalid` if the method is not found.
     */
    Methods getMethod(const json& request);
    /**
     * Helper function to check if a given JSON-RPC request is valid.
     * Does NOT check if the method called is valid, only if the request follows JSON-RPC 2.0 spec.
     * @param request The request object.
     * @return `true` if request is valid, `false` otherwise.
     */
    bool checkJsonRPCSpec(const json& request);
    /**
     * Helper function to check if a given JSON-RPC request is valid.
     * Does NOT check if the method called is valid, only if the request follows JSON-RPC 2.0 spec.
     * @param request The request object.
     * @param faucet Reference to the faucet manager.
     * @return `true` if request is valid, `false` otherwise.
     */
    void dripToAddress(const json& request, Manager& faucet);
  }
}
#endif /// JSONRPC_DECODING_H
