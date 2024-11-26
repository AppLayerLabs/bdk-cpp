/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "comet.h"

#include "../utils/logger.h"
#include "../libs/toml.hpp"

#include "../net/abci/abciserver.h"
#include "../net/abci/abcihandler.h"   // ???

#include <boost/beast.hpp>
#include <boost/asio.hpp>

#include "../libs/base64.hpp"

/*
  NOTE: cometbft public key vs. address

  Public Key types are hard-coded in the Comet driver.

  Address
  Address is a type alias of a slice of bytes. The address is calculated by hashing the public key using sha256 and
  truncating it to only use the first 20 bytes of the slice.

  const (
    TruncatedSize = 20
  )

  func SumTruncated(bz []byte) []byte {
    hash := sha256.Sum256(bz)
    return hash[:TruncatedSize]
  }
*/

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
class CometImpl : public Log::LogicalLocationProvider, public ABCIHandler {
  private:
    CometListener* listener_; ///< Comet class application event listener/handler
    const std::string instanceIdStr_; ///< Identifier for logging
    const Options options_; ///< Copy of the supplied Options.
    const bool stepMode_; ///< Set to 'true' if unit testing with cometbft in step mode (no empty blocks).

    std::unique_ptr<ABCIServer> abciServer_; ///< TCP server for cometbft ABCI connection
    std::optional<boost::process::child> process_; ///< boost::process that points to the running "cometbft start" process

    std::future<void> loopFuture_; ///< Future object holding the consensus engine thread.
    std::atomic<bool> stop_ = false; ///< Flag for stopping the consensus engine thread.
    std::atomic<bool> status_ = true; ///< Global status (true = OK, false = failed/terminated).
    std::string errorStr_; ///< Error message (if any).

    std::atomic<CometState> state_ = CometState::STOPPED; ///< Current step the Comet instance is in.
    std::atomic<CometState> pauseState_ = CometState::NONE; ///< Step to pause/hold the comet instance at, if any.

    std::atomic<int> infoCount_ = 0; ///< Simple counter for how many cometbft Info requests we got.

    std::atomic<int> rpcPort_ = 0; ///< RPC port that will be used by our cometbft instance.

    std::mutex nodeIdMutex_; ///< mutex to protect reading/writing nodeId_.
    std::string nodeId_; ///< Cometbft node id or an empty string if not yet retrieved.

    std::mutex txOutMutex_; ///< mutex to protect txOut_.
    uint64_t txOutTicketGen_ = 0; ///< ticket generator for sendTransaction().
    std::deque<std::tuple<uint64_t, Bytes>> txOut_; ///< Queue of (ticket#,Tx) pending dispatch to the local cometbft node's mempool.

    std::mutex txCheckMutex_; ///< mutex to protect txCheck_.
    std::deque<std::string> txCheck_; ///< Queue of txHash (SHA256/cometbft) pending check via a call to /tx to the cometbft RPC port.

    uint64_t lastCometBFTBlockHeight_; ///< Current block height stored in the cometbft data dir, if known.
    std::string lastCometBFTAppHash_; ///< Current app hash stored in the cometbft data dir, if known.

    // FIXME/TODO: remove connected_ and use socket_ instead ; socket_.reset() to set it to disconnected
    //             rename rpcIoc rpcSocket rpcRequestIdCounter
    boost::asio::io_context ioc_; ///< io_context for our persisted RPC socket connection.
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_; ///< Persisted RPC socket connection.
    bool connected_ = false; ///< Check if the RPC connection is established.
    uint64_t requestIdCounter_ = 0; ///< Client-side request ID generator for our JSON-RPC calls to cometbft.

    void setState(const CometState& state); ///< Apply a state transition, possibly pausing at the new state.
    void setError(const std::string& errorStr); ///< Signal a fatal error condition.
    void resetError(); ///< Reset internal error condition.
    void startCometBFT(const std::string& cometPath); ///< Launch the CometBFT process_ ('cometbft start' for an async node run).
    void stopCometBFT(); ///< Terminate the CometBFT process_.
    int runCometBFT(const std::string& cometPath, std::vector<std::string> cometArgs, std::string &sout, std::string &serr, std::atomic<uint64_t>* pid = nullptr); ///< Blocking run cometbft w/ args
    void workerLoop(); ///< Worker loop responsible for establishing and managing a connection to cometbft.
    void workerLoopInner(); ///< Called by workerLoop().

