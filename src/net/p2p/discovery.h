/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DISCOVERYWORKER_H
#define DISCOVERYWORKER_H

#include <atomic>
#include <future>
#include <unordered_set>
#include "session.h"

namespace P2P {
  // Forward declarations.
  class ManagerBase;

  /**
   * Worker class for the Discovery manager, as a separate thread.
   * Responsible for the process of actually discovering other nodes.
   */
  class DiscoveryWorker : public Log::LogicalLocationProvider {
  private:
    /// Reference to the parent connection manager.
    ManagerBase& manager_;

    /// Flag for stopping the thread.
    std::atomic<bool> stopWorker_ = false;

    /**
     * Future object for the worker thread.
     * This is checked for validity (running) to determine if the thread is running,
     * and by wait(), to wait until thread is finished.
     */
    std::future<bool> workerFuture_;

    /// Map for previously requested nodes (Node ID -> time of last request).
    std::unordered_map<NodeID, uint64_t, SafeHash> requestedNodes_;

    /// Mutex for managing read/write access to requestedNodes.
    std::shared_mutex requestedNodesMutex_;

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
        std::unordered_set<NodeID, SafeHash>,std::unordered_set<NodeID, SafeHash>
    > listConnectedNodes();

    /**
     * Get a connected node from the list.
     * @param nodeId The node ID to search for.
     * @return A map with matching node IDs and their corresponding type, IP and port.
     */
    std::unordered_map<NodeID, NodeType, SafeHash> getConnectedNodes(const NodeID& nodeId);

    /**
     * Connect to a node (checks if not connected already).
     * @param nodeId The unique ID of the node for reference.
     * @param nodeType The type of the node (Discovery or Normal).
     */
    void connectToNode(const NodeID& nodeId, NodeType nodeType);

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
    explicit DiscoveryWorker(ManagerBase& manager) : manager_(manager) {}

    /// Destructor. Automatically stops the worker thread.
    ~DiscoveryWorker() { this->stop(); }

    virtual std::string getLogicalLocation() const; ///< Log instance from P2P

    /// Start the discovery thread.
    void start();

    /// Stop the discovery thread and wait until it is finished.
    void stop();
  };
};

#endif // DISCOVERYWORKER_H
