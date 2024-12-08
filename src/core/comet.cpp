/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "comet.h"

#include "../utils/logger.h"
#include "../utils/safehash.h"
#include "../libs/toml.hpp"

#include "../net/abci/abciserver.h"
#include "../net/abci/abcihandler.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>

#include "../libs/base64.hpp"

/*
  NOTE: cometbft public key vs. cometbft address

  There is only one public key type and it is hard-coded in the Comet driver.
  The cometbft genesis.json file should have "tendermint/PubKeyEd25519" as the only public key type,
  whose equivalent in ABCI parlance is the "ed25519" string below.

  Address
  Address is a type alias of a slice of bytes. The address is calculated by hashing the public key
  using sha256 and truncating it to only use the first 20 bytes of the slice.

  const (
    TruncatedSize = 20
  )

  func SumTruncated(bz []byte) []byte {
    hash := sha256.Sum256(bz)
    return hash[:TruncatedSize]
  }
*/
#define COMET_PUB_KEY_TYPE "ed25519"

// ---------------------------------------------------------------------------------------
// WebsocketRPCConnection class
// ---------------------------------------------------------------------------------------

/**
 * Create a compliant JSON-RPC object with the given error message.
 * @param message The error message.
 */
json rpcMakeInternalError(const std::string& message) {
  return {
    {"jsonrpc", "2.0"},
    {"error",
      {
        {"code", 1000},
        {"message", message}
      }
    },
    {"id", nullptr}
  };
}

/**
 * One reusable websocket JSON-RPC connection to a cometbft process. Keeps track of JSON-RPC requests and responses.
 * NOTE: This class is thread-safe.
 * NOTE: This uses the same io_context for multiple RPC connections; as usual with ASIO, this can cause (i) completion handler
 * cancellation from the previous connection to overlap with the new connection, and (ii) pending completion handlers to revive
 * in the new connection. For (i) we just drop the only handler that matters -- the read handler -- when the operation is
 * cancelled, since the new connection will post a new one. For (ii) we do not care at all since it's the same connection with
 * the same process always, and we can safely assume that the handlers that matter -- calls to 'cometbft start' -- will always
 * land in the same process and run mode -- 'cometbft start'. Calls to 'cometbft inspect' should all be synchronous anyway
 * (since that's a diagnostic mode) and won't result in cancelled/leftover handlers upon close of the inspect RPC conection.
 * NOTE: Even (2,4,6,..) JSON-RPC request IDs are used for async RPC calls, and odd IDs (1,3,5,..) are used for sync calls.
 * This allows the RPC response read handler to know whether it should place the response in rpcAsyncResponseMap_ or
 * rpcSyncResponseMap_, which is needed because sync responses don't pair with rpcAsyncSent_ elements. Splitting the request ID
 * space at say 2^64/2 would just generate large IDs that are unreadable and use more space in plaintext.
 * @tparam T Type of the custom data element stored with every in-flight JSON-RPC request (a std::variant is useful here).
 *
 * FIXME: Need to fix utils/logger.h instance logging macros (LOGxxx) to work in template classes like WebsocketRPCConnection.
 */
template <typename T>
class WebsocketRPCConnection : public Log::LogicalLocationProvider {
  protected:
    const std::string instanceIdStr_; ///< Identifier for logging
    std::atomic<int> rpcServerPort_ = 0; ///< Configured localhost RPC port to connect to.
    std::atomic<uint64_t> rpcRequestIdCounter_ = 0; ///< JSON-RPC request ID generator.

    std::mutex rpcStopMutex_; ///< Mutex for rpcStop_
    bool rpcStop_ = false; ///< Flag used to temporarily hold RPC calls so pending request responses can be flushed before stopping/rebooting the connection

    std::mutex rpcStateMutex_; ///< Mutex that serializes all method calls involving communication.
    bool rpcRunning_ = false; ///< Flag to manage the RPC connection lifecycle (access only while holding rpcStateMutex_).
    std::atomic<bool> rpcFailed_ = false; ///< Error flag raised by failures in I/O handlers (must be atomic since it is set in the read handler).

    std::unique_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> rpcWs_; ///< RPC websocket connection (guarantees async_read whole messages).
    boost::asio::io_context rpcIoc_; ///< io_context for the websocket connection.
    boost::asio::strand<boost::asio::io_context::executor_type> rpcStrand_{rpcIoc_.get_executor()}; ///< Strand to serialize read/write operations.
    std::thread rpcThread_; ///< Thread for rpcIoc_.run() (async RPC responses read loop).
    boost::beast::flat_buffer rpcReadBuffer_; ///< Buffer for reading all RPC responses (serialized/protected by rpcStrand_), retains size of largest message.

    std::mutex rpcAsyncSentMutex_; ///< mutex to protect rpcAsyncSent_.
    std::map<uint64_t, T> rpcAsyncSent_; ///< Map of JSON-RPC request id to application object that models the request.
    std::mutex rpcAsyncResponseMapMutex_; ///< Mutex to protect all access to rpcAsyncResponseMap_.
    std::unordered_map<uint64_t, json> rpcAsyncResponseMap_; ///< Map of request ID to response object, if any received, for rpc async calls

    std::mutex rpcSyncResponseMapMutex_; ///< Mutex to protect all access to rpcAsyncResponseMap_.
    std::unordered_map<uint64_t, json> rpcSyncResponseMap_; ///< Map of request ID to response object, if any received, for rpc sync calls

    /**
     * Helper to set a JSON-RPC response to the correct map.
     * @param requestId ID of the request (even=async, odd=sync).
     * @param response The JSON-RPC response to the request.
     */
    void rpcSetResponse(const uint64_t requestId, const json& response);

    /**
     * Schedule a read task.
     * NOTE: The caller must have locked rpcStateMutex_.
     */
    void rpcAsyncRead();

    /**
     * Handle a read message and schedule the next read task if no error.
     */
    void rpcHandleAsyncRead(boost::system::error_code ec, std::size_t);

    /**
     * Cancel all rpcAsyncCall() JSON-RPC requests in flight by assigning them
     * a JSON-RPC response with the given error message.
     * @param errorMessage The JSON-RPC error message to use for the cancelled RPCs.
     */
    void rpcCancelAllAsyncRequests(const std::string& errorMessage);

    /**
     * Start the persisted rpcSocket_ connection to the cometbft process_.
     * NOTE: The caller must have locked mutex.
     * @param port Localhost TCP listen port to connect to (-1 to use previously given port number).
     * @return `true` if the connection was established or is already established, `false` if failed to
     * connect or can't connect (e.g. RPC port number is not yet known).
     */
    bool rpcDoStart(int rpcPort = -1);

    /**
     * Stop the websocket connection to the cometbft process_ (if there is one).
     * NOTE: The caller must have locked rpcStateMutex_.
     */
    void rpcDoStop();

    /**
     * Actually stops the websocket (called by rpcDoStop() and rpcDoStart()).
     * NOTE: The caller must have locked rpcStateMutex_.
     */
    void rpcDoReset();

    /**
     * Back-end to rpcAsyncCall() and rpcSyncCall(). Generates an odd or even JSON-RPC request ID depending
     * on what the caller is (sync vs async).
     * NOTE: The caller must have locked rpcStateMutex_.
     * @param method RPC method name to call.
     * @param params Parameters to pass to the RPC method.
     * @param requestData If an async RPC call, the application data that is needed to generate the json
     * rpc request call and that the application may want to return to its own client layer in its own
     * RPC call-result callback (*must not* be T{}). If a sync call, this *must be* T{}.
     * @return The requestId (can be passed to rpcGetSyncResponse() later) if successful, or 0 on error.
     */
    uint64_t rpcDoCall(const std::string& method, const json& params, const T& requestData);

  public:

    std::string getLogicalLocation() const override { return instanceIdStr_; } ///< Log instance

    WebsocketRPCConnection(std::string instanceIdStr) : instanceIdStr_(instanceIdStr) {} ///< Constructor

    /**
     * Start the persisted rpcSocket_ connection to the cometbft process_.
     * @param port Localhost TCP listen port to connect to (-1 to use previously given port number).
     * @return `true` if the connection was established or is already established, `false` if failed to
     * connect or can't connect (e.g. RPC port number is not yet known).
     */
    bool rpcStartConnection(int rpcPort = -1);

    /**
     * Stop the websocket connection to the cometbft process_ (if there is one).
     * NOTE: Will invoke rpcConnectionResetCallback_() if the connection was previously open.
     */
    void rpcStopConnection();

    /**
     * Check if the RPC connection is alive and not in a failed state.
     * @return `true` if the connection is alive and not flagged as failed, `false` otherwise.
     */
    bool rpcCheckConnection();

    /**
     * Pops the response to a specific sync request by requestId, if already available.
     * NOTE: The T& outRequestData outparam is missing because this is only used in rpcSyncCall().
     * @param requestId ID of the request.
     * @param outResult Outparam set to the json response to the request.
     * @return `true` if there was a response to read, `false` otherwise (outResult is unset).
     */
    bool rpcGetSyncResponse(const uint64_t requestId, json& outResult);

    /**
     * Pops a response to an async request, if any is available.
     * @param outResult Outparam set to the json response to the request.
     * @param outRequestData Outparam set to the client request data object passed into the original RPC call request.
     * @return ID of the request, or 0 if there weren't any responses stored (outparams are unmodified).
     */
    uint64_t rpcGetNextAsyncResponse(json& outResponse, T& outRequestData);

    /**
     * Make an asynchronous (non-blocking) JSON-RPC call.
     * NOTE: If this method returns `0`, you *MUST* folow up with a call to `rpcStopConnection()` at some
     * point, since this method cannot stop the connection on its own (otherwise it would invoke the
     * rpcConnectionResetCallback_(), which might impede your implementation of that callback from acquiring
     * a mutex that you happen to need to register RPC request IDs that are pending.
     * @param method RPC method name to call.
     * @param params Parameters to pass to the RPC method.
     * @param requestData Application data that is needed to generate the json rpc request call and that
     * the application may want to return to its own client layer in its own RPC call-result callback.
     * @return The requestId (can be passed to rpcGetSyncResponse() later) if successful, or 0 on error.
     */
    uint64_t rpcAsyncCall(const std::string& method, const json& params, const T& requestData);

    /**
     * Make a synchronous (blocking) RPC call.
     * Should use only when inspecting the cometbft node, otherwise should always use rpcAsyncCall() instead.
     * NOTE: SLOW, polls with sleep, locks the entire RPC engine while waiting for its response.
     * @param method RPC method name to call.
     * @param params Parameters to pass to the RPC method.
     * @param outResult Outparam set to the json response to the request.
     * @return `true` if call is successful, `false` on any error (e.g. connection fails while waiting).
     */
    bool rpcSyncCall(const std::string& method, const json& params, json& outResult);
};

template <typename T>
void WebsocketRPCConnection<T>::rpcSetResponse(const uint64_t requestId, const json& response) {
  if (requestId % 2 == 0) {
    std::scoped_lock lock(rpcAsyncResponseMapMutex_);
    rpcAsyncResponseMap_[requestId] = response;
  } else {
    std::scoped_lock lock(rpcSyncResponseMapMutex_);
    rpcSyncResponseMap_[requestId] = response;
  }
}

template <typename T>
void WebsocketRPCConnection<T>::rpcAsyncRead() {
  // - Caller *must* be holding the rpcStateMutex_ here.
  // - Checking !rpcRunning_ *must* substitute for checking !rpcWs_
  // - Do NOT return if rpcFailed_ == true here because the entire point of the rpcStop_ feature is for us to
  //   be able to pump messages out of an otherwise valid connection *before* we close it (i.e. before we make
  //   rpcRunning_ = false -- i.e. actually closing the connection -- by reacting to rpcFailed == true).
  if (!rpcRunning_) {
    // async_read without checking that we actually have an rpcWs_ / connection here will cause:
    // Assertion `! impl.rd_close' failed.
    return;
  }
  // Post handler for the next read of a complete message (websocket/JSON-RPC).
  rpcWs_->async_read(
    rpcReadBuffer_,
    boost::asio::bind_executor(
      rpcStrand_,
      std::bind(&WebsocketRPCConnection::rpcHandleAsyncRead, this, std::placeholders::_1, std::placeholders::_2)
    )
  );
}

