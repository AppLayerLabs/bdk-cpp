#ifndef DISCOVERYWORKER_H
#define DISCOVERYWORKER_H

#include <atomic>
#include <future>
#include "p2pbase.h"

namespace P2P {
  /// Forward declaration
  class ManagerBase;
  class DiscoveryWorker {
  private:
    /// Reference back to the Manager Object
    ManagerBase& manager;

    /// Atomic bool for stopping the thread
    std::atomic<bool> stopWorker = false;

    /// Future object for the worker thread.
    /// This is either checked for valid (running) to determine if the thread is running
    /// And wait(), to waiting until thread is finished
    std::future<bool> workerFuture;

    /// Mutex for requestedNodes
    std::shared_mutex requestedNodesMutex;

    /**
     * Unordered_map of previously requested nodes
     * Key: Node ID
     * Value: time of last request
     */
     std::unordered_map<Hash, uint64_t, SafeHash> requestedNodes;

    /**
     * Refreshes the list of previously requested nodes
     * Remove the nodes that were requested more than 60 seconds ago
     */

    void refreshRequestedNodes();
    /**
     * List current connected nodes
     * @return first: discovery nodes IDs, second: normal nodes IDs
     */
    std::pair<std::unordered_set<Hash, SafeHash>,std::unordered_set<Hash, SafeHash>> listConnectedNodes();

    /**
     * Get nodes from connected nodes.
     * @return a map of node IDs and their corresponding node type, IP and port
     */

    std::unordered_map<Hash, std::tuple<NodeType, boost::asio::ip::address, unsigned short>, SafeHash> getConnectedNodes(const Hash& nodeId);

    /**
     * Connects to a node
     * Check if the node is already connected
     * @param nodeId: Node ID
     */

    void connectToNode(const Hash& nodeId, const std::tuple<NodeType, boost::asio::ip::address, unsigned short>& nodeInfo);
    /**
     * Entry Function for the discovery thread.
     */
    bool discoverLoop();

  public:

    /// Constructor
    DiscoveryWorker(ManagerBase& manager) : manager(manager) {}

    /// Destructor
    ~DiscoveryWorker() { this->stop(); }

    /// Starts the discovery thread
    void start();

    /// Stops the discovery thread
    /// Waits until the thread is finished
    void stop();

  };
};



#endif /// DISCOVERYWORKER_H
