#ifndef DISCOVERYWORKER_H
#define DISCOVERYWORKER_H

#include <atomic>
#include <future>

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
