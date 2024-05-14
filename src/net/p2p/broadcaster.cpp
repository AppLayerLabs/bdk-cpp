/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "broadcaster.h"
#include "managernormal.h"
#include "../../core/blockchain.h"

namespace P2P {

  const Options& Broadcaster::getOptions() { return manager_.getOptions(); }

  void Broadcaster::broadcastMessage(const std::shared_ptr<const Message> message, const std::optional<NodeID>& originalSender) {
    this->manager_.sendMessageToAll(message, originalSender);
  }

  void Broadcaster::handleTxValidatorBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    try {
      auto tx = BroadcastDecoder::broadcastValidatorTx(*message, getOptions().getChainID());
      // Rebroadcast only when the validator transaction was relevant to this node, i.e. absorbed into our data model.
      if (this->state_.addValidatorTx(tx) == TxStatus::ValidNew) {
        this->broadcastMessage(message, nodeId);
      }
    } catch (std::exception const& ex) {
      throw DynamicException("Invalid txValidatorBroadcast (" + std::string(ex.what()) + ")");
    }
  }

  void Broadcaster::handleTxBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    try {
      auto tx = BroadcastDecoder::broadcastTx(*message, getOptions().getChainID());
      // Rebroadcast only when the transaction was relevant to this node, i.e. absorbed into our data model.
      if (this->state_.addTx(std::move(tx)) == TxStatus::ValidNew) {
        this->broadcastMessage(message, nodeId);
      }
    } catch (std::exception const& ex) {
      throw DynamicException("Invalid txBroadcast (" + std::string(ex.what()) + ")");
    }
  }

  void Broadcaster::handleBlockBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {

    // FIXME/TODO: This is not optimal. Concurrency control should be (ideally) e.g. encapsulated
    //             in a new class that should contain a State and a Storage,
    //             and expose an interface for both components that will handle concurrency
    //             control internally. That will also avoid passing around both State and 
    //             Storage objects separately to every object that needs to interact with them.
    //
    // We require a lock here because validateNextBlock **throws** if the block is invalid.
    // The reason for locking because for that a processNextBlock race condition can occur,
    // making the same block be accepted, and then rejected, disconnecting the node.

    // Note that block broadcasts already have an implicit "duplicate detection" mechanism:
    //   we are not forwarding, to the network, blocks that we already have in storage.

    bool rebroadcast = false;
    try {
      auto block = BroadcastDecoder::broadcastBlock(*message, getOptions().getChainID());
      std::unique_lock lock(this->blockBroadcastMutex_);
      // Assuming that if the block has been seen, then it is completely irrelevant to us
      if (! this->storage_.blockExists(block.getHash())) {
        const auto expectedHeight = this->storage_.latest()->getNHeight() + 1;
        // process and connect the block if it is the expected one 
        if (block.getNHeight() == expectedHeight) {
          // This first validates the block and throws if it is invalid.
          this->state_.processNextBlock(std::move(block));
          rebroadcast = true;
        } else if (block.getNHeight() >= expectedHeight - 2) {
          // If the block is at least latest()->getNHeight() - 1, then we should still rebroadcast it
          rebroadcast = true;
        }
      }
    } catch (std::exception const& ex) {
      throw DynamicException("Invalid blockBroadcast (" + std::string(ex.what()) + ")");
    }
    if (rebroadcast) this->broadcastMessage(message, nodeId);
  }

  void Broadcaster::handleBroadcast(
    const NodeID &nodeId, const std::shared_ptr<const Message>& message
  ) {
    switch (message->command()) {
      case BroadcastValidatorTx:
        handleTxValidatorBroadcast(nodeId, message);
        break;
      case BroadcastTx:
        handleTxBroadcast(nodeId, message);
        break;
      case BroadcastBlock:
        handleBlockBroadcast(nodeId, message);
        break;
      default:
        throw DynamicException("Invalid Broadcast Command Type: " + std::to_string(message->command()));
        break;
    }
  }

  void Broadcaster::broadcastTxValidator(const TxValidator& tx) {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastValidatorTx(tx));
    this->broadcastMessage(broadcast, {});
  }

  void Broadcaster::broadcastTxBlock(const TxBlock& txBlock) {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastTx(txBlock));
    this->broadcastMessage(broadcast, {});
  }

  void Broadcaster::broadcastBlock(const std::shared_ptr<const FinalizedBlock>& block) {
    auto broadcast = std::make_shared<const Message>(BroadcastEncoder::broadcastBlock(block));
    this->broadcastMessage(broadcast, {});
  }

}