template <typename T>
void WebsocketRPCConnection<T>::rpcHandleAsyncRead(boost::system::error_code ec, std::size_t) {
  // If the handler knows it is cancelled, then just skip it
  if (ec == boost::asio::error::operation_aborted) {
    SLOGXTRACE("Read operation aborted; ignoring.");
    // Don't repost the handler; if the operation is cancelled, it means one connection
    // stopped, and when it is revived in a new connection, another read task will be posted.
    // This should work better now that we wait a bit for RPC tasks (rpcStop_ feature) before we
    // will issue an rpcIoc_.stop(), which is what can cause an aborted read handler, which causes
    // us to stop reading any potential RPC requests that could follow (and since we do wait now,
    // the odds that we will get everything are increased).
    return;
  } else if (ec) {
    SLOGXTRACE("RPC failed (ec): " + ec.message());
    // In case of connection failures, flag it so that code outside this handler can read the flag and
    // stop the RPC connection.
    rpcFailed_ = true;
    // Any network error likely means there's no RPC messages to read anyway (the connection should be dead)
    // so we don't need to post a new read handler in any case.
    return;
  } else try {
    // Get a const char* and size_t to the current first byte of the flat_buffer
    // (fully reading and processing one RPC response at a time, so message is always at the start)
    auto bufData = rpcReadBuffer_.data();
    auto itBuf = boost::asio::buffer_sequence_begin(bufData);
    const char* dataPtr = static_cast<const char*>(itBuf->data());
    std::size_t dataSize = itBuf->size();
    // Parse the JSON directly from the (const char*, size_t) memory area owned by rpcReadBuffer_
    auto jsonResponse = json::parse(std::string_view(dataPtr, dataSize));
    // Discard the data in rpcReadBuffer_ after use
    rpcReadBuffer_.consume(dataSize);
    // Process the JSON-RPC response
    if (jsonResponse.contains("id")) {
      uint64_t requestId = jsonResponse["id"];
      // If you have a response (rpcAsyncResponseMap_ entry) then you necessarily have a T in rpcAsyncSent_.
      // So if we just got a response that doesn't have a matching rpcAsyncSent_, when we just DROP it:
      // it means the user already collected another response in its place -- probably a
      // rpcMakeInternalError("RPC_CONNECTION_CLOSED").
      // (could be a stale response from a previous connection cycle, for  example, since we
      // aren't currently preventing that)
      bool keepRequest = true;
      if (requestId % 2 == 0) {
        // rpcAsync only
        std::scoped_lock lock(rpcAsyncSentMutex_);
        auto it = rpcAsyncSent_.find(requestId);
        if (it == rpcAsyncSent_.end()) {
          keepRequest = false;
          SLOGDEBUG("RPC dropping orphan response to request ID = " + std::to_string(requestId) + " : " + jsonResponse.dump());
        }
      }
      // REVIEW: Protect against storing stale sync responses as well.
      // Keeping eventual stale sync responses is mostly harmless:
      // - Sync is just used for testing cometbft, not in the running state.
      // - Should be actually pretty rare for the RPC localhost connection to be continually reset and for that to cause aborted RPC calls.
      // - rpcGetSyncResponse() does not loop over a structure like rpcGetNextAsyncResponse() does so it doesn't lose any time anyways.
      // Would require a rpcSyncSent_ structure that's just a set of uint64_t.
      if (keepRequest) {
        rpcSetResponse(requestId, jsonResponse);
      }
      // Continue to post next read handler
    } else {
      SLOGDEBUG("RPC failed (bad JSON-RPC with no 'id'): " + jsonResponse.dump());
      rpcFailed_ = true;
      // Continue to post next read handler
      // A message format failure (which is unlikely) would not necessarily mean a failure of the connection itself
    }
  } catch (const std::exception& ex) {
    SLOGDEBUG("RPC failed (exception): " + std::string(ex.what()));
    rpcFailed_ = true;
    // Continue to post next read handler
    // This could be a json parse error. Probably nothing else.
    // A message format failure (which is unlikely) would not necessarily mean a failure of the connection itself
  }
  // Post the next read handler
  std::scoped_lock stateLock(rpcStateMutex_);
  rpcAsyncRead();
}

template <typename T>
void WebsocketRPCConnection<T>::rpcCancelAllAsyncRequests(const std::string& errorMessage) {
  // Whenever the websocket RPC connection is reset, the RPC request IDs that
  //   we have in flight become meaningless, since they won't be answered in
  //   a new connection that we happen to make (or we don't expect them to be
  //   answered--in any case, we won't ever be reusing these request IDs as
  //   the local JSON-RPC request ID generator is not reset to 1).
  // So we need to iterate over every txSent_ entry we have with a requestID
  //   waiting for an RPC response and generate local failure responses.
  //
  // NOTE: sync calls hold an exclusive mutex over the entire RPC engine, so
  // RPC failures can be directly addressed inside rpcSyncCall() instead.
  json rpcClosedResponse = rpcMakeInternalError(errorMessage);
  // The rpcAsyncSent_ map is what gets filled in with pending requestIDs, so
  // we need to collect all those IDs
  std::vector<uint64_t> requestIds;
  requestIds.reserve(rpcAsyncSent_.size());
  std::unique_lock<std::mutex> sentLock(rpcAsyncSentMutex_);
  for (const auto& kv : rpcAsyncSent_) {
    const uint64_t& requestId = kv.first;
    requestIds.push_back(requestId);
  }
  // Don't need and should not erase rpcAsyncSent_ elements here because the user has
  // to "manually" collect both the rpcAsyncSent_ and the rpcAsyncResponseMap_ entries now
  // via some rpcGetXX() method.
  sentLock.unlock();
  // Manufacture erroneous JSON-RPC responses for all requests that were pending
  std::scoped_lock reqLock(rpcAsyncResponseMapMutex_);
  for (uint64_t requestId : requestIds) {
    rpcAsyncResponseMap_[requestId] = rpcClosedResponse;
  }
}

template <typename T>
bool WebsocketRPCConnection<T>::rpcDoStart(int rpcPort) {
  if (rpcRunning_) {
    return true;
  }
  if (rpcPort >= 0) {
    rpcServerPort_ = rpcPort; // If the rpcPort parameter is set (that is, not -1) then save it
  }
  if (rpcServerPort_ == 0) {
    return false; // CometImpl has not figured out what the RPC port is yet
  }
  try {
    rpcFailed_ = false;
    rpcRunning_ = true; // Set this first to ensure rpcDoReset() in catch() below will work.
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address("127.0.0.1"), rpcServerPort_);
    rpcWs_ = std::make_unique<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>(rpcIoc_);
    rpcWs_->next_layer().connect(endpoint);
    rpcWs_->handshake("127.0.0.1", "/websocket");
    rpcAsyncRead(); // Post the first async_read operation
    rpcThread_ = std::thread([this]() {
      try {
        rpcIoc_.run(); // For async read of RPC responses into the request map
      } catch (const std::exception& ex) {
        SLOGXTRACE("ERROR: Exception in rpcThread_: " + std::string(ex.what()));
        rpcFailed_ = true; // The connection is actually busted/disabled (no ioc.run()) and needs to be restarted
      }
    });
    return true;
  } catch (const std::exception& ex) {
    SLOGXTRACE("ERROR: websocket connection failed: " + std::string(ex.what()));
    rpcDoReset();
    return false;
  }
}

template <typename T>
void WebsocketRPCConnection<T>::rpcDoStop() {
  rpcDoReset();
  rpcCancelAllAsyncRequests("RPC_CONNECTION_CLOSED");
}

template <typename T>
void WebsocketRPCConnection<T>::rpcDoReset() {
  if (!rpcRunning_) {
    return;
  }
  rpcRunning_ = false; // Set this first so that completion handlers can avoid calling rpcDoStop() again
  if (rpcWs_) {
    boost::system::error_code ec;
    if (rpcWs_) {
      rpcWs_->close(boost::beast::websocket::close_code::normal, ec);
      rpcWs_.reset();
    }
  }
  rpcIoc_.stop();
  if (rpcThread_.joinable()) {
    rpcThread_.join();
  }
  rpcIoc_.restart();
}

template <typename T>
uint64_t WebsocketRPCConnection<T>::rpcDoCall(const std::string& method, const json& params, const T& requestData) {
  // Maybe this shouldn't be here -- potentially unnecessary, but otherwise it's guaranteed harmless
  // (the connection was flagged as failed for whatever reason, meaning we should positively not be
  // making RPC calls -- an RPC call request, sync or async, in a failed connection must itself fail).
  if (rpcFailed_) {
    return 0;
  }
  // We are holding the state lock so we can check these
  if (!rpcRunning_) {
    // If we are not connected at this point for whatever reason then obviously this call
    // can't succeed, no point in posting the write handler (even if it would succeed later,
    // that would be potentially worse, not better).
    return 0;
  }

  // Protected by rpcStateMutex_ (necessarily acquired by caller)
  // Increment twice if a single increment lands on even ID for a sync call (T{}) or odd ID for an async call (not T{})
  bool isSyncCall = requestData == T{};
  ++rpcRequestIdCounter_;
  bool gotEvenId = (rpcRequestIdCounter_ % 2) == 0;
  rpcRequestIdCounter_ += (gotEvenId == isSyncCall); // true == 1, false == 0; increment on mismatch
  uint64_t requestId = rpcRequestIdCounter_;

  // Build the JSON-RPC request message
  json requestBody = {
    {"jsonrpc", "2.0"},
    {"method", method},
    {"params", params},
    {"id", requestId}
  };
  auto message = std::make_shared<std::string>(requestBody.dump());

  // rpcAsyncSent_ is used by async calls only
  if (requestData != T{}) {
    std::scoped_lock lock(rpcAsyncSentMutex_);
    rpcAsyncSent_[requestId] = requestData;
  }

  boost::asio::post(rpcStrand_, [this, requestId, message]() {
    boost::system::error_code ec;
    std::unique_lock<std::mutex> stateLock(rpcStateMutex_);
    if (!rpcRunning_ && !rpcDoStart()) { // rpcRunning_ == true *must* mean that rpcWs_ is set (while holding rpcStateMutex_)
      rpcSetResponse(requestId, rpcMakeInternalError("Websocket write failed, can't reconnect to RPC port."));
    } else {
      rpcWs_->write(boost::asio::buffer(*message), ec);
      if (ec) {
        rpcFailed_ = true;
        // Since we can't make rpcDoCall() block on the completion handler, if the
        // websocket->write() fails, it will set an error response to the request Id.
        rpcSetResponse(requestId, rpcMakeInternalError("Websocket write failed: " + ec.message()));
      }
    }
  });

  return requestId;
}

template <typename T>
bool WebsocketRPCConnection<T>::rpcStartConnection(int rpcPort) {
  std::scoped_lock lock(rpcStateMutex_);
  return rpcDoStart(rpcPort);
}

template <typename T>
void WebsocketRPCConnection<T>::rpcStopConnection() {
  // Not super elegant, but we can lock an extra mutex and set an extra flag here that will block any thread
  // trying to make another RPC call, just so that we stop trying to make more calls while we are stopping.
  // Then, we just wait some time (say, 1 second) for any pending responses to be returned by cometbft to
  // us (we are not holding rpcStateMutex_ here, so the read handler is free to repost itself).
  // Finally, we turn off the flag and allow other threads to resume posting RPCs.
  // This should eliminate or at least greatly reduce the possibility of RPC responses that ready to be
  // read but are then thrown away by rpcDoStop().
  std::unique_lock<std::mutex> stopLock(rpcStopMutex_);
  if (rpcStop_) {
    // Whoops, some other thread is also calling rpcStopConnection() for some reason.
    // Wait for *that* rpcStopConnection() call in the other thread to complete.
    while (rpcStop_) {
      stopLock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Good enough latency
      stopLock.lock();
    }
    // By definition we are already stopped, thanks to the other thread that was detected here.
    return;
  }
  rpcStop_ = true; // prevents new rpc requests (and other threads calling rpcStopConnection())
  stopLock.unlock();
  // Wait some time so any pending RPC responses get a chance to be read
  std::this_thread::sleep_for(std::chrono::seconds(1));
  // Now that we gave it some time for in-flight async RPC responses to be absorbed, do stop
  std::unique_lock<std::mutex> stateLock(rpcStateMutex_);
  rpcDoStop();
  stateLock.unlock();
  // Finally, allow new rpc requests to go through
  stopLock.lock();
  rpcStop_ = false;
  //stopLock dtor unlocks here
}

template <typename T>
bool WebsocketRPCConnection<T>::rpcCheckConnection() {
  std::scoped_lock lock(rpcStateMutex_);
  return rpcRunning_ && !rpcFailed_;
}

template <typename T>
bool WebsocketRPCConnection<T>::rpcGetSyncResponse(const uint64_t requestId, json& outResult) {
  std::scoped_lock lock(rpcSyncResponseMapMutex_);
  auto it = rpcSyncResponseMap_.find(requestId);
  if (it != rpcSyncResponseMap_.end()) {
    outResult = it->second;
    rpcSyncResponseMap_.erase(it);
    return true;
  }
  return false;
}