    bool startRPCConnection() {
      if (rpcPort_ == 0) return false; // we have not figured out what the rpcPort_ configuration is yet
      if (connected_) return true;
      try {
          socket_ = std::make_unique<boost::asio::ip::tcp::socket>(ioc_);
            boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::make_address("127.0.0.1"), rpcPort_
          );
          socket_->connect(endpoint);
          connected_ = true;
          return true;
      } catch (const std::exception& e) {
          connected_ = false;
          return false;
      }
    }

    void stopRPCConnection() {
        if (socket_ && connected_) {
            boost::system::error_code ec;
            socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            connected_ = false;
        }
    }

    bool makeJSONRPCCall(const std::string& method, const json& params, std::string& outResult, bool retry = true) {
        if (!connected_ && !startRPCConnection()) {
            outResult = "Connection failed";
            return false;
        }

        int requestId = ++requestIdCounter_;  // Generate a unique request ID
        json requestBody = {
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params},
            {"id", requestId}
        };

        try {
            // Set up the POST request with keep-alive
            boost::beast::http::request<boost::beast::http::string_body> req{
                boost::beast::http::verb::post, "/", 11};
            req.set(boost::beast::http::field::host, "localhost");
            req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(boost::beast::http::field::accept, "application/json");
            req.set(boost::beast::http::field::content_type, "application/json");
            req.set(boost::beast::http::field::connection, "keep-alive");  // Keep connection open
            req.body() = requestBody.dump();  // Serialize JSON to string
            req.prepare_payload();

            boost::beast::http::write(*socket_, req);

            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::dynamic_body> res;

            boost::beast::http::read(*socket_, buffer, res);

          // This is broken, we have to always return json.
          //if (res.result() != boost::beast::http::status::ok) {
          //    LOGDEBUG("ACTUAL RESULT STRING: [" + boost::beast::buffers_to_string(res.body().data()) + "]" );
          //    outResult = "HTTP error: " + std::to_string(res.result_int());
          //    return false;
          // }
          outResult = boost::beast::buffers_to_string(res.body().data());
          auto jsonResponse = json::parse(outResult);

          // Check both the JSON-RPC error field and the HTTP return code to ensure we don't miss errors.
          return ! ( jsonResponse.contains("error") || (res.result() != boost::beast::http::status::ok) );

        } catch (const std::exception& e) {
            stopRPCConnection();  // Close connection on error
            outResult = "Exception occurred: " + std::string(e.what());
            // Retry the call once, in case it is some transient error
            // TODO: review this; this retry is not be strictly necessary as
            //       the caller should handle failures instead
            if (retry && startRPCConnection()) {
                return makeJSONRPCCall(method, params, outResult, false);
            }
            return false;
        }
    }

  public:

    std::string getLogicalLocation() const override { return instanceIdStr_; } ///< Log instance

    explicit CometImpl(CometListener* listener, std::string instanceIdStr, const Options& options, bool stepMode);

    virtual ~CometImpl();

    bool getStatus();

    const std::string& getErrorStr();

    CometState getState();

    void setPauseState(const CometState pauseState = CometState::NONE);

    CometState getPauseState();

    std::string waitPauseState(uint64_t timeoutMillis);

    std::string getNodeID();

    void start();

    void stop();

    uint64_t sendTransaction(const Bytes& tx);

    void checkTransaction(const std::string& txHash);

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
  : listener_(listener), instanceIdStr_(instanceIdStr), options_(options), stepMode_(stepMode)
{
}

CometImpl::~CometImpl() {
  stop();
}

bool CometImpl::getStatus() {
  return status_;
}

