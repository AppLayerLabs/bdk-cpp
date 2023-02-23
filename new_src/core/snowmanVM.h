#ifndef SNOWMANVM_H
#define SNOWMANVM_H

#include <mutex>
#include <string>
#include <vector>

#include "storage.h"
#include "../net/grpcclient.h"
#include "../net/grpcserver.h"
#include "../utils/block.h"
#include "../utils/db.h"
#include "../utils/utils.h"

using grpc::ServerContext;

/**
 * Internal struct that contains data for initializing the SnowmanVM.
 * TODO: Ava Labs never answered us on what some of those do.
 */
struct InitializeRequest {
  uint32_t networkId; ///< ID of the network to connect to.
  std::string subnetId; ///< ID of the subnet to connect to.
  std::string chainId; ///< ID of the chain to connect to.
  std::string nodeId; // ID of the node to connect to.
  std::string xChainId; ///< See todo.
  std::string avaxAssetId; ///< See todo.
  std::string genesisBytes; ///< See todo.
  std::string upgradeBytes; ///< See todo.
  std::string configBytes; ///< See todo.
  std::vector<DBServer> dbServers; ///< Array of database server to connect to.
  std::string gRPCServerAddress; ///< gRPC server address to connect to.
};

/**
 * Abstraction of AvalancheGo's SnowmanVM protocol.
 * See [Ava Labs' docs](https://github.com/ava-labs/avalanchego/tree/master/vms#readme) for more details.
 */
class SnowmanVM {
  private:
    /// Struct with initialization request data from AvalancheGo.
    InitializeRequest initParams;

    /// List of all connected nodes.
    std::vector<std::string> connectedNodes;

    /// Mutex for managing read/write access to connectedNodes.
    std::mutex connectedNodesLock;

    /// Preferred block hash. Set by SetPreference from gRPC.
    Hash preferredBlockHash;

    /// Mempool for blocks from AvalancheGo's network. Lookup is made by block hash.
    std::unordered_map<Hash, std::shared_ptr<const Block>, SafeHash> mempool;

    /// Cached block status. Lookup is made by block hash.
    std::unordered_map<Hash, BlockStatus, SafeHash> cachedBlockStatus;

    /// Mutex for managing read/write access to the SnowmanVM object.
    mutable std::mutex lock;

    /// Pointer to the blockchain history.
    const std::shared_ptr<Storage> storage;

    /// Pointer to the gRPC server.
    const std::shared_ptr<gRPCServer> grpcServer;

    /// Pointer to the gRPC client.
    const std::shared_ptr<gRPCClient> grpcClient;

  public:
    /// Enum for block status.
    enum BlockStatus { Unknown, Processing, Rejected, Accepted };

    /**
     * Constructor.
     * @param storage Pointer to the blockchain history.
     * @param grpcServer Pointer to the gRPC server.
     * @param grpcClient Pointer to the gRPC client.
     */
    SnowmanVM(
      const std::shared_ptr<Storage>& storage,
      const std::shared_ptr<gRPCServer>& grpcServer,
      const std::shared_ptr<gRPCClient>& grpcClient
    ) : storage(storage), grpcServer(grpcServer), grpcClient(grpcClient) {}

    /// Getter for `preferredBlockHash`.
    const Hash& getPreferredBlockHash() { return this->preferredBlockHash; }

    /// Setter for `preferredBlockHash`.
    void setPreferredBlockHash(const Hash& hash) { this->preferredBlockHash = hash; }

    /**
     * Initialize the SnowmanVM services.
     * Called by gRPCServer.
     * The initialization request is made by the AvalancheGo Daemon.
     * See vm.proto for more information.
     * Throws if called when the services are already initialized.
     */
    void initialize(
      const vm::InitializeRequest* request, vm::InitializeResponse* reply
    );

    /**
     * Parse a given block and push it to the blockchain if required.
     * Called by gRPCClient.
     * @return `true` if the block was parsed successfully, `false` otherwise.
     */
    bool parseBlock(
      ServerContext* context, const std::string& blockBytes, vm::ParseBlockResponse* reply
    );

    /**
     * Set the state of the SnowmanVM.
     * Called by `initialize()` if no info is found on the database.
     * For more info about the SetState request, see vm.proto and
     * https://github.com/ava-labs/avalanchego/blob/master/snow/engine/snowman/bootstrap/bootstrapper.go#L111
     */
    void setState(const vm::SetStateRequest* request, vm::SetStateResponse* reply);

    /**
     * Request a block to be created.
     * Called by gRPCServer.
     * @return `true` if request is successful, `false` otherwise.
     */
    bool blockRequest(ServerContext* context, vm::BuildBlockResponse* reply);

    /**
     * Get a block that was requested.
     * Called by gRPCServer.
     */
    void getBlock(
      ServerContext* context, const vm::GetBlockRequest* request, vm::GetBlockResponse* reply
    );

    /**
     * Get the ancestors of a block.
     * Called by gRPCServer.
     * @return `true` on success, `false` on failure.
     */
    bool getAncestors(
      ServerContext* context, const vm::GetAncestorsRequest* request, vm::GetAncestorsResponse* reply
    );

    /**
     * Set the preferred block for acceptance/continuing the chain.
     * Called by gRPCServer.
     */
    void setPreference(ServerContext* context, const vm::SetPreferenceRequest* request);

    /**
     * Get a block's status in the mempool.
     * @param hash The block hash to get the status from.
     * @return The current block's status.
     */
    const BlockStatus getBlockStatus(const Hash& hash);

    /**
     * Set a block's status in the mempool.
     * @param hash The block hash to set the status to.
     * @param status The status to set.
     */
    void setBlockStatus(const Hash& hash, const BlockStatus& status);

    /**
     * Request a block to be verified.
     * Called by gRPCServer.
     * @param bytes The raw block bytes.
     * @return A pointer to the block, or `nullptr` in case of error.
     */
    const std::shared_ptr<const Block> verifyBlock(const std::string bytes);

    /**
     * Accept a block.
     * Called by gRPCServer.
     * @param hash The block hash to accept.
     * @return `true` if block was accepted, `false` otherwise.
     */
    bool acceptBlock(const Hash& hash);

    /**
     * Reject a block.
     * Called by gRPCServer.
     * @param hash The block hash to reject.
     */
    void rejectBlock(const Hash& hash);

    /**
     * Check if a block exists in the mempool.
     * @param hash The block hash to check.
     * @return `true` if the block exists, `false` otherwise.
     */
    bool blockExists(const Hash& hash);

    /**
     * Check if a block has the "Processing" status (no consensus reached yet).
     * @param hash The block hash to check.
     * @return `true` if block is processing, `false` otherwise.
     */
    bool blockIsProcessing(const Hash& hash);

    /**
     * Get a block from the mempool by its hash.
     * @param hash The block hash to get.
     * @return The found block, or `nullptr` if block is not found.
     */
    const std::shared_ptr<const Block> getBlock(const Hash& hash);

    /**
     * Connect to a given node.
     * @param id The node's 20-byte hex ID.
     */
    void connectNode(const std::string& id);

    /**
     * Disconnect from a given node.
     * @param id The node's 20-byte hex ID.
     */
    void disconnectNode(const std::string& id);
};

#endif  // SNOWMANVM_H