template <typename T>
uint64_t WebsocketRPCConnection<T>::rpcGetNextAsyncResponse(json& outResponse, T& outRequestData) {
  // Get next element and return it, if any
  uint64_t requestId;
  {
    std::scoped_lock lock(rpcAsyncResponseMapMutex_);
    if (rpcAsyncResponseMap_.empty()) {
      return 0;
    }
    auto it = rpcAsyncResponseMap_.begin();
    requestId = it->first;
    // Retrieve the response and erase it from the async request map
    outResponse = it->second;
    rpcAsyncResponseMap_.erase(it);
  }
  // If you have a response (rpcAsyncResponseMap_ entry) then you necessarily have a T in rpcAsyncSent_.
  // And if every request always gets a response set eventually (which is required for this
  // class to be correct) then both objects get eventually cleared by rpcGetNextAsyncResponse.
  {
    std::scoped_lock lock(rpcAsyncSentMutex_);
    auto it = rpcAsyncSent_.find(requestId);
    outRequestData = it->second;
    rpcAsyncSent_.erase(it);
  }
  return requestId;
}

template <typename T>
uint64_t WebsocketRPCConnection<T>::rpcAsyncCall(const std::string& method, const json& params, const T& requestData) {
  if (requestData == T{}) {
    SLOGERROR("Internal error. rpcAsyncCal() invoked with empty requestData, which is reserved for rpcSyncCall().");
    return 0;
  }

  // If we want to check rpcFailed_ to close the connection opportunistically, we must do it first here
  if (rpcFailed_) {
    rpcStopConnection();
  }

  // If rpcStop_ is true, spin until it is false (must retain the mutex when exiting the loop)
  // rpcStopMutex_ is always acquired before rpcStateMutex_ for all rpc requests
  std::unique_lock<std::mutex> stopLock(rpcStopMutex_);
  while (rpcStop_) {
    stopLock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Good enough latency
    stopLock.lock();
  }

  // Get the state mutex
  std::scoped_lock stateLock(rpcStateMutex_);

  // While holding both mutexes, it is safe to try to *start* the connection if we are not connected
  // (say, if we stopped due to detecting rpcFailed_ above).
  if (!rpcRunning_) {
    rpcDoStart();
  }

  // Finally, do the call (if not connected or still failed, this will return 0)
  return rpcDoCall(method, params, requestData);
}