const std::string& CometImpl::getErrorStr() {
  return errorStr_;
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

uint64_t CometImpl::sendTransaction(const Bytes& tx) {
  // Add transaction to a queue that will try to push it to our cometbft instance.
  // Queue is NEVER reset on continue; etc. it is the same node, need to deliver tx to it eventually.
  // NOTE: sendTransaction is about queuing the tx object so that it is eventually dispatched to
  //       the localhost cometbft node. the only guarantee is that the local cometbft node will see it.
  //       it does NOT mean it is accepted by the application's blockchain network or anything else.
  std::lock_guard<std::mutex> lock(txOutMutex_);
  ++txOutTicketGen_;
  txOut_.emplace_back(txOutTicketGen_, tx);
  return txOutTicketGen_;
}

void CometImpl::checkTransaction(const std::string& txHash) {

  // FIXME/TODO:
  // Turns out the cometbft /tx RPC endpoint is not ideal, as it returns the entire transaction body
  // when we are checking for the status of a transaction, and it also takes some time between
  // seeing that the transaction went into a block and indexing it (it just says "not found"/error
  // if the transaction is pending, i.e. as if it didn't exist at all). So, it's better to put a cache
  // in front of the RPC request, so that if we know the transaction hash (because it is a transaction
  // that we sent through here) then we instead wait to see it in a block, and when we do, we update
  // our own transaction result tracking structure that we query here and return via a direct
  // checkTransactionResult() callback from here (or we still put it the request in the queue here and
  // let it resolve in the worker thread).
  // OR, alternatively, we use our uint64_t tx ticket, or let the application handle this with its
  // own scanning of produced blocks and indexing the transactions it sees there (since it does get
  // CometListener::incomingBlock() after all, which would be sufficient to implement any transaction
  // result/checking scheme).
  //
  // Since the execution environment (cometbft app) is going to use its own hashing function to
  // generate transaction hashes (say sha3/keccak256), the application should probably also keep
  // track, on its own, of pushed transactions, indexed by its own idea of what the transaction hash
  // is, and keep its own database of completed transactions as it receives e.g. incomingBlock() calls.

  std::lock_guard<std::mutex> lock(txCheckMutex_);
  txCheck_.emplace_back(txHash);
}

void CometImpl::start() {
  if (!this->loopFuture_.valid()) {
    this->stop_ = false;
    resetError(); // ensure error status is off
    setState(CometState::STARTED);
    this->loopFuture_ = std::async(std::launch::async, &CometImpl::workerLoop, this);
  }
}

void CometImpl::stop() {

  if (this->loopFuture_.valid()) {
    this->stop_ = true;

    // Stop the RPC connection
    stopRPCConnection();

    // (1)
    // We should stop the cometbft process first; we are a server to the consensus engine,
    //   and the side making requests should be the one to be shut down.
    // We can use the stop_ flag on our side to know that we are stopping as we service
    //   ABCI callbacks from the consensus engine, but it probably isn't needed.
    //
    // It actually does not make sense to shut down the gRPC server from our end, since
    //   the cometbft process will, in this case, simply attempt to re-establish the
    //   connection with the ABCI application (the gRPC server); it assumes it is running
    //   under some sort of process control that restarts it in case it fails.
    //
    // CometBFT should receive SIGTERM and then just terminate cleanly, including sending a
    //   socket disconnection or shutdown message to us, which should ultimately allow us
    //   to wind down our end of the gRPC connection gracefully.

    stopCometBFT();

    // (2)
    // We should know cometbft has disconnected from us when:
    //
    //     grpcServerRunning_ == false
    //
    // Which means the grpcServer_->Wait() call has exited, which it should, if the gRPC server
    //   on the other end has disconnected from us or died.
    //
    // TODO: We shouldn't rely on this, though. If a timeout elapses, we should consider
    //       doing something else such as forcing kill -9 on cometbft, instead of looping
    //       forever here without a timeout check.
    //
    if (abciServer_) {
      LOGDEBUG("Waiting for abciServer_ networking to stop running (a side-effect of the cometbft process exiting.)");
      while (abciServer_->running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
      }
      LOGDEBUG("abciServer_ networking has stopped running, we can now stop the ABCI net engine.");
      abciServer_->stop();
      LOGDEBUG("abciServer_ networking engine stopped.");
      abciServer_.reset();
      LOGDEBUG("abciServer_ networking engine destroyed.");
    } else {
      LOGDEBUG("No abciServer_ instance, so nothing to do.");
    }

    this->setPauseState(); // must reset any pause state otherwise it won't ever finish
    this->loopFuture_.wait();
    this->loopFuture_.get();
    resetError(); // stop() clears any error status
    setState(CometState::STOPPED);
  }
}

void CometImpl::setState(const CometState& state) {
  LOGTRACE("Set comet state: " + std::to_string((int)state));
  this->state_ = state;
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
  this->status_ = false;
}

void CometImpl::resetError() {
  this->status_ = true;
  this->errorStr_ = "";
}

void CometImpl::startCometBFT(const std::string& cometPath) {
  if (!abciServer_) {
    LOGFATALP_THROW("Cannot call startCometBFT() without having first started the ABCI app server.");
  }
  // Search for the executable in the system's PATH
  boost::filesystem::path exec_path = boost::process::search_path("cometbft");
  if (exec_path.empty()) {
    // This is a non-recoverable error
    // The ABCI server will be stopped/collected during stop()
    setError("cometbft executable not found in system PATH");
    return;
  }

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
    "--proxy_app=unix://" + abciServer_->getSocketPath(),
    "--home=" + cometPath
  };
  std::string argsString;
  for (const auto& arg : cometArgs) { argsString += arg + " "; }
  LOGDEBUG("Launching cometbft with arguments: " + argsString);

  // Launch the process
  auto bpout = std::make_shared<boost::process::ipstream>();
  auto bperr = std::make_shared<boost::process::ipstream>();
  process_ = boost::process::child(
    exec_path,
    boost::process::args(cometArgs),
    boost::process::std_out > *bpout,
    boost::process::std_err > *bperr
  );
  std::string pidStr = std::to_string(process_->id());
  LOGDEBUG("cometbft start launched with PID: " + pidStr);

  // Spawn two detached threads to pump stdout and stderr to bdk.log.
  // They should go away naturally when process_ is terminated.
  std::thread stdout_thread([bpout, pidStr]() {
    std::string line;
    while (*bpout && std::getline(*bpout, line) && !line.empty()) {
      // REVIEW: may want to upgrade this to DEBUG
      GLOGTRACE("[cometbft stdout]: " + line);
    }
    GLOGDEBUG("cometbft stdout stream pump thread finished, cometbft pid = " + pidStr);
  });
  std::thread stderr_thread([bperr, pidStr]() {
    std::string line;
    while (*bperr && std::getline(*bperr, line) && !line.empty()) {
      GLOGDEBUG("[cometbft stderr]: " + line);
    }
    GLOGDEBUG("cometbft stderr stream pump thread finished, cometbft pid = " + pidStr);
  });
  stdout_thread.detach();
  stderr_thread.detach();
}

