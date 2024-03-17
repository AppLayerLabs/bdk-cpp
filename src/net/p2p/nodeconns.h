/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef NODECONNS_H
#define NODECONNS_H

#include "encoding.h" // NodeID, NodeInfo
#include "managernormal.h"

#include "../../utils/logger.h"
#include "../../utils/safehash.h"

#include <algorithm> // std::find
#include <thread> // std::this_thread::sleep_for
#include <unordered_map>

// TODO: tests for NodeConns (if necessary)

class Blockchain; // Forward declaration.

namespace P2P {
  /**
   * Class that manages a list of connected nodes and their info, keeping it
   * synced periodically with the most up-to-date node info possible.
   */
  class NodeConns {
    private:
      Blockchain& blockchain_;  ///< Reference to the blockchain.

      /// List of currently connected nodes and their info.
      std::unordered_map<P2P::NodeID, P2P::NodeInfo, SafeHash> connected_;

    public:
      /**
       * Constructor.
       * @param blockchain Reference to the blockchain.
       */
      explicit NodeConns(Blockchain& blockchain) : blockchain_(blockchain) {}

      /// Getter.
      std::unordered_map<P2P::NodeID, P2P::NodeInfo, SafeHash>& getConnected() { return this->connected_; }

      void refresh(); ///< Refresh the list of currently connected nodes.
  };
};

#endif // NODECONNS_H