template <typename T>
bool WebsocketRPCConnection<T>::rpcSyncCall(const std::string& method, const json& params, json& outResult) {

  // If we want to check rpcFailed_ to close the connection opportunistically, we must do it first here
  if (rpcFailed_) {
    rpcStopConnection();
  }

  // If rpcStop_ is true, spin until it is false (must retain the mutex when exiting the loop)
  // rpcStopMutex_ is always acquired before rpcStateMutex_ for all rpc requests
  std::unique_lock<std::mutex> stopLock(rpcStopMutex_);
  while (rpcStop_) {
    stopLock.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Good enough latency
    stopLock.lock();
  }

  // Get the state mutex
  std::unique_lock<std::mutex> stateLock(rpcStateMutex_);

  // While holding both mutexes, it is safe to try to *start* the connection if we are not connected
  // (say, if we stopped due to detecting rpcFailed_ above).
  if (!rpcRunning_) {
    rpcDoStart();
  }

  // Do the sync call
  uint64_t requestId;
  try {
    // T{} is how rpcDoCall() knows it's being used for a sync call
    requestId = rpcDoCall(method, params, T{});
  } catch (const std::exception& ex) {
    outResult = rpcMakeInternalError("Exception caught: " + std::string(ex.what()));
    return false;
  }

  // Release the state mutex as we are done with comms and will
  // just busy loop polling the rpcSyncRequestMap_
  stateLock.unlock();

  // Block waiting for a response to the request.
  // It is imperative that every request that is successfully sent gets a response
  // OR that the RPC connection is closed.
  if (requestId == 0) {
    return false;
  }
  while (true) {
    // If the connection fails or closes for whatever reason, just fail the sync call since, upon stop,
    // rpcCancelAllAsyncRequests() -- as the name implies -- doesn't do anything for sync calls (and stop
    // isn't even called here on failure).
    // This acquires the state mutex so you can't be already holding it here.
    if (!rpcCheckConnection()) {
      return false;
    }
    // Poll for the response in the sync responses map
    if (rpcGetSyncResponse(requestId, outResult)) {
      return !outResult.contains("error");
    }
    // Yes, it's slow, because you shouldn't use rpcSyncCall() unless it's for some kind of probing or testing.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// ---------------------------------------------------------------------------------------
// RPC request data types
// ---------------------------------------------------------------------------------------

// Comet::sendTransaction() async RPC request data
// - std::shared_ptr<Hash>: Eth hash of the transaction being sent (nullptr if ignoring txCache)
// - Bytes: transaction data (Bytes)
using TxSendType = std::tuple<std::shared_ptr<Hash>, Bytes>;

// Comet::checkTransaction() async RPC request data
// - std::string: cometbft SHA256 hex hash of the transaction being queried (no "0x" prefix)
using TxCheckType = std::string;

// Generic async RPC request data (when calling any RPC)
// - std::string: JSON-RPC call method name
// - json: JSON-RPC call parameters
// Mostly for driver user async calls (Comet::rpcAsyncCall()); can also be used by CometImpl
// internally to perform async RPC calls that don't need to bind any extra data to the request
// (can further multiplex in the std::visit step by e.g. method, parameters and/or result).
using DefaultAsyncRPCType = std::tuple<std::string, json>;

// Variant type that combines the request data type of all possible rpcAsyncCall() requests.
using CometRPCRequestType = std::variant<TxSendType, TxCheckType, DefaultAsyncRPCType>;

// ---------------------------------------------------------------------------------------
// CometImpl class
// ---------------------------------------------------------------------------------------

// Maximum size of an RPC request (should be loopback only, so the limit can be relaxed)
// This should probably be greater than the maximum block size, in case we get a full
// block transmitted via RPC for some reason.
#define COMET_RPC_MAX_BODY_BYTES 200000000

/**
 * CometImpl implements the interface to CometBFT using ABCIServer and ABCISession from
 * src/net/abci/*, which implement the TCP ABCI Socket server for a cometbft instance to
 * connect to. The ABCI server implementation uses proto/*, which is the Protobuf message
 * codec used to exchange objects through the ABCI with a running cometbft instance.
 * CometImpl also manages configuring, launching, monitoring and terminating the cometbft
 * process, as well as interfacing with its RPC port which is not exposed to BDK users.
 */
class CometImpl : public ABCIHandler, public Log::LogicalLocationProvider {
  private:
    CometListener* listener_; ///< Comet class application event listener/handler
    const std::string instanceIdStr_; ///< Identifier for logging
    const Options options_; ///< Copy of the supplied Options.
    const bool stepMode_; ///< Set to 'true' if unit testing with cometbft in step mode (no empty blocks).

    std::unique_ptr<ABCIServer> abciServer_; ///< TCP server for cometbft ABCI connection
    std::optional<boost::process::child> process_; ///< boost::process that points to the internally-tracked async cometbft process
    std::string processStdout_, processStderr_; ///< Buffers for saving the output of process_ (if not redirecting to the log file).
    std::atomic<bool> processDone_; ///< Set to `true` when process_ (likely) exited.

    std::mutex stateMutex_; //< Serializes start() and stop() calls
    std::future<void> loopFuture_; ///< Future object holding the consensus engine thread.
    std::atomic<bool> stop_ = false; ///< Flag for stopping the consensus engine thread.
    std::atomic<bool> status_ = true; ///< Global status (true = OK, false = failed/terminated).
    std::string errorStr_; ///< Error message (if any).
    CometError errorCode_ = CometError::NONE; ///< Error code (CometError::NONE if no error).

    std::atomic<CometState> state_ = CometState::STOPPED; ///< Current step the Comet instance is in.
    std::atomic<CometState> pauseState_ = CometState::NONE; ///< Step to pause/hold the comet instance at, if any.

    std::atomic<int> infoCount_ = 0; ///< Simple counter for how many cometbft Info requests we got.

    std::atomic<int> rpcPort_ = 0; ///< RPC port that will be used by our cometbft instance.

    std::mutex nodeIdMutex_; ///< mutex to protect reading/writing nodeId_.
    std::string nodeId_; ///< Cometbft node id or an empty string if not yet retrieved.

    uint64_t lastCometBFTBlockHeight_; ///< Current block height stored in the cometbft data dir, if known.
    std::string lastCometBFTAppHash_; ///< Current app hash stored in the cometbft data dir, if known.

    std::atomic<uint64_t> txCacheSize_ = 1000000; ///< Transaction cache size in maximum entries per bucket (0 to disable).
    std::mutex txCacheMutex_; ///< Mutex to protect cache access.
    std::array<std::unordered_map<Hash, CometTxStatus, SafeHash>, 2> txCache_; ///< Transaction cache as two rotating buckets.
    uint64_t txCacheBucket_ = 0; ///< Active txCache_ bucket.

    WebsocketRPCConnection<CometRPCRequestType> rpc_; ///< Singleton websocket RPC connection to process_ (started/stopped as needed).

    void setState(const CometState& state); ///< Apply a state transition, possibly pausing at the new state.
    void setError(const std::string& errorStr); ///< Signal a fatal error condition.
    void setErrorCode(CometError errorCode); ///< Specify the error code when there's an error condition.
    void resetError(); ///< Reset internal error condition.
    void cleanup(); ///< Ensure Comet is in a cleaned up state (kill cometbft, close RPC connection, stop & delete ABCI server, etc.)
    void startCometBFT(const std::vector<std::string>& cometArgs, bool saveOutput); ///< Launch the CometBFT process_ (at most one at a time).
    void stopCometBFT(); ///< Terminate the CometBFT process_.
    void workerLoop(); ///< Worker loop responsible for establishing and managing a connection to cometbft.
    void workerLoopInner(); ///< Called by workerLoop().

  public:

    std::string getLogicalLocation() const override { return instanceIdStr_; } ///< Log instance
    CometImpl(CometListener* listener, std::string instanceIdStr, const Options& options, bool stepMode);
    virtual ~CometImpl();
    void setTransactionCacheSize(const uint64_t cacheSize);
    bool getStatus();
    const std::string& getErrorStr();
    CometError getErrorCode();
    CometState getState();
    void setPauseState(const CometState pauseState = CometState::NONE);
    CometState getPauseState();
    std::string waitPauseState(uint64_t timeoutMillis);
    std::string getNodeID();
    bool start();
    bool stop();
    uint64_t sendTransaction(const Bytes& tx, std::shared_ptr<Hash>* ethHash);
    uint64_t checkTransaction(const std::string& txHash);
    bool checkTransactionInCache(const Hash& txEthHash, CometTxStatus& txStatus);
    bool rpcSyncCall(const std::string& method, const json& params, json& outResult);
    uint64_t rpcAsyncCall(const std::string& method, const json& params);

    // ---------------------------------------------------------------------------------------
    // ABCIHandler interface
    // ---------------------------------------------------------------------------------------

    virtual void echo(const cometbft::abci::v1::EchoRequest& req, cometbft::abci::v1::EchoResponse* res);
    virtual void flush(const cometbft::abci::v1::FlushRequest& req, cometbft::abci::v1::FlushResponse* res);
    virtual void info(const cometbft::abci::v1::InfoRequest& req, cometbft::abci::v1::InfoResponse* res);
    virtual void init_chain(const cometbft::abci::v1::InitChainRequest& req, cometbft::abci::v1::InitChainResponse* res);
    virtual void prepare_proposal(const cometbft::abci::v1::PrepareProposalRequest& req, cometbft::abci::v1::PrepareProposalResponse* res);
    virtual void process_proposal(const cometbft::abci::v1::ProcessProposalRequest& req, cometbft::abci::v1::ProcessProposalResponse* res);
    virtual void check_tx(const cometbft::abci::v1::CheckTxRequest& req, cometbft::abci::v1::CheckTxResponse* res);
    virtual void commit(const cometbft::abci::v1::CommitRequest& req, cometbft::abci::v1::CommitResponse* res);
    virtual void finalize_block(const cometbft::abci::v1::FinalizeBlockRequest& req, cometbft::abci::v1::FinalizeBlockResponse* res);
    virtual void query(const cometbft::abci::v1::QueryRequest& req, cometbft::abci::v1::QueryResponse* res);
    virtual void list_snapshots(const cometbft::abci::v1::ListSnapshotsRequest& req, cometbft::abci::v1::ListSnapshotsResponse* res);
    virtual void offer_snapshot(const cometbft::abci::v1::OfferSnapshotRequest& req, cometbft::abci::v1::OfferSnapshotResponse* res);
    virtual void load_snapshot_chunk(const cometbft::abci::v1::LoadSnapshotChunkRequest& req, cometbft::abci::v1::LoadSnapshotChunkResponse* res);
    virtual void apply_snapshot_chunk(const cometbft::abci::v1::ApplySnapshotChunkRequest& req, cometbft::abci::v1::ApplySnapshotChunkResponse* res);
    virtual void extend_vote(const cometbft::abci::v1::ExtendVoteRequest& req, cometbft::abci::v1::ExtendVoteResponse* res);
    virtual void verify_vote_extension(const cometbft::abci::v1::VerifyVoteExtensionRequest& req, cometbft::abci::v1::VerifyVoteExtensionResponse* res);
};

CometImpl::CometImpl(CometListener* listener, std::string instanceIdStr, const Options& options, bool stepMode)
  : listener_(listener), instanceIdStr_(instanceIdStr), options_(options), stepMode_(stepMode), rpc_(instanceIdStr)
{
}

CometImpl::~CometImpl() {
  stop();
}

void CometImpl::setTransactionCacheSize(const uint64_t cacheSize) {
  txCacheSize_ = cacheSize;
  // If cache gets smaller than current capacity it just keeps its current elements until the next bucket flip cycle.
  // Exception is if the size is set to 0. In that case we just purge the cache.
  // A cache purge can be forced by setting size to 0 then resetting it back to its previous positive size.
  if (txCacheSize_ == 0) {
    std::scoped_lock lock(txCacheMutex_);
    txCache_[0].clear();
    txCache_[1].clear();
    txCacheBucket_ = 0;
  }
}

bool CometImpl::getStatus() {
  return status_;
}

const std::string& CometImpl::getErrorStr() {
  return errorStr_;
}

CometError CometImpl::getErrorCode() {
  return errorCode_;
}

CometState CometImpl::getState() {
  return state_;
}

void CometImpl::setPauseState(const CometState pauseState) {
  pauseState_ = pauseState;
}

CometState CometImpl::getPauseState() {
  return pauseState_;
}

std::string CometImpl::waitPauseState(uint64_t timeoutMillis) {
  auto timeoutTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMillis);
  while (timeoutMillis == 0 || std::chrono::steady_clock::now() < timeoutTime) {
    if (!this->status_) {
      return this->errorStr_;
    }
    if (this->pauseState_ == this->state_ || this->pauseState_ == CometState::NONE) {
      return ""; // succeed if the pause state is reached or if pause is disabled (set to NONE)
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return "TIMEOUT";
}

std::string CometImpl::getNodeID() {
  std::scoped_lock(this->nodeIdMutex_);
  return this->nodeId_;
}

uint64_t CometImpl::sendTransaction(const Bytes& tx, std::shared_ptr<Hash>* ethHash) {
  // Add transaction to a queue that will try to push it to our cometbft instance.
  // Queue is NEVER reset on continue; etc. it is the same node, need to deliver tx to it eventually.
  // NOTE: sendTransaction is about queuing the tx object so that it is eventually dispatched to
  //       the localhost cometbft node. the only guarantee is that the local cometbft node will see it.
  //       it does NOT mean it is accepted by the application's blockchain network or anything else.

  // Since we killed the txOut_ queue and hardwired this to sending to the WebsocketRPCConnection,
  // we need to ensure the connection is to a cometbft start (CometState::RUNNING) node, not inspect
  // or anything else (worse: just not running at all and can't connect).
  // Luckily, once the driver is at CometState::RUNNING, the worst that can happen is an erroneous
  // shutdown, so if we pass the test here the worst that can happen is a failure to send, which
  // is correct anyway.
  auto currState = getState();
  if (currState != CometState::RUNNING) {
    LOGDEBUG("Error: called sendTransaction() while at state: " + std::to_string(static_cast<int>(currState)));
    return 0;
  }

  // If cache is enabled and caller wants us to insert/track this tx in the cache (i.e. ethHash != nullptr)
  if (txCacheSize_ > 0 && ethHash) {
    // If ethHash is a shared_ptr, but the shared_ptr tracks a null object, compute the eth hash
    // of the transaction bytes and store them in the shared_ptr.
    if (!*ethHash) {
      *ethHash = std::make_shared<Hash>(Utils::sha3(tx));
    }

    Hash& ethHashRef = *(*ethHash);

    std::scoped_lock lock(txCacheMutex_);

    // If there's already a transaction in the cache with the same sha3 key, and the transaction
    // entry isn't fully dead (i.e. REJECTED, meaning it CAN be resent), then refuse to send it.
    for (int i = 0; i < 2; ++i) {
      auto& bucket = txCache_[(txCacheBucket_ + i) % 2];
      auto it = bucket.find(ethHashRef);
      if (it != bucket.end()) {
        if (it->second.height != CometTxStatusHeight::REJECTED) {
          return 0; // Invalid ticket: not sent (error)
        }
      }
    }
  }

  std::string encodedTx = base64::encode_into<std::string>(tx.begin(), tx.end());
  json stparams = { {"tx", encodedTx} };

  CometRPCRequestType requestData = TxSendType{ethHash ? *ethHash : nullptr, tx};

  uint64_t requestId = rpc_.rpcAsyncCall("broadcast_tx_async", stparams, requestData);

  if (requestId != 0) {
    // Only need to add the transaction to the cache if there's a chance
    // broadcast_tx_async actually went through (asynccall ticket# > 0).
    if (txCacheSize_ > 0 && ethHash) {
      CometTxStatus txStatus;
      txStatus.height = CometTxStatusHeight::SUBMITTING; // Got rid of intermediary queue
      txStatus.index = -1;
      txStatus.cometTxHash = "";
      txStatus.result = CometExecTxResult();

      Hash& ethHashRef = *(*ethHash);

      std::scoped_lock lock(txCacheMutex_);

      auto& activeBucket = txCache_[txCacheBucket_];
      activeBucket[ethHashRef] = txStatus;

      if (activeBucket.size() >= txCacheSize_) {
        txCacheBucket_ = 1 - txCacheBucket_;
        txCache_[txCacheBucket_].clear();
      }
    }
  }
  return requestId;
}

bool CometImpl::checkTransactionInCache(const Hash& txEthHash, CometTxStatus& txStatus) {
  // Since this method takes an eth hash, this just does a lookup in the cache.
  // If this is a transaction that this node sent, then it is visible through its
  // whole lifetime. Also, the cache is updated when a FinalizedBlock is processed,
  // so even if you never *sent* the transaction, you will eventually find it through
  // here when it gets included in a block (a recent block, that is, as the cache has
  // a maximum size).
  if (txCacheSize_ > 0) {
    // Check both buckets, but check the active bucket (txCacheBucket_) first.
    std::scoped_lock lock(txCacheMutex_);
    for (int i = 0; i < 2; ++i) {
      const auto& bucket = txCache_[(txCacheBucket_ + i) % 2];
      auto it = bucket.find(txEthHash);
      if (it != bucket.end()) {
        txStatus = it->second;
        return true;
      }
    }
  }
  return false;
}

uint64_t CometImpl::checkTransaction(const std::string& txHash) {
  // Turns out the cometbft /tx RPC endpoint is not ideal, as it returns the entire transaction body
  // when we are checking for the status of a transaction, and it also takes some time between
  // seeing that the transaction went into a block and indexing it (it just says "not found"/error
  // if the transaction is pending, i.e. as if it didn't exist at all).
  // Instead of putting a cache in front of this method, it's better to leave it as doing a /tx query
  // only, since it takes the SHA256 hash anyway (meaning you want cometbft's idea of the tx).

  // NOTE: If you want to call 'tx' on cometbft ispect using an async RPC, use Comet::rpcAsyncCall().

  // Since we killed the txCheck_ queue and hardwired this to sending to the WebsocketRPCConnection,
  // we need to ensure the connection is to a cometbft start (CometState::RUNNING) node, not inspect
  // or anything else (worse: just not running at all and can't connect).
  // Luckily, once the driver is at CometState::RUNNING, the worst that can happen is an erroneous
  // shutdown, so if we pass the test here the worst that can happen is a failure to send, which
  // is correct anyway.
  auto currState = getState();
  if (currState != CometState::RUNNING) {
    LOGDEBUG("Error: called checkTransaction() while at state: " + std::to_string(static_cast<int>(currState)));
    return 0;
  }

  Bytes hx = Hex::toBytes(txHash);
  std::string encodedHexBytes = base64::encode_into<std::string>(hx.begin(), hx.end());
  json ctparams = { {"hash", encodedHexBytes} };

  CometRPCRequestType requestData = TxCheckType{txHash};

  uint64_t requestId = rpc_.rpcAsyncCall("tx", ctparams, requestData);
  return requestId;
}

bool CometImpl::rpcSyncCall(const std::string& method, const json& params, json& outResult) {
  if (!process_.has_value()) {
    outResult = rpcMakeInternalError("Cometbft is not running.");
    return false;
  }
  return rpc_.rpcSyncCall(method, params, outResult);
}

uint64_t CometImpl::rpcAsyncCall(const std::string& method, const json& params) {
  if (!process_.has_value()) {
    return 0;
  }
  CometRPCRequestType requestData = DefaultAsyncRPCType{method, params};
  return rpc_.rpcAsyncCall(method, params, requestData);
}

bool CometImpl::start() {
  std::lock_guard<std::mutex> lock(this->stateMutex_);
  if (this->loopFuture_.valid()) {
    return false;
  }
  this->stop_ = false;
  resetError(); // ensure error status is off
  setState(CometState::STARTED);
  this->loopFuture_ = std::async(std::launch::async, &CometImpl::workerLoop, this);
  return true;
}

bool CometImpl::stop() {
  std::lock_guard<std::mutex> lock(this->stateMutex_);
  if (!this->loopFuture_.valid()) {
    return false;
  }
  this->stop_ = true;
  this->setPauseState(); // must reset any pause state otherwise it won't ever finish
  this->loopFuture_.wait();
  this->loopFuture_.get(); // joins with the workerLoop thread and sets the future to invalid
  // The loopFuture (workerLoop) is responsible for calling cleanup(), irrespective of
  // the ending state (FINSIHED vs TERMINATED).
  resetError(); // stop() clears any error status
  setState(CometState::STOPPED);
  return true;
}

void CometImpl::setState(const CometState& state) {
  LOGTRACE("Set comet state: " + std::to_string((int)state));
  auto oldState = state;
  this->state_ = state;
  listener_->cometStateTransition(this->state_, oldState);
  if (this->pauseState_ == this->state_) {
    LOGTRACE("Pausing at comet state: " + std::to_string((int)state));
    while (this->pauseState_ == this->state_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    LOGTRACE("Unpausing at comet state: " + std::to_string((int)state));
  }
}

void CometImpl::setError(const std::string& errorStr) {
  LOGDEBUG("Comet ERROR raised: " + errorStr);
  this->errorStr_ = errorStr;
  // If we raise an error condition, with an error message, but the error code
  // is still unset, then set the error to the generic CometError::ERROR value.
  if (this->errorCode_ == CometError::NONE) {
    this->errorCode_ = CometError::ERROR;
  }
  this->status_ = false;
}

void CometImpl::setErrorCode(CometError errorCode) {
  LOGDEBUG("Comet ERROR raised (code): " + std::to_string(static_cast<int>(errorCode)));
  this->errorCode_ = errorCode;
  this->status_ = false;
}

void CometImpl::resetError() {
  this->status_ = true;
  this->errorStr_ = "";
  this->errorCode_ = CometError::NONE;
}

void CometImpl::cleanup() {

  // Close the RPC connection, if any
  rpc_.rpcStopConnection();

  // Stop the CometBFT node, if any
  stopCometBFT();

  // Stop and destroy the ABCI net engine, if any
  if (abciServer_) {
    LOGTRACE("Waiting for abciServer_ networking to stop running (a side-effect of the cometbft process exiting.)");
    // wait up to 4s for the ABCI connection to close from the cometbft end
    // we don't actually need to wait at all for this, but it's arguably better if cometbft is gone
    //   before we close the ABCI app server.
    int waitRunningTries = 200;
    while (abciServer_->running() && --waitRunningTries > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    if (waitRunningTries) {
      LOGTRACE("abciServer_ networking has stopped running, we can now stop the ABCI net engine.");
    } else {
      LOGDEBUG("WARNING: abciServer_ has not stopped running after waiting (cometbft might still be up); will stop the ABCI net engine regardless.");
    }
    abciServer_->stop();
    LOGTRACE("abciServer_ networking engine stopped.");
    abciServer_.reset();
    LOGTRACE("abciServer_ networking engine destroyed.");
  } else {
    LOGTRACE("No abciServer_ instance, so nothing to do.");
  }

  // Reset what we know about the cometbft store state (this is the default state if the block store is empty)
  lastCometBFTBlockHeight_ = 0;
  lastCometBFTAppHash_ = "";

  // Reset any state we might have as an ABCIHandler
  infoCount_ = 0; // needed for the TESTING_COMET -> TESTED_COMET state transition.
  rpcPort_ = 0;
  std::unique_lock<std::mutex> resetInfoLock(this->nodeIdMutex_);
  this->nodeId_ = ""; // only known after "comet/config/node_key.json" is set and "cometbft show-node-id" is run.
  resetInfoLock.unlock();
}

/**
 * Run the internally-tracked CometBFT instance if it is not already started.
 * @param cometArgs Arguments to pass to cometbft.
 * @param saveOutput If `true`, don't send stdout/stderr to the logfile and instead save to processStdout_ and processStderr_.
 */
void CometImpl::startCometBFT(const std::vector<std::string>& cometArgs, bool saveOutput) {
  if (process_.has_value()) {
    setErrorCode(CometError::FATAL);
    throw DynamicException("Internal error: startCometBFT() called but there's already one process_ running.");
  }

  this->processStdout_ = "";
  this->processStderr_ = "";
  this->processDone_ = false;

  // execute either setpriv or cometbft directly
  boost::filesystem::path exec_path;
  std::vector<std::string> exec_args;

  // Search for the cometbft executable in the system's PATH
  boost::filesystem::path cometbft_exec_path = boost::process::search_path("cometbft");
  if (cometbft_exec_path.empty()) {
    // This is a non-recoverable error
    // The ABCI server will be stopped/collected during stop()
    setErrorCode(CometError::FATAL);
    throw DynamicException("cometbft executable not found in system PATH");
    return;
  }

  // Search for setpriv in the PATH
  boost::filesystem::path setpriv_exec_path = boost::process::search_path("setpriv");
  if (setpriv_exec_path.empty()) {
    // setpriv not found, so just run cometbft directly, which is less good and requires the node
    // operator to run its own watchdog or handle dangling cometbft processes.
    LOGWARNING("setpriv utility not found in system PATH (usually found at /usr/bin/setpriv). cometbft child process will not be automatically terminated if this BDK node process crashes.");
    exec_path = cometbft_exec_path;
    exec_args = cometArgs;
  } else {
    // setpriv found, so use it to send SIGTERM to cometbft if this BDK node process (parent process) dies
    // note that setpriv replaces its executable image with that of the process given in its args, in this
    //   case, cometbft. so the PID of the setpriv child process will be the same PID of the cometbft process
    //   that is passed as an arg to setpriv, along with the arguments to forward to cometbft.
    //   however, and that is the point of using setpriv, the resulting cometbft process will receive a SIGTERM
    //   if its parent process (this BDK node process) dies, so that we don't get a dangling cometbft in that case.
    LOGDEBUG("Launching cometbft via setpriv --pdeathsig SIGTERM");
    exec_path = setpriv_exec_path;
    exec_args = {
      "setpriv",
      "--pdeathsig", "SIGTERM",
      "--",
      cometbft_exec_path.string()
    };
    // append cometbft args after the setpriv args
    exec_args.insert(exec_args.end(), cometArgs.begin(), cometArgs.end());
  }

  std::string argsString;
  for (const auto& arg : exec_args) { argsString += arg + " "; }
  LOGDEBUG("Launching " + exec_path.string() + " with arguments: " + argsString);

  // Launch the process
  auto bpout = std::make_shared<boost::process::ipstream>();
  auto bperr = std::make_shared<boost::process::ipstream>();
  process_ = boost::process::child(
    exec_path,
    boost::process::args(exec_args),
    boost::process::std_out > *bpout,
    boost::process::std_err > *bperr
  );
  std::string pidStr = std::to_string(process_->id());
  LOGDEBUG("Launched cometbft with PID: " + pidStr);

  // Spawn two detached threads to pump stdout and stderr to bdk.log.
  // They should go away naturally when process_ is terminated.
  std::thread stdout_thread([saveOutput, bpout, pidStr, this]() {
    std::string line;
    while (*bpout && std::getline(*bpout, line) && !line.empty()) {
      if (saveOutput) {
        GLOGXTRACE("[cometbft stdout]: " + line);
        this->processStdout_ += line + '\n';
      } else {
        GLOGDEBUG("[cometbft stdout]: " + line);
      }
    }
    // remove trailing \n so that e.g. the node id from cometbft show-node-id is exactly processStdout_ without a need to trim it.
    if (!this->processStdout_.empty()) { this->processStdout_.pop_back(); }
    GLOGDEBUG("cometbft stdout stream pump thread finished, cometbft pid = " + pidStr);
    this->processDone_ = true; // if actually interested in reading processStderr_ you can just e.g. sleep(1s) first
  });
  std::thread stderr_thread([saveOutput, bperr, pidStr, this]() {
    std::string line;
    while (*bperr && std::getline(*bperr, line) && !line.empty()) {
      if (saveOutput) {
        GLOGXTRACE("[cometbft stderr]: " + line);
        this->processStderr_ += line + '\n';
      } else {
        GLOGDEBUG("[cometbft stderr]: " + line);
      }
    }
    if (!this->processStderr_.empty()) { this->processStderr_.pop_back(); }
    GLOGDEBUG("cometbft stderr stream pump thread finished, cometbft pid = " + pidStr);
  });
  stdout_thread.detach();
  stderr_thread.detach();
}

void CometImpl::stopCometBFT() {
  // if we have a process_ running then we will send SIGTERM to it and if needed SIGKILL
  if (process_.has_value()) {
    LOGDEBUG("Terminating CometBFT process");
    // terminate the process
    pid_t pid = process_->id();
    try {
      process_->terminate(); // SIGTERM (graceful termination, equivalent to terminal CTRL+C/SIGINT)
      LOGDEBUG("Process with PID " + std::to_string(pid) + " terminated");
      process_->wait();  // Ensure the process is fully terminated
      LOGDEBUG("Process with PID " + std::to_string(pid) + " joined");
    } catch (const std::exception& ex) {
      // This is bad, and if it actually happens, we need to be able to do something else here to ensure the process disappears
      //   because we don't want a process using the data directory and using the socket ports.
      LOGWARNING("Failed to terminate process: " + std::string(ex.what()));
      // Fallback: Forcefully kill the process using kill -9
      try {
        std::string killCommand = "kill -9 " + std::to_string(pid);
        LOGINFO("Attempting to force kill process with PID " + std::to_string(pid) + " using kill -9");
        int result = std::system(killCommand.c_str());
        if (result == 0) {
          LOGINFO("Successfully killed process with PID " + std::to_string(pid) + " using kill -9");
        } else {
          LOGWARNING("Failed to kill process with PID " + std::to_string(pid) + " using kill -9. Error code: " + std::to_string(result));
        }
      } catch (const std::exception& ex2) {
        LOGERROR("Failed to execute kill -9: " + std::string(ex2.what()));
      }
    }
    process_.reset(); // this signals that we are ready to call startCometBFT() again
    LOGDEBUG("CometBFT process terminated");
  }
}

void CometImpl::workerLoop() {
  LOGDEBUG("Comet worker thread: started");
  // So basically we take every exception that can be thrown in the inner worker loop and if
  //   we catch one it means the Comet driver goes into TERMINATED state and we record the
  //   error condition as an error message.
  // In case of error (exception), before explicitly throwing the exception a custom error
  //   code should have already been set. If an explicit error code wasn't set, then the
  //   setError() method will set one internally.
  // Both FINISHED and TERMINATED state transitions only trigger AFTER we have cleaned up
  // the state. Note that you *cannot* reenter Comet via Comet::stop() from that callback!
  try {
    workerLoopInner();
    LOGDEBUG("Comet worker thread: finishing normally (calling cleanup)");
    cleanup();
    LOGDEBUG("Comet worker thread: finished normally (cleanup done, setting FINISHED state)");
    setState(CometState::FINISHED); // state transition callback CANNOT reenter Comet::stop()
  } catch (const std::exception& ex) {
    setError("Exception caught in comet worker thread: " + std::string(ex.what()));
    LOGDEBUG("Comet worker thread: finishing with error (calling cleanup)");
    cleanup();
    LOGDEBUG("Comet worker thread: finished with error (cleanup done, setting TERMINATED state)");
    setState(CometState::TERMINATED); // state transition callback CANNOT reenter Comet::stop()
  }
}

void CometImpl::workerLoopInner() {

  LOGDEBUG("Comet worker: started");

  // If we are stopping, then quit
  while (!stop_) {

    LOGDEBUG("Comet worker: start loop");

    // --------------------------------------------------------------------------------------
    // If this is a continue; and we are restarting the cometbft workerloop, ensure that any
    //   state and connections from the previous attempt are wiped out, regardless of the
    //   state they were previously in.
    // Continue; should ONLY be used for errors we know are transient and that we don't want
    //   to notify the application, just silently try again.

    cleanup();

    LOGDEBUG("Comet worker: running configuration step");

    // --------------------------------------------------------------------------------------
    // Run configuration step (writes to the comet/config/* files before running cometbft)
    //
    // The global option rootPath from options.json tells us the root data directory for BDK.
    // The Comet worker thread works by expecting a rootPath + /comet/ directory to be the
    //   home directory used by its managed cometbft instance.
    //
    // Before BDK will work with comet, it already needs to have access to all the consensus
    //   parameters it needs to configure cometbft with. These parameters must all be
    //   given to BDK itself, via e.g. options.json. So any parameters needed for cometbft
    //   must be first modeled as BDK parameters which are then passed through.
    //
    // If the home directory does not exist, it must be initialized using `cometbft init`,
    //   and then the configuration must be modified with the relevant parameters supplied
    //   to the BDK, such as validator keys.
    //
    // NOTE: It is cometbft's job to determine whether the node is acting as a
    //   validator or not. In options.json we can infer whether the node can ever be acting
    //   as a validator by checking whether cometBFT::privValidatorKey is set, but that is all;
    //   whether it is currently a validator or not is up to the running network. In the
    //   future we may want to unify the Options constructors into just one, and leave the
    //   initialization of "non-validator nodes" as the ones that set the
    //   cometBFT::privValidatorKey option to empty or undefined.
    //
    // node_key.json can also be forced to a known value. This should be the default behavior,
    //   since deleting the comet/* directory (as an e.g. recovery strategy) would cause a
    //   peer to otherwise regenerate their node key, which would impact its persistent
    //   connection to other nodes that have explicitly configured themselves to connect
    //   to the node id that specific node/peer.

    setState(CometState::CONFIGURING);

    const std::string rootPath = options_.getRootPath();
    const std::string cometPath = rootPath + "/comet/";
    const std::string cometConfigPath = cometPath + "config/";
    const std::string cometConfigGenesisPath = cometConfigPath + "genesis.json";
    const std::string cometConfigNodeKeyPath = cometConfigPath + "node_key.json";
    const std::string cometConfigPrivValidatorKeyPath = cometConfigPath + "priv_validator_key.json";
    const std::string cometConfigTomlPath = cometConfigPath + "config.toml";

    const std::string cometUNIXSocketPath = cometPath + "abci.sock";

    LOGDEBUG("Options RootPath: " + options_.getRootPath());

    const json& opt = options_.getCometBFT();

    if (opt.is_null()) {
      LOGWARNING("Configuration option cometBFT is null.");
    } else {
      LOGDEBUG("Configuration option cometBFT: " + opt.dump());
    }

    bool hasGenesis = opt.contains("genesis");
    json genesisJSON = json::object();
    if (hasGenesis) genesisJSON = opt["genesis"];

    bool hasPrivValidatorKey = opt.contains("privValidatorKey");
    json privValidatorKeyJSON = json::object();
    if (hasPrivValidatorKey) privValidatorKeyJSON = opt["privValidatorKey"];

    bool hasNodeKey = opt.contains("nodeKey");
    json nodeKeyJSON = json::object();
    if (hasNodeKey) nodeKeyJSON = opt["nodeKey"];

    bool hasP2PPort = opt.contains("p2p_port");
    json p2pPortJSON = json::object();
    if (hasP2PPort) p2pPortJSON = opt["p2p_port"];

    bool hasRPCPort = opt.contains("rpc_port");
    json rpcPortJSON = json::object();
    if (hasRPCPort) rpcPortJSON = opt["rpc_port"];

    bool hasPeers = opt.contains("peers");
    json peersJSON = json::object();
    if (hasPeers) peersJSON = opt["peers"];

    // --------------------------------------------------------------------------------------
    // Sanity check configuration: a comet genesis file must be explicitly given.

    if (!hasGenesis) {
      // Cannot proceed with an empty comet genesis spec on options.json.
      // E.g.: individual testcases or the test harness must fill in a valid
      //   cometBFT genesis config.
      setErrorCode(CometError::CONFIG);
      throw DynamicException("Configuration option cometBFT::genesis is empty.");
    } else {
      LOGINFO("CometBFT::genesis config found: " + genesisJSON.dump());
    }

    if (!hasP2PPort) {
      setErrorCode(CometError::CONFIG);
      throw DynamicException("Configuration option cometBFT:: p2p_port is empty.");
    } else {
      LOGINFO("CometBFT::p2p_port config found: " + p2pPortJSON.get<std::string>());
    }

    if (!hasRPCPort) {
      setErrorCode(CometError::CONFIG);
      throw DynamicException("Configuration option cometBFT:: rpc_port is empty.");
    } else {
      LOGINFO("CometBFT::rpc_port config found: " + rpcPortJSON.get<std::string>());

      // Save it so that we can reach the cometbft node via RPC to e.g. send transactions.
      rpcPort_ = atoi(rpcPortJSON.get<std::string>().c_str());
    }

    if (!hasNodeKey) {
      // This is allowed (some nodes will not care about what node ID they get), so just log it.
      LOGINFO("Configuration option cometBFT::nodeKey is empty.");
    } else {
      LOGINFO("CometBFT::nodeKey config found: " + nodeKeyJSON.dump());
    }

    if (!hasPrivValidatorKey) {
      // This is allowed (some nodes are not validators), so just log it.
      LOGINFO("Configuration option cometBFT::privValidatorKey is empty.");
    } else {
      LOGINFO("CometBFT::privValidatorKey config found: " + privValidatorKeyJSON.dump());
    }

    // --------------------------------------------------------------------------------------
    // BDK root path must be set up before the Comet worker is started.

    // If rootPath does not exist for some reason, quit.
    if (!std::filesystem::exists(rootPath)) {
      setErrorCode(CometError::FATAL);
      throw DynamicException("Root path not found: " + rootPath);
    }

    // --------------------------------------------------------------------------------------
    // If comet home directory does not exist inside rootPath, then create it via
    //   cometbft init. It will be created with all required options with standard values.

    if (!std::filesystem::exists(cometPath)) {
      LOGDEBUG("Comet worker: creating comet directory");

      // run cometbft init cometPath to create the cometbft directory with default configs
      Utils::execute("cometbft init --home " + cometPath);

      // check it exists now, otherwise halt node
      if (!std::filesystem::exists(cometPath)) {
        setErrorCode(CometError::FATAL);
        throw DynamicException("Could not create cometbft home directory");
      }
    }

    if (!std::filesystem::exists(cometConfigPath)) {
      // comet/config/ does not exist for some reason, which means the comet/ directory is broken
      setErrorCode(CometError::DATA);
      throw DynamicException("CometBFT home directory is broken: it doesn't have a config/ subdirectory");
    }

    LOGDEBUG("Comet worker: comet directory exists");

    // --------------------------------------------------------------------------------------
    // Comet home directory exists; check its configuration is consistent with the current
    //   BDK configuration options. If it isn't then sync them all here.

    // If cometBFT::nodeKey is set in options, write it over the default
    //   node_key.json comet file to ensure it is the same.
    if (hasNodeKey) {
      std::ofstream outputFile(cometConfigNodeKeyPath);
      if (outputFile.is_open()) {
        outputFile << nodeKeyJSON.dump(4);
        outputFile.close();
      } else {
        setErrorCode(CometError::FATAL);
        throw DynamicException("Cannot open comet nodeKey file for writing: " + cometConfigNodeKeyPath);
      }
    }

    // If cometBFT::privValidatorKey is set in options, write it over the default
    //   priv_validator_key.json comet file to ensure it is the same.
    if (hasPrivValidatorKey) {
      std::ofstream outputFile(cometConfigPrivValidatorKeyPath);
      if (outputFile.is_open()) {
        outputFile << privValidatorKeyJSON.dump(4);
        outputFile.close();
      } else {
        setErrorCode(CometError::FATAL);
        throw DynamicException("Cannot open comet privValidatorKey file for writing: " + cometConfigPrivValidatorKeyPath);
      }
    }

    // NOTE: If genesis option is required, must test for it earlier.
    // If cometBFT::genesis is set in options, write it over the default
    //   genesis.json comet file to ensure it is the same.
    if (hasGenesis) {
      std::ofstream outputFile(cometConfigGenesisPath);
      if (outputFile.is_open()) {
        outputFile << genesisJSON.dump(4);
        outputFile.close();
      } else {
        setErrorCode(CometError::FATAL);
        throw DynamicException("Cannot open comet genesis file for writing: " + cometConfigGenesisPath);
      }
    }

    // Sanity check the existence of the config.toml file
    if (!std::filesystem::exists(cometConfigTomlPath)) {
      setErrorCode(CometError::DATA);
      throw DynamicException("Comet config.toml file does not exist: " + cometConfigTomlPath);
    }

    // Open and parse the main comet config file (config.toml)
    toml::table configToml;
    try {
      configToml = toml::parse_file(cometConfigTomlPath);
    } catch (const toml::parse_error& err) {
      setErrorCode(CometError::DATA);
      throw DynamicException("Error parsing TOML file: " + std::string(err.description()));
    }

    // Force all relevant option values into config.toml
    configToml.insert_or_assign("abci", "socket"); // gets overwritten by --abci, and this is the default value anyway
    configToml.insert_or_assign("proxy_app", "unix://" + cometUNIXSocketPath); // gets overwritten by --proxy_app
    configToml["storage"].as_table()->insert_or_assign("discard_abci_responses", toml::value(true));
    std::string p2p_param = "tcp://0.0.0.0:" + p2pPortJSON.get<std::string>();
    std::string rpc_param = "tcp://0.0.0.0:" + rpcPortJSON.get<std::string>();
    configToml["p2p"].as_table()->insert_or_assign("laddr", p2p_param);
    configToml["rpc"].as_table()->insert_or_assign("laddr", rpc_param);

    // RPC options. Since the RPC port should be made accessible for local (loopback) connections only, it can
    //   accept unsafe comands if we need it to.
    // REVIEW: Maybe we should try and have the RPC endpoint be an unix:/// socket as well?
    configToml["rpc"].as_table()->insert_or_assign("max_body_bytes", COMET_RPC_MAX_BODY_BYTES);
    //configToml["rpc"].as_table()->insert_or_assign("unsafe", toml::value(true));

    // FIXME/TODO: right now we are just testing, so these security params are relaxed to allow
    //   testing on the same machine. these will have to be exposed as BDK options as well so
    //   they can be toggled for testing.
    configToml["p2p"].as_table()->insert_or_assign("allow_duplicate_ip", toml::value(true));
    configToml["p2p"].as_table()->insert_or_assign("addr_book_strict", toml::value(false));

    if (hasPeers) {
      // persistent_peers is a single string with the following format:
      // <ID>@<IP>:<PORT>
      // BDK validators should specify as many nodes as possible as persistent peers.
      // Additional methods for adding or discovering other validators and non-validators should also be available.
      //  (e.g. seeds/PEX).
      configToml["p2p"].as_table()->insert_or_assign("persistent_peers", peersJSON.get<std::string>());
    }

    // If running in stepMode (testing ONLY) set all the options required to make cometbft
    // never produce a block unless at least one transaction is there to be included in one,
    // and never generating null/timeout blocks.
    if (stepMode_) {
      LOGDEBUG("stepMode_ is set, setting step mode parameters for testing.");
      configToml["consensus"].as_table()->insert_or_assign("create_empty_blocks", toml::value(false));
      configToml["consensus"].as_table()->insert_or_assign("timeout_propose", "1s");
      configToml["consensus"].as_table()->insert_or_assign("timeout_propose_delta", "0s");
      configToml["consensus"].as_table()->insert_or_assign("timeout_prevote", "1s");
      configToml["consensus"].as_table()->insert_or_assign("timeout_prevote_delta", "0s");
      configToml["consensus"].as_table()->insert_or_assign("timeout_precommit", "1s");
      configToml["consensus"].as_table()->insert_or_assign("timeout_precommit_delta", "0s");
      configToml["consensus"].as_table()->insert_or_assign("timeout_commit", "0s");
    }

    // Overwrite updated config.toml
    std::ofstream configTomlOutFile(cometConfigTomlPath);
    if (!configTomlOutFile.is_open()) {
      setErrorCode(CometError::FATAL);
      throw DynamicException("Could not open file for writing: " + cometConfigTomlPath);
    }
    configTomlOutFile << configToml;
    if (configTomlOutFile.fail()) {
      setErrorCode(CometError::FATAL);
      throw DynamicException("Could not write file: " + cometConfigTomlPath);
    }
    configTomlOutFile.close();
    if (configTomlOutFile.fail()) {
      setErrorCode(CometError::FATAL);
      throw DynamicException("Failed to close file properly: " + cometConfigTomlPath);
    }

    LOGDEBUG("Comet setting configured");

    setState(CometState::CONFIGURED);

    LOGDEBUG("Comet set configured");

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Run cometbft inspect and check that everything is as expected.

    setState(CometState::INSPECTING_COMET);

    // Run cometbft show-node-id to figure out what the node ID is
    LOGDEBUG("Fetching own cometbft node-id...");
    try {
      std::vector<std::string> cometArgs = {
        "show-node-id",
        "--home=" + cometPath
      };
      startCometBFT(cometArgs, true);
    } catch (const std::exception& ex) {
      setErrorCode(CometError::RUN);
      throw DynamicException("Exception caught when trying to run cometbft show-node-id: " + std::string(ex.what()));
    }

    // Loop until process exits or we time out here
    int inspectTries = 50; //5s
    while (!processDone_ && --inspectTries > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (!processDone_) {
      setErrorCode(CometError::RUN_TIMEOUT);
      throw DynamicException("Timed out while waiting for run cometbft show-node-id.");
    }

    stopCometBFT();

    if (processStdout_.size() != 40) {
      setErrorCode(CometError::FAIL);
      throw DynamicException("Got a cometbft node-id of unexpected size (!= 40 hex chars): [" + processStdout_ + "]");
    }

    LOGDEBUG("Got comet node ID: [" + processStdout_ + "]");
    std::unique_lock<std::mutex> lock(this->nodeIdMutex_);
    this->nodeId_ = processStdout_;
    this->nodeIdMutex_.unlock();

    // Check if quitting
    if (stop_) break;

    // Here we need to inspect the current state of the cometbft node.
    // Any error thrown will close the running cometbft inspect since it's tracked by process_, just like
    //   cometbft start (regular node) is.

    LOGDEBUG("Starting cometbft inspect");

    // start cometbft inspect
    try {
      std::vector<std::string> cometArgs = {
        "inspect",
        "--home=" + cometPath
      };
      startCometBFT(cometArgs, true);
    } catch (const std::exception& ex) {
      setErrorCode(CometError::RUN);
      throw DynamicException("Exception caught when trying to run cometbft inspect: " + std::string(ex.what()));
    }

    LOGDEBUG("Starting RPC connection (inspect)");

    // start RPC connection
    int inspectRpcTries = 50; //5s
    bool inspectRpcSuccess = false;
    while (inspectRpcTries-- > 0 && !stop_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (rpc_.rpcStartConnection(rpcPort_)) {
        inspectRpcSuccess = true;
        break;
      }
      LOGDEBUG("Retrying RPC connection (inspect): " + std::to_string(inspectRpcTries));
    }

    LOGDEBUG("Done starting RPC connection");

    // Check if quitting
    if (stop_) break;

    if (!inspectRpcSuccess) {
      setErrorCode(CometError::RPC_TIMEOUT);
      throw DynamicException("Can't connect to the cometbft RPC port (inspect).");
    }

    json insRes;
    LOGDEBUG("Making sample RPC call");
    if (!rpc_.rpcSyncCall("header", json::object(), insRes)) {
      setErrorCode(CometError::RPC_CALL_FAILED);
      throw DynamicException("ERROR: cometbft inspect RPC header call failed: " + insRes.dump());
    }

    LOGDEBUG("cometbft inspect RPC header call returned OK: "+ insRes.dump());

    // We got an inspect latest header response; parse it to figure out
    //  lastCometBFTBlockHeight_ and lastCometBFTAppHash_.
    if (!insRes.is_object() || !insRes.contains("result") || !insRes["result"].is_object()) {
      setErrorCode(CometError::RPC_BAD_RESPONSE);
      throw DynamicException("Invalid or missing 'result' in cometbft inspect header response.");
    }
    if (!insRes["result"].contains("header")) {
      setErrorCode(CometError::RPC_BAD_RESPONSE);
      throw DynamicException("Invalid or missing 'header' in cometbft inspect header response.");
    }
    const auto& header = insRes["result"]["header"];
    if (header.is_null()) {
      // Header is null, which is valid and indicates an empty block store
      LOGDEBUG("Header is null; block store is empty.");
    } else {
      if ((!header.contains("height") || !header["height"].is_string())) {
        setErrorCode(CometError::RPC_BAD_RESPONSE);
        throw DynamicException("Missing or invalid 'height' in header.");
      }
      if ((!header.contains("app_hash") || !header["app_hash"].is_string())) {
        setErrorCode(CometError::RPC_BAD_RESPONSE);
        throw DynamicException("Missing or invalid 'app_hash' in header.");
      }
      // Got a non-empty header response, so we are past genesis in the cometbft store
      lastCometBFTBlockHeight_ = header["height"].get<std::string>().empty() ? 0 : std::stoull(header["height"].get<std::string>());
      lastCometBFTAppHash_ = header["app_hash"].get<std::string>();
      LOGDEBUG("Parsed header successfully: Last Block Height = " + std::to_string(lastCometBFTBlockHeight_) + ", Last App Hash = " + lastCometBFTAppHash_);
    }

    // Check if quitting
    if (stop_) break;

    // Notify the application of the height that CometBFT has in its block store.
    // If the application is ahead of this, it will need a strategy to cope with it.
    listener_->currentCometBFTHeight(lastCometBFTBlockHeight_);

    // --------------------------------------------------------------------------------------
    // Intermediary hold state that the app can setPauseState() at and then use the RPC
    //  call method to check anything it wants.

    // Check if quitting
    if (stop_) break;

    setState(CometState::INSPECT_RUNNING);

    // --------------------------------------------------------------------------------------
    // Finished inspect step.

    rpc_.rpcStopConnection();

    stopCometBFT();

    setState(CometState::INSPECTED_COMET);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Start our cometbft application's ABCI socket server; make sure it is started.

    setState(CometState::STARTING_ABCI);

    // start the ABCI server
    abciServer_ = std::make_unique<ABCIServer>(this, cometUNIXSocketPath);
    abciServer_->start();

    // the ABCI server listen socket will probably be up and running after this sleep.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // if it is not running by now then it is probably because starting the ABCI server failed.
    if (!abciServer_->running()) {
      setErrorCode(CometError::ABCI_SERVER_FAILED);
      throw DynamicException("Comet failed: ABCI server failed to start");
    }

    setState(CometState::STARTED_ABCI);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Run cometbft start, passing the socket address of our ABCI server as a parameter.

    setState(CometState::STARTING_COMET);

    // run cometbft which will connect to our ABCI server
    try {

      // ABCI server needs to be up and running at this point, which we ensured above.

      // CometBFT arguments
      //
      // We are making it so that an abci.sock file within the root of the
      //   CometBFT home directory ("cometPath" below) is the proxy_app
      //   URL. That is, each home directory can only be used for one
      //   running and communicating pair of BDK <-> CometBFT.
      //
      std::vector<std::string> cometArgs = {
        "start",
        "--abci=socket",
        "--proxy_app=unix://" + cometUNIXSocketPath,
        "--home=" + cometPath
      };

      startCometBFT(cometArgs, false);

    } catch (const std::exception& ex) {
      setErrorCode(CometError::RUN);
      throw DynamicException("Exception caught when trying to run cometbft start: " + std::string(ex.what()));
    }

    setState(CometState::STARTED_COMET);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Test cometbft: check that the node has started successfully.

    setState(CometState::TESTING_COMET);

    // First, wait to receive an ABCI Info callback, which is part of the initial handshake sequence,
    //   as cometbft has to know what is the current state (e.g. current block height) of the ABCI app.
    uint64_t testingStartTime = Utils::getCurrentTimeMillisSinceEpoch();
    while (!stop_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      // Info is a reliable call; cometbft always calls it to figure out what
      //  is the last block height that we have so it will e.g. replay from there.
      if (this->infoCount_ > 0) {
        break;
      }
      // Time out in 10 seconds, should never happen
      uint64_t currentTime = Utils::getCurrentTimeMillisSinceEpoch();
      if (currentTime - testingStartTime >= 10000) {
        setErrorCode(CometError::ABCI_TIMEOUT);
        throw DynamicException("Timed out while waiting for an Info call from cometbft");
      }
    }

    // Check if quitting
    if (stop_) break;

    // Start RPC connection
    LOGDEBUG("Will connect to cometbft RPC at port: " + std::to_string(rpcPort_));
    int rpcTries = 50; //5s
    bool rpcSuccess = false;
    while (rpcTries-- > 0 && !stop_) {
      // wait first, otherwise 1st connection attempt always fails
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (rpc_.rpcStartConnection(rpcPort_)) {
        rpcSuccess = true;
        break;
      }
      LOGDEBUG("Retrying RPC connection: " + std::to_string(rpcTries));
    }

    // Check if quitting
    if (stop_) break;

    // So, it turns out that in some kinds of failed-to-start node scenarios, the RPC port will not be available.
    // As far as we know, some of these failure modes may be recoverable with a retry.
    if (!rpcSuccess) {
      setErrorCode(CometError::RPC_TIMEOUT);
      throw DynamicException("Can't connect to the cometbft RPC port (RPC test).");
    }

    // Test the RPC connection (and also check node health)
    // Make a sample RPC call using the persisted connection
    json healthResult;
    if (!rpc_.rpcSyncCall("health", json::object(), healthResult)) {
      setErrorCode(CometError::RPC_CALL_FAILED);
      throw DynamicException("ERROR: cometbft RPC health call failed: " + healthResult.dump());
    }
    LOGDEBUG("cometbft RPC health call returned OK: " + healthResult.dump());

    setState(CometState::TESTED_COMET);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Main loop.
    // If there are queued requests, send them to the comet process.

    setState(CometState::RUNNING);

    // NOTE: If this loop breaks for whatever reason without !stop being true, we will be
    //       in the TERMINATED state, which is there to catch bugs.
    while (!stop_) {

      // --------------------------------------------------------------------------------
      // The ABCI connection can die if e.g. cometbft errors out and decides to close it or if
      //   the cometbft process dies. In any case, the ABCI connection closing means this
      //   run of the cometbft process is over.
      // --------------------------------------------------------------------------------

      if (!abciServer_->running()) {
        setErrorCode(CometError::ABCI_SERVER_FAILED);
        throw DynamicException("ABCIServer is not running (ABCI connection with cometbft has been closed.");
      }

      // --------------------------------------------------------------------------------
      // Check RPC connection health and power-cycle it if needed.
      // Only throw an error if the RPC connection can't be immediately reestablished,
      //   which might mean the cometbft process died.
      // In any case, being unable to connect to the RPC port is sufficiently bad to
      //   warrant failing the entire driver connection.
      // --------------------------------------------------------------------------------

      if (!rpc_.rpcCheckConnection()) {
        LOGWARNING("Cometbft RPC connection (RPC port: " + std::to_string(rpcPort_) + ") has failed or closed. Will attempt to restart it.");
        LOGINFO("Cometbft RPC connection: stopping...");
        rpc_.rpcStopConnection();
        LOGINFO("Cometbft RPC connection: stopped. Restarting...");
        if (!rpc_.rpcStartConnection()) {
          // Can't reboot the RPC connection for whatever reason (maybe the cometbft process just crashed)
          setErrorCode(CometError::RPC_TIMEOUT);
          throw DynamicException("Can't reconnect to the cometbft RPC port.");
        }
        LOGINFO("Cometbft RPC connection restarted successfully.");
      }

      // --------------------------------------------------------------------------------
      // Process all ready-to-read async RPC responses here
      // Interrupted if stop_ is detected
      // --------------------------------------------------------------------------------

      CometRPCRequestType requestData;
      json requestResponseJson;
      uint64_t requestId;
      while (!stop_) {

        // Fetch an async-call RPC response (any one)
        requestId = rpc_.rpcGetNextAsyncResponse(requestResponseJson, requestData);
        if (requestId == 0) {
          break; // Done pumping out all pending responses, so wait a bit, check quit flag, ...
        }
        bool requestSuccess = ! requestResponseJson.contains("error");

        // Process the RPC response, which you know what it is depending on the type of the requestData
        // You MUST define a new requestData type for each supported async RPC call, since all async RPC
        // calls have their responses pulled here, and the type of the response that is pulled determines
        // which CometListener callback method gets invoked.
        std::visit([&](auto&& arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, TxSendType>) {
            const std::shared_ptr<Hash>& txEthHashPtr = std::get<0>(arg);
            const Bytes& tx = std::get<1>(arg);

            std::string txHash = "";
            if (requestSuccess && requestResponseJson.contains("result") && requestResponseJson["result"].contains("hash")) {
              txHash = requestResponseJson["result"]["hash"].get<std::string>();
            }

            // If txCache is enabled and we enabled sha3 computation for outgoing txs,
            // find the cache entry and update its state.
            if (txCacheSize_ > 0 && txEthHashPtr) {
              // Search active bucket first
              std::scoped_lock lock(txCacheMutex_);
              for (int i = 0; i < 2; ++i) {
                auto& bucket = txCache_[(txCacheBucket_ + i) % 2];
                auto it = bucket.find(*txEthHashPtr);
                if (it != bucket.end()) {
                  CometTxStatus& txStatus = it->second;
                  if (requestSuccess) {
                    txStatus.height = CometTxStatusHeight::SUBMITTED;
                    txStatus.cometTxHash = txHash;
                  } else {
                    // cometTxHash is not returned in the RPC response in case of error
                    txStatus.height = CometTxStatusHeight::REJECTED;
                  }
                  break;
                }
              }
            }

            listener_->sendTransactionResult(requestId, tx, requestSuccess, txHash, requestResponseJson);

          } else if constexpr (std::is_same_v<T, TxCheckType>) {
            const std::string& txHash = arg;
            listener_->checkTransactionResult(requestId, txHash, requestSuccess, requestResponseJson);

          } else if constexpr (std::is_same_v<T, DefaultAsyncRPCType>) {
            const std::string& method = std::get<0>(arg);
            const json& params = std::get<1>(arg);

            listener_->rpcAsyncCallResult(requestId, method, params, requestSuccess, requestResponseJson);

          } else {
            // Bug, should never happen.
            // For example, rpcGetNextAsyncResponse() should not return any request marked with T{} (from sync calls)
            LOGERROR("Internal error: got unknown RPC result type!");
          }
        }, requestData);
      }

      // --------------------------------------------------------------------------------
      // Wait a bit before we check everything again
      // --------------------------------------------------------------------------------

      if (stop_) break;

      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // --------------------------------------------------------------------------------------
    // If the main loop above exits and this is reached, it is because we are shutting down.
    // Will shut down cometbft, clean up and break loop.
    // stop_ *must* be set to true at this point.
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Reaching here is a bug as stop_ *must* have been set to true.
    // This is useful to detect bugs in the RUNNING state loop above.

    setErrorCode(CometError::FATAL);
    throw DynamicException("Comet worker: exiting (loop end reached); this is an error!");
  }

  LOGDEBUG("Comet worker: exiting (quit loop)");
}

// CometImpl's ABCIHandler implementation
//
// NOTE: the "bytes" field in a Protobuf .proto file is implemented as a std::string, which
//       is terrible. there is some support for absl::Cord for non-repeat bytes fields in
//       version 23 of Protobuf, and repeat bytes with absl::Cord is already supported internally
//       but is unreleased. not exactly sure how that would improve interfacing with our Bytes
//       (vector<uint8_t>) convention (maybe we'd change the Bytes typedef to absl::Cord), but
//       in the meantime we have to convert everything which involves one extra memory copy step
//       for every instance of a bytes field.

// Helper to convert the C++ implementation of a Protobuf "repeat bytes" field to std::vector<std::vector<uint8_t>>
void toBytesVector(const google::protobuf::RepeatedPtrField<std::string>& repeatedField, std::vector<Bytes>& result) {
  for (const auto& str : repeatedField) {
    result.emplace_back(str.begin(), str.end());
  }
}

// Helper to convert the C++ implementation of a Protobuf "bytes" field to std::vector<uint8_t>
Bytes toBytes(const std::string& str) {
  return Bytes(str.begin(), str.end());
}

// Helper to convert std::vector<uint8_t> to the C++ implementation of a Protobuf "bytes" field
std::string toStringForProtobuf(const Bytes& bytes) {
  return std::string(bytes.begin(), bytes.end());
}

// Helper to convert a google.protobuf.Timestamp to uint64_t nanoseconds since epoch.
uint64_t toNanosSinceEpoch(const google::protobuf::Timestamp& timestamp) {
  uint64_t nanoseconds_since_epoch = static_cast<uint64_t>(timestamp.seconds()) * 1000000000ULL + static_cast<uint64_t>(timestamp.nanos());
  return nanoseconds_since_epoch;
}

void CometImpl::echo(const cometbft::abci::v1::EchoRequest& req, cometbft::abci::v1::EchoResponse* res) {
  //res->set_message(req.message()); // This is done at the net/abci caller, we don't need to do it here.
  // This callback doesn't seem to be called for ABCI Sockets vs. ABCI gRPC? Not sure what's going on.
}

void CometImpl::flush(const cometbft::abci::v1::FlushRequest& req, cometbft::abci::v1::FlushResponse* res) {
  // Nothing to do for now as all handlers should be synchronous.
}

void CometImpl::info(const cometbft::abci::v1::InfoRequest& req, cometbft::abci::v1::InfoResponse* res) {
  uint64_t height;
  Bytes hashBytes;
  std::string appSemVer;
  uint64_t appVersion;
  listener_->getCurrentState(height, hashBytes, appSemVer, appVersion);

  // We cannot pass to cometbft an application height that is AHEAD of its block store. if we do that,
  // then cometbft just panics. Not sure if there's a way to force cometbft to just sync the block store
  // silently until it catches up to the application state.
  //
  // If the application hasn't reacted appropriately to the CometListener::currentCometBFTHeight() callback,
  // here we will have to error out, which is better than having cometbft error out downstream from this.
  if (lastCometBFTBlockHeight_ < height) {
    LOGFATALP_THROW("FATAL: Comet app height is " + std::to_string(height) + " but cometbft block store is at previous height " + std::to_string(lastCometBFTBlockHeight_));
  }

  std::string hashString(hashBytes.begin(), hashBytes.end());
  res->set_version(appSemVer);
  res->set_app_version(appVersion);
  res->set_last_block_height(height);
  res->set_last_block_app_hash(hashString);
  ++infoCount_;
}

void CometImpl::init_chain(const cometbft::abci::v1::InitChainRequest& req, cometbft::abci::v1::InitChainResponse* res) {
  std::vector<CometValidatorUpdate> validatorUpdates;
  for (const auto& update : req.validators()) {
    CometValidatorUpdate validatorUpdate;
    validatorUpdate.publicKey = toBytes(update.pub_key_bytes());
    validatorUpdate.power = update.power();
    validatorUpdates.push_back(validatorUpdate);
  }

  Bytes hashBytes;
  listener_->initChain(req.time().seconds(), req.chain_id(), toBytes(req.app_state_bytes()), req.initial_height(), validatorUpdates, hashBytes);

  std::string hashString(hashBytes.begin(), hashBytes.end());
  res->set_app_hash(hashString);

  auto* consensus_params = res->mutable_consensus_params();

  // NOTE: initial_height, block params, validator params, evidence params, validator pub key types, etc.
  // all that should be is in the genesis file so we assume we don't have to update or confirm these here.

  // If we'd want to override what the genesis file is requesting w.r.t block limits, this is how we'd do it.
  //
  //auto* block_params = consensus_params->mutable_block();
  //block_params->set_max_bytes(COMETBFT_BLOCK_PARAM_MAX_BYTES);
  //block_params->set_max_gas(COMETBFT_BLOCK_PARAM_MAX_GAS);

  // NOTE: From the cometbft docs:
  //  If InitChainResponse.Validators is empty, the initial validator set will be the InitChainRequest.Validators
  // Meaning we don't want to set validator nodes here; we just always accept what the genesis file provides.

  auto* feature_params = consensus_params->mutable_feature();

  // TODO/REVIEW: If we enable Vote Extensions (which we probably want to enable even if we aren't using them
  // for anything) here, we need to actually fill in vote extensions related ABCI request and response parameters
  // otherwise it won't work (you'll get consensus failures).
  //auto* vote_extensions_height = feature_params->mutable_vote_extensions_enable_height();
  //vote_extensions_height->set_value(1);

  // Enable PBTS from block #1 and onwards, and configure its consensus parameters
  auto* pbts_enable_height = feature_params->mutable_pbts_enable_height();
  pbts_enable_height->set_value(1);
  auto* synchrony_params = res->mutable_consensus_params()->mutable_synchrony();
  auto* precision = synchrony_params->mutable_precision();
  precision->set_seconds(COMETBFT_PBTS_SYNCHRONY_PARAM_PRECISION_SECONDS);
  precision->set_nanos(0);
  auto* message_delay = synchrony_params->mutable_message_delay();
  message_delay->set_seconds(COMETBFT_PBTS_SYNCHRONY_PARAM_MESSAGE_DELAY_SECONDS);
  message_delay->set_nanos(0);
}

void CometImpl::prepare_proposal(const cometbft::abci::v1::PrepareProposalRequest& req, cometbft::abci::v1::PrepareProposalResponse* res) {
  std::unordered_set<size_t> delTxIds;
  std::vector<Bytes> allTxs;
  toBytesVector(req.txs(), allTxs);
  listener_->buildBlockProposal(allTxs, delTxIds);
  for (size_t i = 0; i < req.txs().size(); ++i) {
    if (delTxIds.find(i) == delTxIds.end()) {
      res->add_txs(req.txs()[i]);
    }
  }
}

void CometImpl::process_proposal(const cometbft::abci::v1::ProcessProposalRequest& req, cometbft::abci::v1::ProcessProposalResponse* res) {
  bool accept = false;
  std::vector<Bytes> allTxs;
  toBytesVector(req.txs(), allTxs);
  listener_->validateBlockProposal(req.height(), allTxs, accept);
  if (accept) {
    res->set_status(cometbft::abci::v1::PROCESS_PROPOSAL_STATUS_ACCEPT);
  } else {
    res->set_status(cometbft::abci::v1::PROCESS_PROPOSAL_STATUS_REJECT);
  }
}

void CometImpl::check_tx(const cometbft::abci::v1::CheckTxRequest& req, cometbft::abci::v1::CheckTxResponse* res) {
  bool accept = false;
  int64_t gasWanted = -1;
  listener_->checkTx(toBytes(req.tx()), gasWanted, accept);
  int ret_code = 0;
  if (!accept) { ret_code = 1; }
  res->set_code(ret_code);
  if (gasWanted != -1) {
    res->set_gas_wanted(gasWanted);
  }
}

void CometImpl::commit(const cometbft::abci::v1::CommitRequest& req, cometbft::abci::v1::CommitResponse* res) {
  uint64_t height;
  listener_->getBlockRetainHeight(height);
  res->set_retain_height(height);
}

void CometImpl::finalize_block(const cometbft::abci::v1::FinalizeBlockRequest& req, cometbft::abci::v1::FinalizeBlockResponse* res) {
  Bytes hashBytes;
  std::vector<CometExecTxResult> txResults;
  std::vector<CometValidatorUpdate> validatorUpdates;
  std::vector<Bytes> allTxs;
  toBytesVector(req.txs(), allTxs);
  listener_->incomingBlock(
    req.height(), req.syncing_to_height(), allTxs, toBytes(req.proposer_address()), toNanosSinceEpoch(req.time()),
    hashBytes, txResults, validatorUpdates
  );

  // application must give us one txResult entry for every tx entry we have given it.
  if (txResults.size() != req.txs().size()) {
    LOGFATALP_THROW("FATAL: Comet incomingBlock got " + std::to_string(txResults.size()) + " txResults but txs size is " + std::to_string(req.txs().size()));
  }

  std::string hashString(hashBytes.begin(), hashBytes.end());
  res->set_app_hash(hashString);

  // TODO/REVIEW: Check if we need to expose more ExecTxResult fields to the application.
  for (int32_t i = 0; i < req.txs().size(); ++i) {
    cometbft::abci::v1::ExecTxResult* tx_result = res->add_tx_results();
    CometExecTxResult& txRes = txResults[i];
    tx_result->set_code(txRes.code);
    tx_result->set_data(toStringForProtobuf(txRes.data));
    tx_result->set_gas_wanted(txRes.gasWanted);
    tx_result->set_gas_used(txRes.gasUsed);

    // If the transaction cache is enabled, we can fill in the result of each transaction
    // as their execution and inclusion in a block is definitive.
    if (txCacheSize_ > 0) {
      // REVIEW: Should we instead acquire this mutex only once and block the
      // tx cache for the whole transaction processing loop?
      std::scoped_lock lock(txCacheMutex_);
      Hash txEthHash = Utils::sha3(allTxs[i]);
      CometTxStatus* txStatusPtr = nullptr;
      for (int j = 0; j < 2; ++j) {
        auto& bucket = txCache_[(txCacheBucket_ + j) % 2];
        auto it = bucket.find(txEthHash);
        if (it != bucket.end()) {
          txStatusPtr = &it->second;
          break;
        }
      }
      CometTxStatus& txStatus = txStatusPtr ? *txStatusPtr : txCache_[txCacheBucket_][txEthHash];
      txStatus.height = req.height();
      txStatus.index = i;
      txStatus.result = txRes;
      // We don't know the CometBFT hex hash Bytes here as that's only returned by the broadcast_tx_async RPC method.
      // If there was a cache entry before with this field set, then you have it, otherwise you need to compute the value if you need it.
      //txStatus.txHash = ...
    }
  }

  // Relay validator update commands to cometbft
  for (const auto& validatorUpdate : validatorUpdates) {
    auto* update = res->add_validator_updates();
    update->set_power(validatorUpdate.power);
    update->set_pub_key_type(COMET_PUB_KEY_TYPE);
    update->set_pub_key_bytes(toStringForProtobuf(validatorUpdate.publicKey));
  }

  // TODO: The Application can use FinalizeBlockRequest.decided_last_commit and FinalizeBlockRequest.misbehavior
  // to determine rewards and punishments for the validators.

  // TODO: We will have to support consensus param updates, which are done through here.
}

void CometImpl::query(const cometbft::abci::v1::QueryRequest& req, cometbft::abci::v1::QueryResponse* res) {
  // Absorbed internally, may be used to implement other callbacks.

  // From the CometBFT doc:
  //  Query is a generic method with lots of flexibility to enable diverse sets of queries on application state.
  //
  //  The most important use of Query is to return Merkle proofs of the application state at some height that can be used for efficient application-specific light-clients.
  //
  //  CometBFT makes use of Query to filter new peers based on ID and IP, and exposes Query to the user over RPC.
  //  When CometBFT connects to a peer, it sends two queries to the ABCI application using the following paths, with no additional data:
  //   /p2p/filter/addr/<IP:PORT>, where <IP:PORT> denote the IP address and the port of the connection
  //   p2p/filter/id/<ID>, where <ID> is the peer node ID (ie. the pubkey.Address() for the peer’s PubKey)
  //  If either of these queries return a non-zero ABCI code, CometBFT will refuse to connect to the peer.
  //
  //  Note CometBFT has technically no requirements from the Query message for normal operation - that is, the ABCI app developer need not implement Query functionality if they do not wish to.

  // TODO:
  // - Implement query for light-client use (e.g. snapshot/state-sync).
  // - Implement query for peer ID/IP rejection/filter (e.g. an IP address blacklist).
}

void CometImpl::list_snapshots(const cometbft::abci::v1::ListSnapshotsRequest& req, cometbft::abci::v1::ListSnapshotsResponse* res) {
  // TODO
}

void CometImpl::offer_snapshot(const cometbft::abci::v1::OfferSnapshotRequest& req, cometbft::abci::v1::OfferSnapshotResponse* res) {
  // TODO
}

void CometImpl::load_snapshot_chunk(const cometbft::abci::v1::LoadSnapshotChunkRequest& req, cometbft::abci::v1::LoadSnapshotChunkResponse* res) {
  // TODO
}

void CometImpl::apply_snapshot_chunk(const cometbft::abci::v1::ApplySnapshotChunkRequest& req, cometbft::abci::v1::ApplySnapshotChunkResponse* res) {
  // TODO
}

void CometImpl::extend_vote(const cometbft::abci::v1::ExtendVoteRequest& req, cometbft::abci::v1::ExtendVoteResponse* res) {
  // TODO -- not sure if this will be needed
}

void CometImpl::verify_vote_extension(const cometbft::abci::v1::VerifyVoteExtensionRequest& req, cometbft::abci::v1::VerifyVoteExtensionResponse* res) {
  // TODO -- not sure if this will be needed
}

// ---------------------------------------------------------------------------------------
// Comet class
// ---------------------------------------------------------------------------------------

Comet::Comet(CometListener* listener, std::string instanceIdStr, const Options& options, bool stepMode)
  : instanceIdStr_(instanceIdStr)
{
  impl_ = std::make_unique<CometImpl>(listener, instanceIdStr, options, stepMode);
}

Comet::~Comet() {
  impl_->stop();
}

void Comet::setTransactionCacheSize(const uint64_t cacheSize) {
  impl_->setTransactionCacheSize(cacheSize);
}

bool Comet::getStatus() {
  return impl_->getStatus();
}

const std::string& Comet::getErrorStr() {
  return impl_->getErrorStr();
}

CometError Comet::getErrorCode() {
  return impl_->getErrorCode();
}

CometState Comet::getState() {
  return impl_->getState();
}

void Comet::setPauseState(const CometState pauseState) {
  return impl_->setPauseState(pauseState);
}

CometState Comet::getPauseState() {
  return impl_->getPauseState();
}

std::string Comet::waitPauseState(uint64_t timeoutMillis) {
  return impl_->waitPauseState(timeoutMillis);
}

std::string Comet::getNodeID() {
  return impl_->getNodeID();
}

uint64_t Comet::sendTransaction(const Bytes& tx, std::shared_ptr<Hash>* ethHash) {
  return impl_->sendTransaction(tx, ethHash);
}

uint64_t Comet::checkTransaction(const std::string& txHash) {
  return impl_->checkTransaction(txHash);
}

bool Comet::checkTransactionInCache(const Hash& txEthHash, CometTxStatus& txStatus) {
  return impl_->checkTransactionInCache(txEthHash, txStatus);
}

bool Comet::rpcSyncCall(const std::string& method, const json& params, json& outResult) {
  return impl_->rpcSyncCall(method, params, outResult);
}

uint64_t Comet::rpcAsyncCall(const std::string& method, const json& params) {
  return impl_->rpcAsyncCall(method, params);
}

bool Comet::start() {
  return impl_->start();
}

bool Comet::stop() {
  return impl_->stop();
}

