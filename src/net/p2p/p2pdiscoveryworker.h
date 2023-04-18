#ifndef DISCOVERYWORKER_H
#define DISCOVERYWORKER_H

#include <atomic>
#include <future>
#include "p2pbase.h"

namespace P2P {
  // Forward declarations.
  class ManagerBase;

  /**
   * Worker class for the Discovery manager, as a separate thread.
   * Responsible for the process of actually discovering other nodes.
   */
  class DiscoveryWorker {
    private:
      /// Reference to the parent connection manager.
      ManagerBase& manager;

      /// Flag for stopping the thread.
      std::atomic<bool> stopWorker = false;

      /**
       * Future object for the worker thread.
       * This is checked for validity (running) to determine if the thread is running,
       * and by wait(), to wait until thread is finished.
       */
      std::future<bool> workerFuture;

      /// Map for previously requested nodes (Node ID -> time of last request).
      std::unordered_map<Hash, uint64_t, SafeHash> requestedNodes;

      /// Mutex for managing read/write access to requestedNodes.
      std::shared_mutex requestedNodesMutex;

      /**
       * Refresh the list of previously requested nodes.
       * Removes the nodes that were requested more than 60 seconds ago.
       */
      void refreshRequestedNodes();

      /**
       * List the currently connected nodes.
       * @return A pair with two sets of IDs - the first for Discovery nodes, the second for Normal nodes.
       */
      std::pair<
        std::unordered_set<Hash, SafeHash>,std::unordered_set<Hash, SafeHash>
      > listConnectedNodes();

      /**
       * Get a connected node from the list.
       * @param nodeId The node ID to search for.
       * @return A map with matching node IDs and their corresponding type, IP and port.
       */
      std::unordered_map<
        Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash
      > getConnectedNodes(const Hash& nodeId);

      /**
       * Connect to a node (checks if not connected already).
       * @param nodeId The unique ID of the node for reference.
       * @param nodeInfo Info about the node (type, IP, port).
       */
      void connectToNode(
        const Hash& nodeId, const std::tuple<NodeType, boost::asio::ip::address,
        unsigned short>& nodeInfo
      );

      /**
       * Entry point for the discovery process thread.
       *
       * We can summarize it like this:
       * - Ask currently connected nodes to give us a list of nodes they are connected to.
       * - Wait up to 5 seconds for looping the connected nodes.
       * - If already asked in the last 60 seconds, skip the node.
       * - Give priority to discovery nodes at first pass.
       * - Do not connect to nodes that are already connected.
       * - Connect to nodes that are not already connected.
       * - If number of connections is over maxConnections, stop discovery.
       * - As discovery nodes should be *hardcoded*, we cannot connect to other discovery nodes.
       *
       * @return `true` when the thread is forced to stop.
       */
      bool discoverLoop();

    public:
      /**
       * Constructor.
       * @param manager Reference to the parent connection manager.
       */
      DiscoveryWorker(ManagerBase& manager) : manager(manager) {}

      /// Destructor. Automatically stops the worker thread.
      ~DiscoveryWorker() { this->stop(); }

      /// Start the discovery thread.
      void start();

      /// Stop the discovery thread and wait until it is finished.
      void stop();
  };
};

#endif // DISCOVERYWORKER_H