void CometImpl::stopCometBFT() {
  // process shutdown
  if (process_.has_value()) {
    LOGDEBUG("Terminating CometBFT process");
    // terminate the process
    pid_t pid = process_->id();
    try {
      process_->terminate(); // SIGTERM (graceful termination, equivalent to terminal CTRL+C)
      LOGDEBUG("Process with PID " + std::to_string(pid) + " terminated");
      process_->wait();  // Ensure the process is fully terminated
      LOGDEBUG("Process with PID " + std::to_string(pid) + " joined");
    } catch (const std::exception& ex) {
      // This is bad, and if it actually happens, we need to be able to do something else here to ensure the process disappears
      //   because we don't want a process using the data directory and using the socket ports.
      // TODO: is this the best we can do? (what about `cometbft debug kill`?)
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
    LOGXTRACE("Term end");
  }
  // we need to ensure we got rid of any process_ instance in any case so we can start another
  process_.reset();
  LOGDEBUG("CometBFT process terminated");
}

// One-shot run cometbft with the given cometArgs and block until it finishes, returning both
//  the complete stdout and stderr streams as outparams and the exit_code as a return value.
// This is to be used with e.g. "cometbft show-node-id" etc.
// Once it knows the PID, it sets the pid outparam (so it can be killed if needed)
int CometImpl::runCometBFT(const std::string& cometPath, std::vector<std::string> cometArgs, std::string &sout, std::string &serr, std::atomic<uint64_t>* pid) {
  boost::filesystem::path exec_path = boost::process::search_path("cometbft");
  if (exec_path.empty()) {
    setError("cometbft executable not found in system PATH");
    return -1;
  }
  auto bpout = std::make_shared<boost::process::ipstream>();
  auto bperr = std::make_shared<boost::process::ipstream>();
  // using a local "process" variable here since this is an one-shot run:
  boost::process::child process(
      exec_path,
      boost::process::args(cometArgs),
      boost::process::std_out > *bpout,
      boost::process::std_err > *bperr
  );
  // Set the PID as soon as the process is created
  if (pid) {
    *pid = process.id();
  }
  // these threads will pump the output streams while we wait (blocking) for cometbft to finish
  // when the process terminates, they stop looping
  std::thread stdout_thread([bpout, &sout]() {
    std::string line;
    if (std::getline(*bpout, line)) { sout += line; }
    while (std::getline(*bpout, line)) { sout += '\n' + line; }
  });
  std::thread stderr_thread([bperr, &serr]() {
    std::string line;
    if (std::getline(*bperr, line)) { serr += line; }
    while (std::getline(*bperr, line)) { serr += '\n' + line; }
  });
  process.wait(); // blocking wait until the cometbft execution completes
  if (stdout_thread.joinable()) { stdout_thread.join(); }
  if (stderr_thread.joinable()) { stderr_thread.join(); }
  int exit_code = process.exit_code();
  return exit_code;
}

void CometImpl::workerLoop() {
  LOGDEBUG("Comet worker thread: started");
  try {
    workerLoopInner();
  } catch (const std::exception& ex) {
    setError("Exception caught in comet worker thread: " + std::string(ex.what()));
  }
  LOGDEBUG("Comet worker thread: finished");
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

    // If there's an old RPC connection, make sure it is gone
    stopRPCConnection();

    // Ensure that the CometBFT process is in a terminated state.
    // This should be a no-op; it should be stopped before the continue; statement.
    stopCometBFT();

    // Ensure the ABCI Server is in a stopped/destroyed state
    abciServer_.reset();

    // Reset what we know about the cometbft store state (this is the default state if the block store is empty)
    lastCometBFTBlockHeight_ = 0;
    lastCometBFTAppHash_ = "";

    // Reset any state we might have as an ABCIHandler
    infoCount_ = 0; // needed for the TESTING_COMET -> TESTED_COMET state transition.
    rpcPort_ = 0;
    std::unique_lock<std::mutex> resetInfoLock(this->nodeIdMutex_);
    this->nodeId_ = ""; // only known after "comet/config/node_key.json" is set and "cometbft show-node-id" is run.
    resetInfoLock.unlock();

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
      throw DynamicException("Configuration option cometBFT::genesis is empty.");
    } else {
      LOGINFO("CometBFT::genesis config found: " + genesisJSON.dump());
    }

    if (!hasP2PPort) {
      throw DynamicException("Configuration option cometBFT:: p2p_port is empty.");
    } else {
      LOGINFO("CometBFT::p2p_port config found: " + p2pPortJSON.get<std::string>());
    }

    if (!hasRPCPort) {
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
      throw DynamicException("Root path not found: " + rootPath);
    }

    // --------------------------------------------------------------------------------------
    // If comet home directory does not exist inside rootPath, then create it via
    //   cometbft init. It will be created with all required options with standard values,
    //   which is what we want.

    if (!std::filesystem::exists(cometPath)) {

      LOGDEBUG("Comet worker: creating comet directory");

      // run cometbft init cometPath to create the cometbft directory with default configs
      Utils::execute("cometbft init --home " + cometPath);

      // check it exists now, otherwise halt node
      if (!std::filesystem::exists(cometPath)) {
        throw DynamicException("Could not create cometbft home directory");
      }
    }

    if (!std::filesystem::exists(cometConfigPath)) {
      // comet/config/ does not exist for some reason, which means the comet/ directory is broken
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
        throw DynamicException("Cannot open comet genesis file for writing: " + cometConfigGenesisPath);
      }
    }

    // Sanity check the existence of the config.toml file
    if (!std::filesystem::exists(cometConfigTomlPath)) {
      throw DynamicException("Comet config.toml file does not exist: " + cometConfigTomlPath);
    }

    // Open and parse the main comet config file (config.toml)
    toml::table configToml;
    try {
      configToml = toml::parse_file(cometConfigTomlPath);
    } catch (const toml::parse_error& err) {
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
      throw DynamicException("Could not open file for writing: " + cometConfigTomlPath);
    }
    configTomlOutFile << configToml;
    if (configTomlOutFile.fail()) {
      throw DynamicException("Could not write file: " + cometConfigTomlPath);
    }
    configTomlOutFile.close();
    if (configTomlOutFile.fail()) {
      throw DynamicException("Failed to close file properly: " + cometConfigTomlPath);
    }

    LOGDEBUG("Comet setting configured");

    setState(CometState::CONFIGURED);

    LOGDEBUG("Comet set configured");

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Run cometbft inspect and check that everything is as expected here (validator key,
    ///   state, ...). If anything is wrong that is fixable in a non-forceful fashion,
    //    then fix it that way and re-check, otherwise "fix it" by rm -rf the
    //    rootPath + /comet/  directory entirely and continue; this loop.
    // TODO: ensure cometbft is terminated if we are killed (prctl()?)

    setState(CometState::INSPECTING_COMET);

    // If there's a need to run cometbft inspect first, do it here.
    //
    // As we map out all the usual error conditions, we'll add more code here to catch them
    //  and recover from them. By default, recovery can be done by just wiping off the comet/
    //  directory and starting the node from scratch. Eventually, this fallback case can be
    //  augmented with e.g. using a local snapshots directory to speed up syncing.

    // Run cometbft show-node-id to figure out what the node ID is
    try {
      std::string sout, serr;
      int exitCode = runCometBFT(cometPath, { "show-node-id", "--home=" + cometPath }, sout, serr);
      if (exitCode != 0) {
        LOGERROR("ERROR: Error while attempting to fetch the cometbft node id. Exit code: " + std::to_string(exitCode) + "; stdout: " + sout + "; stderr: " + serr);
        // Continue without knowing the node ID
        // TODO: review this; this probably needs to be a setError and "continue;" to retry
      } else {
        // TODO: here we want to assert that the node ID we got is a proper hex string of the correct size
        LOGDEBUG("Got comet node ID: [" + sout + "]");
        std::scoped_lock(this->nodeIdMutex_);
        this->nodeId_ = sout;
      }
    } catch (const std::exception& ex) {
      // TODO: maybe we should attempt a restart in this case, with a retry counter?
      setError("Exception caught when trying to run cometbft start: " + std::string(ex.what()));
      return;
    }

    // Here we need to inspect the current state of the cometbft node.
    // TODO: inspect validator set and other parameters as well

    // start cometbft inspect
    std::atomic<uint64_t> inspectPid(0);
    bool inspectFailed = false;
    std::atomic<bool> inspectKilled = false;
    std::atomic<bool> gotInspectInfo = false;
    std::string inspectHeaderResult;

    auto runCometFunc = [&]() {
      std::string sout, serr;
      try {
        int exitCode = runCometBFT(cometPath, { "inspect", "--home=" + cometPath }, sout, serr, &inspectPid);
        if (inspectKilled) {
          LOGTRACE("cometbft inspect was killed (this is expected)");
          return;
        }
        if (exitCode != 0) {
          LOGERROR("ERROR: Error while attempting to run cometbft inspect. Exit code: " + std::to_string(exitCode) + "; stdout: " + sout + "; stderr: " + serr);
        }
        inspectFailed = (exitCode != 0);
      } catch (const std::exception& ex) {
        LOGERROR("Exception caught when trying to run cometbft inspect: " + std::string(ex.what()));
        inspectFailed = true;
      }
    };

    std::thread cometThread(runCometFunc);

    while (!inspectFailed && inspectPid == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (inspectFailed) {
      if (cometThread.joinable()) {
        cometThread.join();
      }
      // TODO: retry instead?
      setError("Failed to run cometbft inspect, can't know the current cometbft state");
      return;
    }

    // start RPC connection
    int inspectRpcTries = 50; //5s
    bool inspectRpcSuccess = false;
    while (inspectRpcTries-- > 0 && !stop_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (startRPCConnection()) {
        inspectRpcSuccess = true;
        break;
      }
      LOGDEBUG("Retrying RPC connection (inspect): " + std::to_string(inspectRpcTries));
    }

    if (inspectRpcSuccess) {

      // Retrieve the latest header
      gotInspectInfo = makeJSONRPCCall("header", json::object(), inspectHeaderResult);
      if (gotInspectInfo) {
        LOGDEBUG("cometbft inspect RPC header call returned OK: "+ inspectHeaderResult);
      } else {
        LOGERROR("ERROR: cometbft inspect RPC header call failed: " + inspectHeaderResult);
      }

      // Stop the RPC connection with cometbft inspect
      stopRPCConnection();
    } else {
      LOGERROR("Can't connect to the cometbft RPC port (inspect).");
    }

    // unconditionally, we need to make sure cometbft inspect is terminated
    //
    try {
      std::string killCommand = "kill -9 " + std::to_string(inspectPid);
      LOGINFO("Attempting to force kill process with PID " + std::to_string(inspectPid) + " using kill -9");
      inspectKilled = true; // when it errors out, ignore it
      int result = std::system(killCommand.c_str());
      if (result == 0) {
        LOGINFO("Successfully killed process with PID " + std::to_string(inspectPid) + " using kill -9");
      } else {
        // This is bad
        LOGERROR("Failed to kill process with PID " + std::to_string(inspectPid) + " using kill -9. Error code: " + std::to_string(result));
        setError("Could not kill cometbft inspect");
        return;
      }
    } catch (const std::exception& ex2) {
      LOGERROR("Failed to execute kill -9: " + std::string(ex2.what()));
    }

    if (cometThread.joinable()) {
      cometThread.join();
    }

    // Handle failures after we are sure cometbft inspect is terminated
    if (!inspectRpcSuccess) {
      // REVIEW: Do we really want to continue/retry here?
      //         For one Comet::start(), we can have a retry count (relaunch and do over, e.g. once (1)) and when
      //         the retry count is exhausted, we use the data recovery dir if it exists, and if
      //         that one fails two times, then we stop Comet with setError.
      //         All this behavior should be controlled via the Comet class interface (options/properties).
      LOGERROR("Will stop cometbft, relaunch cometbft and reconnect (inspect RPC failure).");
      continue;
    }
    if (!gotInspectInfo) {
      // REVIEW: Do we really want to continue/retry here?
      LOGERROR("Will stop cometbft, relaunch cometbft and reconnect (inspect RPC call failed).");
      continue;
    }

    // We got an inspect latest header response.
    try {
      bool validResponse = true;
      std::string parseError;

      // Parse the JSON result
      json response = json::parse(inspectHeaderResult);

      // Validate response structure
      if (!response.is_object() || !response.contains("result") || !response["result"].is_object()) {
        validResponse = false;
        parseError = "Invalid or missing 'result' in cometbft inspect header response.";
      }

      const auto& result = validResponse ? response["result"] : json::object();

      // Validate header structure
      if (validResponse && (!result.contains("header"))) {
        validResponse = false;
        parseError = "Invalid or missing 'header' in cometbft inspect header response.";
      }

      const auto& header = validResponse ? result["header"] : json::object();

      if (validResponse && header.is_null()) {
        // Header is null, which is valid and indicates an empty block store
        LOGDEBUG("Header is null; block store is empty. Last Block Height = 0, Last App Hash = \"\"");
      } else {
        // Validate required fields
        if (validResponse && (!header.contains("height") || !header["height"].is_string())) {
          validResponse = false;
          parseError = "Missing or invalid 'height' in header.";
        }
        if (validResponse && (!header.contains("app_hash") || !header["app_hash"].is_string())) {
          validResponse = false;
          parseError = "Missing or invalid 'app_hash' in header.";
        }
        if (validResponse) {
          // Assign values
          lastCometBFTBlockHeight_ = header["height"].get<std::string>().empty() ? 0 : std::stoull(header["height"].get<std::string>());
          lastCometBFTAppHash_ = header["app_hash"].get<std::string>();
          LOGDEBUG("Parsed header successfully: Last Block Height = " + std::to_string(lastCometBFTBlockHeight_) + ", Last App Hash = " + lastCometBFTAppHash_);
        }
      }
      if (!validResponse) {
        LOGERROR("Error parsing cometbft inspect header response: " + parseError);
        LOGERROR("Will stop cometbft, relaunch cometbft, and reconnect.");
        continue;
      }
    } catch (const std::exception& e) {
      LOGERROR(std::string("Exception occurred while parsing cometbft inspect header response: ") + e.what());
      LOGERROR("Will stop cometbft, relaunch cometbft, and reconnect.");
      continue;
    }

    // Notify the application of the height that CometBFT has in its block store.
    // If the application is ahead of this, it will need a strategy to cope with it.
    listener_->currentCometBFTHeight(lastCometBFTBlockHeight_);

    // --------------------------------------------------------------------------------------
    // Finished inspect step.

    setState(CometState::INSPECTED_COMET);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Start our cometbft application gRPC server; make sure it is started.

    setState(CometState::STARTING_ABCI);

    // start the ABCI server
    abciServer_ = std::make_unique<ABCIServer>(this, cometUNIXSocketPath);
    abciServer_->start();

    // the right thing would be to connect to ourselves here and test the connection before firing up
    //   the external engine, but a massive enough sleep here (like 1 second) should do the job.
    //   If 1s is not enough, make it e.g. 3s then. or use the GRPC async API which is significantly
    //   more complex.
    // All we are waiting here is for the other thread to be in scheduling to be able to get into Wait()
    //   and do a blocking read on a TCP listen socket that is already opened (or instantly fail on some
    //   read or create error), which is almost zero work.
    // Unless the machine's CPU is completely overloaded, a massive sleep should be able to get that
    //   scheduled in and executed.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // if it is not running by now then it is probably because starting the GRPC server failed.
    if (!abciServer_->running()) {
      LOGERROR("Comet failed: ABCI server failed to start");

      // Retry
      //
      // TODO: So, the main idea of this loop we are in is that we will continously try and retry
      //       to set up comet. But every time we do a continue; we need to make sure that makes
      //       sense, i.e. there is a chance it will work next time; log it, maybe introduce a
      //       delay, etc. OR fail permanently with an error message.
      //
      continue;
    }

    setState(CometState::STARTED_ABCI);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Run cometbft start, passing the socket address of our gRPC server as a parameter.
    // TODO: ensure cometbft is terminated if we are killed (prctl()?)
    //
    // NOTE: cannot simply continue; if we know the process_ exists; we need to handle any
    //       alive process_ in this loop iteration before a continue;
    //       the only other place where we stop process_ is in stop().

    setState(CometState::STARTING_COMET);

    // run cometbft which will connect to our ABCI server
    try {

      startCometBFT(cometPath);

    } catch (const std::exception& ex) {
      // TODO: maybe we should attempt a restart in this case, with a retry counter?
      setError("Exception caught when trying to run cometbft start: " + std::string(ex.what()));
      return;
    }

    // TODO: check if this wait is actually useful/needed
    //std::this_thread::sleep_for(std::chrono::seconds(1));

    // If any immediate error trying to start cometbft, quit here
    // TODO: must check this elsewhere as well?
    if (!this->status_) return;

    setState(CometState::STARTED_COMET);

    // --------------------------------------------------------------------------------------
    // Test cometbft: check that the node has started successfully.
    // TODO: ensure cometbft is terminated if we are killed (prctl()?)

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
          // Timeout
          // TODO: should warn and continue; instead?
          //       failing to get an info callback seems it would be some kind of unrecoverable condition.
          //       ideally, we would have a saved up data directory that we can use to replace the data
          //       directory that we are using now, so we can remove the current one and start again with the
          //       fresh one (unit tests can be run without the saved/recovery data dir, which would then
          //       be the mode where we DON'T remove the data dir and retry.)
          // TODO: also, shouldn't setError + return always ensure cometbft and the abci server are killed (not running)?
          setError("Timed out while waiting for an Info call from cometbft");
          return;
        }
    }

    // Check if quitting
    if (stop_) break;

    // Test the RPC connection
    // So, it turns out that in some kinds of failed-to-start node scenarios, the RPC port will not be available.
    // As far as we know, some of these failure modes may be recoverable with a retry, so if we fail to connect
    //   to the RPC port, then just restart the comet instance (retry the connection) instead of setting an error
    //   condition.
    LOGDEBUG("Will connect to cometbft RPC at port: " + std::to_string(rpcPort_));
    int rpcTries = 50; //5s
    bool rpcSuccess = false;
    while (rpcTries-- > 0 && !stop_) {
      // wait first, otherwise 1st connection attempt always fails
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (startRPCConnection()) {
        rpcSuccess = true;
        break;
      }
      LOGDEBUG("Retrying RPC connection: " + std::to_string(rpcTries));
    }

    // Check if quitting
    if (stop_) break;

    if (!rpcSuccess) {
      LOGERROR("Can't connect to the cometbft RPC port.");
      // REVIEW: Do we really want to continue/retry here?
      //         For one Comet::start(), we can have a retry count (relaunch and do over, e.g. once (1)) and when
      //         the retry count is exhausted, we use the data recovery dir if it exists, and if
      //         that one fails two times, then we stop Comet with setError.
      //         All this behavior should be controlled via the Comet class interface (options/properties).
      LOGERROR("Will stop cometbft, relaunch cometbft and reconnect.");
      continue;
    }

    // Make a sample RPC call using the persisted connection
    std::string result;
    bool success = makeJSONRPCCall("health", json::object(), result);
    if (success) {
      LOGDEBUG("cometbft RPC health call returned OK: "+ result);
    } else {
      LOGERROR("ERROR: cometbft RPC health call failed: " + result);
      // REVIEW: Do we really want to continue/retry here?
      LOGERROR("Will stop cometbft, relaunch cometbft and reconnect.");
      continue;
    }

    setState(CometState::TESTED_COMET);

    // --------------------------------------------------------------------------------------
    // Main loop.
    // If there are queued requests, send them to the comet process.
    // Monitor cometbft integration health until node is shutting down.
    // If something goes wrong, terminate cometbft and restart this worker
    //   (i.e. continue; this outer loop) and it will reconnect/restart comet.

    setState(CometState::RUNNING);

    bool relaunch = false;

    // NOTE: If this loop breaks for whatever reason without !stop being true, we will be
    //       in the TERMINATED state, which is there to catch bugs.
    while (!stop_) {

      // The ABCI connection can die if e.g. cometbft errors out and decides to close it or if
      //   the cometbft process dies. In any case, the ABCI connection closing means this
      //   run of the cometbft process is over and we either error out the Comet engine or
      //   we transparently attempt to restart the backing cometbft process.
      if (!abciServer_->running()) {
        LOGERROR("ABCIServer is not running (ABCI connection with cometbft has been closed.");
        // REVIEW: Do we really want to continue/retry here?
        LOGERROR("Will stop cometbft, relaunch cometbft and reconnect.");
        relaunch = true;
        break;
      }

      // FIXME/TODO: replace the cometbft rpc connection with the websocket version
      // TODO: improve the RPC transaction pump to be asynchronous, which allows us to send
      //       multiple transactions at once and then wait for the responses. this is probably
      //       using the websocket version of the transaction pump.
      //       in the websocket version, once the tx is pulled from the main queue, it is
      //       inserted into an in-flight txsend map with the RPC request ID as the index
      //       and a timeout; the timeout is the primary way the in-flight request expires
      //       in case the response for the (client-side-generated req id) never arrives for
      //       some reason (like RPC connection remade, cometbft restarted, etc)
      //       if it expires or errors out, it is just reinserted in the main queue.
      //       if it succeeds, it is just removed from the in-flight txsend map.

      // pump all pending transactions to send first
      while (!stop_) {

        // lock the txOut_ queue to check its size
        std::unique_lock<std::mutex> lock(txOutMutex_);
        if (txOut_.empty()) {
          lock.unlock();
          break;
        }
        lock.unlock();

        // fetching the element in front works outside of locking txOutMutex_
        // since only this loop can remove the front element, and elements in
        // general (other threads will only be pushing new elements to the back).
        const auto& txOutItem = txOut_.front();
        uint64_t txTicketId = std::get<0>(txOutItem);
        const Bytes* tx = &std::get<1>(txOutItem);

        std::string encodedTx = base64::encode_into<std::string>(tx->begin(), tx->end());

        LOGXTRACE("Sending tx via RPC, size: " + std::to_string(tx->size()));

        json params = { {"tx", encodedTx} };
        std::string result;
        bool success = makeJSONRPCCall("broadcast_tx_async", params, result);

        // try to extract the SHA256 txhash computed by cometbft
        std::string txHash = "";
        try {
          json response = json::parse(result);
          if (response.contains("result") && response["result"].contains("hash")) {
            txHash = response["result"]["hash"].get<std::string>();
          }
        } catch (const json::exception& e) {
        }

        // Give the transaction back to the application in any case
        listener_->sendTransactionResult(*tx, txTicketId, success, txHash, result);

        // Always remove the transaction from the send queue (if it failed, it will have to be resent)
        std::unique_lock<std::mutex> lockPop(txOutMutex_);
        txOut_.pop_front();
        lockPop.unlock();

        // If we got an RPC error, we might have bigger problems (e.g. cometbft died) so quit and re-check abciServer_ above.
        if (!success) {
          break;
        }
      }

      // TODO: We should actually have an array of persisted RPC connections, not just one,
      // so that e.g. sendTransaction can't starve checkTransaction.
      //
      // when we don't have any more transactions to send, do all transaction check requests
      while (!stop_) {

        std::unique_lock<std::mutex> lock(txCheckMutex_);
        if (txCheck_.empty()) {
          lock.unlock();
          break;
        }
        lock.unlock();

        const std::string& txCheckHash = txCheck_.front();

        LOGXTRACE("Checking txHash: " + txCheckHash);

        // We need to convert the hex string of txCheckHash to bytes first
        // Then we need to Base64-encode that because that is a requirement of the JSON-RPC POST interface of cometbft
        Bytes hx = Hex::toBytes(txCheckHash);
        std::string encodedHexBytes = base64::encode_into<std::string>(hx.begin(), hx.end());

        json params = { {"hash", encodedHexBytes} };
        std::string result;
        bool success = makeJSONRPCCall("tx", params, result);

        // Sample response: {"jsonrpc":"2.0","id":6,"result":{"hash":"2A62F69DB37417A3EB7E72219BDE4D6ADCD1A9878527DA245D4CC30FD1F899AB",
        // "height":"2","index":0,"tx_result":{"code":0,"data":null,"log":"","info":"","gas_wanted":"0","gas_used":"0","events":[],
        // "codespace":""},"tx":"rd7e3t7e3t7e3........
        //
        // Apparently cometbft responds "not found" if you ask for a transaction hash that is pending / in the mempool. It only
        // returns via /tx in a non-erroneous way if it was in fact included in a block.
        //
        // This call also returns the entire transaction body, which is inefficient if all you want is a confirmation that the
        // transaction went through and the transaction is big.

        listener_->checkTransactionResult(txCheckHash, success, result);

        std::unique_lock<std::mutex> lockPop(txCheckMutex_);
        txCheck_.pop_front();
        lockPop.unlock();

        if (!success) {
          break;
        }
      }

      // wait a little bit before we poll the transaction queue and the stop flag again
      // TODO: optimize to condition variables later (when everything else is done)
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // If the running loop exits with the relaunch flag set, then continue to relaunch cometbft.
    if (relaunch) {
      continue;
    }

    // --------------------------------------------------------------------------------------
    // If the main loop above exits and this is reached, it is because we are shutting down.
    // Will shut down cometbft, clean up and break loop.
    // stop_ *must* be set to true at this point.
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Reaching here is a bug as stop_ *must* have been set to true.
    // This is useful to detect bugs in the RUNNING state loop above.

    setState(CometState::TERMINATED);

    LOGERROR("Comet worker: exiting (loop end reached); this is an error!");
    setError("Reached TERMINATED state (should not be possible).");
    return;
  }

  setState(CometState::FINISHED);

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
std::vector<Bytes> toBytesVector(const google::protobuf::RepeatedPtrField<std::string>& repeatedField) {
  std::vector<Bytes> result;
  for (const auto& str : repeatedField) {
      result.emplace_back(str.begin(), str.end());
  }
  return result;
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

  // FIXME/TODO/REVIEW: If we enable Vote Extensions (which we probably want to enable even if we aren't using them
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
  listener_->buildBlockProposal(toBytesVector(req.txs()), delTxIds);
  for (size_t i = 0; i < req.txs().size(); ++i) {
    if (delTxIds.find(i) == delTxIds.end()) {
      res->add_txs(req.txs()[i]);
    }
  }
}

