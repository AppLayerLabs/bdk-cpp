/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef NODECONNS_H
#define NODECONNS_H

#include "encoding.h" // NodeID, NodeInfo

#include "../../utils/logger.h"
#include "../../utils/safehash.h"

#include <algorithm> // std::find
#include <thread> // std::this_thread::sleep_for
#include <shared_mutex>
#include <unordered_map>

// TODO: tests for NodeConns (if necessary)

namespace P2P {

  // Forward declaration.
  class ManagerNormal;

  /**
   * Class that manages a list of connected nodes and their info, keeping it
   * synced periodically with the most up-to-date node info possible.
   */
  class NodeConns {
    private:
      ManagerNormal& manager_;  ///< Reference to the P2P engine object that owns this.

      /// List of valid NodeInfo objects from remote nodes.
      std::unordered_map<P2P::NodeID, P2P::NodeInfo, SafeHash> nodeInfo_;

      /// List of last time since epoch in milliseconds we received a remote node's most recent info, so we can expire them.
      std::unordered_map<P2P::NodeID, uint64_t, SafeHash> nodeInfoTime_;

      mutable std::shared_mutex stateMutex_; ///< Mutex for serializing all inner state and requests to it.

      std::future<void> loopFuture_;  ///< Future object holding the thread for the nodeconns loop.
      std::atomic<bool> stop_ = false; ///< Flag for stopping nodeconns processing.

    public:
      /**
       * Constructor.
       * @param manager Reference to the P2P engine object that owns this.
       */
      explicit NodeConns(ManagerNormal& manager) : manager_(manager) {}

      /// Save an incoming info update from a remote node.
      void incomingInfo(const P2P::NodeID& sender, const P2P::NodeInfo& info);

      /// Get a copy of the nodeInfo_ map.
      std::unordered_map<P2P::NodeID, P2P::NodeInfo, SafeHash> getConnected();

      void forceRefresh(); ///< Caller synchronously forces a refresh of the nodeInfos of all currently connected nodes.

      void loop(); ///< Nodeconns loop (sends nodeinfo to peers and times out remote peer nodeinfo as needed).

      void start(); ///< Start the nodeconns worker thread if necessary.

      void stop();  ///< Stop the nodeconns worker thread if any.
  };
};

#endif // NODECONNS_H
