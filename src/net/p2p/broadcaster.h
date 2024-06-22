/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BROADCASTER_H
#define BROADCASTER_H

#include "encoding.h" // NodeID, NodeInfo

#include "../../utils/logger.h"
#include "../../utils/safehash.h"

#include <algorithm>
#include <thread>
#include <shared_mutex>
#include <unordered_map>

// Forward declaration.
class Storage;
class State;

namespace P2P {
  // Forward declaration.
  class ManagerNormal;

  /**
   * The Broadcaster is the component of the P2P engine that encapsulates all
   * P2P multi-hop networking needs, which ultimately involves sending and
   * receiving all messages that have  the 'Broadcast' command code.
   * @see net/p2p/encoding.h
   * @see net/p2p/managernormal.h
   */
  class Broadcaster {
    private:
      ManagerNormal& manager_; ///< Reference to the P2P engine object that owns this.
      const Storage& storage_; ///< Reference to the blockchain's storage.
      State& state_; ///< Reference to the blockchain's state.

      const Options& getOptions(); ///< Get the Options object from the P2P engine that owns this Broadcaster.

      /**
       * Broadcast a message to all connected nodes.
       * @param message The message to broadcast.
       * @param originalSender Node whose latest known peers won't receive the message from us (optional).
       */
      void broadcastMessage(const std::shared_ptr<const Message> message, const std::optional<NodeID>& originalSender);

      /**
       * Handle a Validator transaction broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleTxValidatorBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a block transaction broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleTxBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a block broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleBlockBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Handle a info broadcast message.
       * @param session The node that sent the broadcast.
       * @param message The message that was broadcast.
       */
      void handleInfoBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);
  
    public:
      /**
       * Constructor.
       * @param manager Reference to the P2P engine object that owns this.
       * @param storage Pointer to the blockchain's storage.
       * @param state Pointer to the blockchain's state.
       */
      explicit Broadcaster(ManagerNormal& manager, const Storage &storage, State &state) 
        : manager_(manager), storage_(storage), state_(state) 
      {}

      /**
       * Handle a broadcast from a node.
       * @param session The session that sent the broadcast.
       * @param message The broadcast message to handle.
       */
      void handleBroadcast(const NodeID &nodeId, const std::shared_ptr<const Message>& message);

      /**
       * Broadcast a Validator transaction to all connected nodes.
       * @param tx The transaction to broadcast.
       */
      void broadcastTxValidator(const TxValidator& tx);

      /**
       * Broadcast a block transaction to all connected nodes.
       * @param txBlock The transaction to broadcast.
       */
      void broadcastTxBlock(const TxBlock& txBlock);

      /**
       * Broadcast a block to all connected nodes.
       * @param block The block to broadcast.
       */
      void broadcastBlock(const std::shared_ptr<const FinalizedBlock>& block);
  };
};

#endif // BROADCASTER_H