void CometImpl::process_proposal(const cometbft::abci::v1::ProcessProposalRequest& req, cometbft::abci::v1::ProcessProposalResponse* res) {
  bool accept = false;
  listener_->validateBlockProposal(req.height(), toBytesVector(req.txs()), accept);
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
  listener_->incomingBlock(
    req.height(), req.syncing_to_height(), toBytesVector(req.txs()), toBytes(req.proposer_address()), toNanosSinceEpoch(req.time()),
    hashBytes, txResults, validatorUpdates
  );

  // application must give us one txResult entry for every tx entry we have given it.
  if (txResults.size() != req.txs().size()) {
    LOGFATALP_THROW("FATAL: Comet incomingBlock got " + std::to_string(txResults.size()) + " txResults but txs size is " + std::to_string(req.txs().size()));
  }

  std::string hashString(hashBytes.begin(), hashBytes.end());
  res->set_app_hash(hashString);

  // FIXME/TODO: Check if we need to expose more ExecTxResult fields to the application.
  for (size_t i = 0; i < req.txs().size(); ++i) {
    cometbft::abci::v1::ExecTxResult* tx_result = res->add_tx_results();
    CometExecTxResult& txRes = txResults[i];
    tx_result->set_code(txRes.code);
    tx_result->set_data(toStringForProtobuf(txRes.data));
    tx_result->set_gas_wanted(txRes.gas_wanted);
    tx_result->set_gas_used(txRes.gas_used);
  }

  // Relay validator update commands to cometbft
  for (const auto& validatorUpdate : validatorUpdates) {
    auto* update = res->add_validator_updates();
    update->set_power(validatorUpdate.power);
    update->set_pub_key_type("ed25519"); //<-- string that appears by default in a cometbft init genesis file ("tendermint/PubKeyEd25519");
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

bool Comet::getStatus() {
  return impl_->getStatus();
}

const std::string& Comet::getErrorStr() {
  return impl_->getErrorStr();
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

uint64_t Comet::sendTransaction(const Bytes& tx) {
  return impl_->sendTransaction(tx);
}

void Comet::checkTransaction(const std::string& txHash) {
  impl_->checkTransaction(txHash);
}

void Comet::start() {
  impl_->start();
}

void Comet::stop() {
  impl_->stop();
}


