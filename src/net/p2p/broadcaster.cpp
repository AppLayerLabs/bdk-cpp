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
    try {
      auto block = BroadcastDecoder::broadcastBlock(*message, getOptions().getChainID());
      // If we already have the block, then this message is guaranteed irrelevant
      if (!this->storage_.blockExists(block.getHash())) {
        // We don't have it, so check if there's a chance it will connect to our blockchain (current height + 1)
        if (block.getNHeight() == this->storage_.latest()->getNHeight() + 1) {
          // Block seems to have the expected height for the next block, so try to connect it
          BlockValidationStatus vStatus = this->state_.tryProcessNextBlock(std::move(block));
          switch (vStatus) {
            case BlockValidationStatus::valid:
              // Connected block successfully; rebroadcast it
              this->broadcastMessage(message, nodeId);
              break;
            case BlockValidationStatus::invalidWrongHeight:
              // processNextBlock() might fail to validate the block if there is a race in incoming
              //   block broadcasts, resulting in an attempt to connect the block at a wrong height.
              // In that case, the block won't be rebroadcast by this thread. There will be one winning
              //   thread that will pass tryProcessNextBlock() with 'valid' and broadcast the block.
              // We don't want to throw an exception in this case because that would close the session.
              break;
            default:
              // Block contains bad data
              throw DynamicException("Erroneous block data");
              break;
          }
        }
      }
    } catch (std::exception const& ex) {
      throw DynamicException("Invalid blockBroadcast (" + std::string(ex.what()) + ")");
    }
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