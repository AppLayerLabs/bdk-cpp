/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "comet.h"
#include "../utils/logger.h"
#include "../libs/toml.hpp"

#include <grpcpp/server_builder.h>
#include <grpcpp/server.h>
#include <grpcpp/security/server_credentials.h>

// FIXME: figure out why it crashes at cometbft --> Echo --> our gRPC server here (ubuntu 22 setup)

// ---------------------------------------------------------------------------------------
// ABCIServiceImpl class
// This is an internal detail of the Comet class
// ---------------------------------------------------------------------------------------

// Generated from .proto files
#include <cometbft/abci/v1/service.grpc.pb.h>
#include <cometbft/abci/v1/types.grpc.pb.h>

using namespace cometbft::abci::v1;
using namespace cometbft::crypto::v1;
using namespace cometbft::types::v1;

// Comet implements the GRPC interface via this ABCIServiceImpl class, so that this can all be in the 
//  cpp and so when the rest of the code includes <comet.h> they won't get the cometbft:: symbols.
//
// TODO: The Comet class should expose a setListener() interface that allows it to take in client code 
//         that will actually implement the semantics of the ABCI application.
//       The listener should be a pointer to a CometListener base class with callback virtual methods
//         that are implemented by the client that is instantiating and using the Comet class (analogous
//         to the cometbft::abci::v1::ABCIService::Service virtual base class itself).
//       By implementing the ABCI callbacks, we can e.g. hide the ABCI types and expose our own types
//         to the caller/user of the Comet class if we want to, and handle stuff we want to hide and deal
//         with internally before passing it on.
//       We can expose the same ABCI types, but in that case we should probably not remove the scoping
//         with using namespace cometbft::xxx like we do above for client code to avoid collisions.
//
//       The listener is set via Comet, and then forwarded to this class eventually OR we access it using
//         our comet_ reference here indirectly (but then this has to be a friend class, and the methods
//         we call for that via comet_ are private to others, just accessible to this class)
//
//  https://github.com/cometbft/cometbft/blob/main/spec/abci/abci++_methods.md
//
class ABCIServiceImpl final : public cometbft::abci::v1::ABCIService::Service {
public:
    Comet& comet_;

    ABCIServiceImpl(Comet& comet) : comet_(comet) {
    }

    ~ABCIServiceImpl() override = default;

    grpc::Status Echo(grpc::ServerContext* context, const EchoRequest* request, EchoResponse* response) override {
      LOGXTRACE("Got Echo");
      response->set_message(request->message());
      return grpc::Status::OK;
    }

    grpc::Status Flush(grpc::ServerContext* context, const FlushRequest* request, FlushResponse* response) override {
      LOGXTRACE("Got Flush");
      return grpc::Status::OK;
    }

    grpc::Status Info(grpc::ServerContext* context, const InfoRequest* request, InfoResponse* response) override {
      LOGXTRACE("Got Info");
      response->set_version("1.0.0");  // Provide a simple version string
      response->set_last_block_height(0);  // No blocks committed in a no-op setup
      response->set_last_block_app_hash("");  // No app hash for the first block
      return grpc::Status::OK;
    }

    grpc::Status CheckTx(grpc::ServerContext* context, const CheckTxRequest* request, CheckTxResponse* response) override {
      LOGXTRACE("Got CheckTx");
      response->set_code(0);  // Return a success code (0) for all transactions
      return grpc::Status::OK;
    }

    grpc::Status Query(grpc::ServerContext* context, const QueryRequest* request, QueryResponse* response) override {
      LOGXTRACE("Got Query");
      response->set_code(0);  // Return a success code for queries
      return grpc::Status::OK;
    }

    grpc::Status Commit(grpc::ServerContext* context, const CommitRequest* request, CommitResponse* response) override {
      LOGXTRACE("Got Commit");
      response->set_retain_height(0); // Set the retain_height to zero to retain all blocks. (This is the default anyway)
      return grpc::Status::OK;
    }

    grpc::Status InitChain(grpc::ServerContext* context, const InitChainRequest* request, InitChainResponse* response) override {
      LOGXTRACE("Got InitChain");
      return grpc::Status::OK;
    }

    grpc::Status ListSnapshots(grpc::ServerContext* context, const ListSnapshotsRequest* request, ListSnapshotsResponse* response) override {
      LOGXTRACE("Got ListSnapshots");
      return grpc::Status::OK;
    }

    grpc::Status OfferSnapshot(grpc::ServerContext* context, const OfferSnapshotRequest* request, OfferSnapshotResponse* response) override {
      LOGXTRACE("Got OfferSnapshot");
      return grpc::Status::OK;
    }

    grpc::Status LoadSnapshotChunk(grpc::ServerContext* context, const LoadSnapshotChunkRequest* request, LoadSnapshotChunkResponse* response) override {
      LOGXTRACE("Got LoadSnapshotChunk");
      return grpc::Status::OK;
    }

    grpc::Status ApplySnapshotChunk(grpc::ServerContext* context, const ApplySnapshotChunkRequest* request, ApplySnapshotChunkResponse* response) override {
      LOGXTRACE("Got ApplySnapshotChunk");
      return grpc::Status::OK;
    }

    grpc::Status PrepareProposal(grpc::ServerContext* context, const PrepareProposalRequest* request, PrepareProposalResponse* response) override {
      LOGXTRACE("Got PrepareProposal");
      response->clear_txs();  // Ensure no transactions are included in the block proposal
      return grpc::Status::OK;
    }

    grpc::Status ProcessProposal(grpc::ServerContext* context, const ProcessProposalRequest* request, ProcessProposalResponse* response) override {
      LOGXTRACE("Got ProcessProposal");
      response->set_status(cometbft::abci::v1::PROCESS_PROPOSAL_STATUS_ACCEPT); // Accept all proposals
      return grpc::Status::OK;
    }

    grpc::Status ExtendVote(grpc::ServerContext* context, const ExtendVoteRequest* request, ExtendVoteResponse* response) override {
      LOGXTRACE("Got ExtendVote");
      response->set_vote_extension("");
      return grpc::Status::OK;
    }

    grpc::Status VerifyVoteExtension(grpc::ServerContext* context, const VerifyVoteExtensionRequest* request, VerifyVoteExtensionResponse* response) override {
      LOGXTRACE("Got VerifyVoteExtension");
      return grpc::Status::OK;
    }

    grpc::Status FinalizeBlock(grpc::ServerContext* context, const FinalizeBlockRequest* request, FinalizeBlockResponse* response) override {
      LOGXTRACE("Got FinalizeBlock");
      response->set_app_hash("");  // No state changes, so app hash is empty
      return grpc::Status::OK;
    }
};

// ---------------------------------------------------------------------------------------
// Comet class
// ---------------------------------------------------------------------------------------

Comet::Comet(std::string instanceIdStr, const Options& options)
  : instanceIdStr_(std::move(instanceIdStr)), 
    options_(options),
    abciService_(std::make_unique<ABCIServiceImpl>(*this))
{
}

void Comet::setState(const CometState& state) {
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

void Comet::setError(const std::string& errorStr) {
  this->errorStr_ = errorStr;
  this->status_ = false;
}

void Comet::resetError() {
  this->status_ = true;
  this->errorStr_ = "";
}

std::string Comet::waitPauseState(uint64_t timeoutMillis) {
  auto timeoutTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMillis);
  while (timeoutMillis == 0 || std::chrono::steady_clock::now() < timeoutTime) {
    if (!this->status_) {
      return this->errorStr_;
    }
    if (this->pauseState_ == this->state_) {
      return "";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return "TIMEOUT";
}

void Comet::workerLoopInner() {

  LOGDEBUG("Comet worker: started");

  // If we are stopping, then quit
  while (!stop_) {

    LOGDEBUG("Comet worker: start loop");

    // Not sure this is the best place to put these
    // Ensure these are reset
    grpcServerStarted_ = false;
    grpcServerRunning_ = false;

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

    setState(CometState::CONFIGURING);

    const std::string rootPath = options_.getRootPath();
    const std::string cometPath = rootPath + "/comet/";
    const std::string cometConfigPath = cometPath + "config/";
    const std::string cometConfigGenesisPath = cometConfigPath + "genesis.json";
    const std::string cometConfigPrivValidatorKeyPath = cometConfigPath + "priv_validator_key.json";
    const std::string cometConfigTomlPath = cometConfigPath + "config.toml";

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

    LOGDEBUG("Comet worker: past cometgenesis");

    if (!hasPrivValidatorKey) {
      // This is allowed, so just log it.
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
    configToml.insert_or_assign("abci", "grpc");
    configToml["storage"].as_table()->insert_or_assign("discard_abci_responses", toml::value(true));

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
    // TODO: check stop_ while in inner loop
    // TODO: ensure cometbft is terminated if we are killed (prctl()?)

    setState(CometState::INSPECTING_COMET);

    // If there's a need to run cometbft inspect first, do it here.
    //
    // As we map out all the usual error conditions, we'll add more code here to catch them
    //  and recover from them. By default, recovery can be done by just wiping off the comet/
    //  directory and starting the node from scratch. Eventually, this fallback case can be
    //  augmented with e.g. using a local snapshots directory to speed up syncing.

    // --------------------------------------------------------------------------------------
    // Stop cometbft inspect server.

    // (Not needed so far; see above)

    setState(CometState::INSPECTED_COMET);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Start our cometbft application gRPC server; make sure it is started.

    setState(CometState::STARTING_ABCI);

    // start the GRPC server
    // assert (!server_thread)
    grpcServerThread_.emplace(&Comet::grpcServerRun, this);

    // wait until we are past opening the grpc server
    while (!grpcServerStarted_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

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
    if (!grpcServerRunning_) {
      LOGERROR("Comet failed: gRPC server failed to start");

      // cleanup failed grpc server/thread startup
      grpcServerThread_->join();
      grpcServerThread_.reset();
      grpcServer_.reset();

      // Ensure these are reset
      grpcServerStarted_ = false;
      grpcServerRunning_ = false;

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

    // TODO: redirect output
    //       capture and log comet output (should go to bdk.log as debug, etc (?))
    boost::process::ipstream bpout;
    boost::process::ipstream bperr;

    // run cometbft which will connect to our GRPC server
    try {

      // Search for the executable in the system's PATH
      boost::filesystem::path exec_path = boost::process::search_path("cometbft");
      if (exec_path.empty()) {
        // This is a non-recoverable error
        // The gRPC server will be stopped/collected during stop(), which
        //   is also called by the destructor.
        setError("cometbft executable not found in system PATH");
        return;
      }

      // FIXME: proxy_app port has to be an Options configuration

      // CometBFT arguments
      std::vector<std::string> cometArgs = {
        "start", 
        "--abci=grpc", 
        "--proxy_app=tcp://127.0.0.1:26658", 
        "--home=" + cometPath
      };

      LOGDEBUG("Launching cometbft");// with arguments: " + cometArgs);

      // Launch the process
      process_ = boost::process::child(
        exec_path,
        //cometArgs
        boost::process::args(cometArgs)
        // TODO: redirect (currently going to terminal during basic testing)
        //boost::process::std_out > out_stream,
        //boost::process::std_err > err_stream
        );

      LOGDEBUG("cometbft start launched with PID: " + std::to_string(process_->id()));

    } catch (const std::exception& ex) {
      // TODO: maybe we should attempt a restart in this case, with a retry counter?
      setError("Exception caught when trying to run cometbft start: " + std::string(ex.what()));
      return;
    }

    // TODO: do we need to wait or otherwise sync with something so that we can be sure
    //       the grpc connection is estabished? (e.g. read logs)? or do we even need to?
    //       in any case, comet "started" state means the process is running AND it is
    //       connected successfully to our gRPC server (as reported by it)
    //       in the next state transition/phase, we will be testing a connection that
    //       should already be there, which is why we need to make sure that testing code
    //       can run without having to itself synchronize explicitly with the grpc connection
    //       being established.
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // If error (set by gRPC exception for example), quit here
    // TODO: must check this elsewhere as well?
    if (!this->status_) return;

    setState(CometState::STARTED_COMET);

    // --------------------------------------------------------------------------------------
    // Test the gRPC connection (e.g. run echo). If this does not work, then there's
    //   a fatal problem somewhere, so just halt the entire node / exit the process.
    // TODO: ensure cometbft is terminated if we are killed (prctl()?)

    setState(CometState::TESTING_COMET);

    // TODO: So, we actually do get an Echo from cometbft first thing after the connection
    //       is established, so we can loop here waiting for an echo received flag to be 
    //       set, and also loop seeing if cometbft has been killed or crashed.

    setState(CometState::TESTED_COMET);

    // --------------------------------------------------------------------------------------
    // Main loop.
    // If there are queued requests, send them to the comet process.
    // Monitor cometbft integration health until node is shutting down.
    // If something goes wrong, terminate cometbft and restart this worker
    //   (i.e. continue; this outer loop) and it will reconnect/restart comet.

    setState(CometState::RUNNING);

    // NOTE:
    // If this loop breaks for whatever reason without !stop being true, we will be
    //   in the TERMINATED state, unless we decide to continue; to retry, which is
    //   also possibly what we will end up doing here.
    //
    while (!stop_) {

        // TODO: here we are doing work such as:
        //   - polling/blocking at the outgoing transactions queue and pumping them into the running
        //     'cometbft start' process_

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // If the main loop exits and this is reached then, it is because we are shutting down.
    // Shut down cometbft, clean up and break loop
    // TODO: reaching here is probably an error, and should actually not be possible.
    //       this TERMINATED state may be removed.

    setState(CometState::TERMINATED);

    // ...

    LOGDEBUG("Comet worker: exiting (loop end reached)");
    return;
  }

  setState(CometState::FINISHED);

  LOGDEBUG("Comet worker: exiting (quit loop)");
}

void Comet::workerLoop() {
  LOGDEBUG("Comet worker thread: started");
  try {
    workerLoopInner();
  } catch (const std::exception& ex) {
    setError("Exception caught in comet worker thread: " + std::string(ex.what()));
  }
  LOGDEBUG("Comet worker thread: finished");
}

void Comet::start() {
  if (!this->loopFuture_.valid()) {
    this->stop_ = false;

    // TODO: is this a place to deal with the grpcXXX_ state? probably not

    // TODO: is this a place to deal with process_? probably not

    resetError(); // ensure error status is off
    setState(CometState::STARTED);
    this->loopFuture_ = std::async(std::launch::async, &Comet::workerLoop, this);
  }
}

void Comet::stop() {
  LOGXTRACE("1");

  if (this->loopFuture_.valid()) {
    this->stop_ = true;

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


    // -- Begin stop process_ if any
    // process shutdown -- TODO: assuming this is needed i.e. it doesn't shut down when the connected application goes away?
    if (process_.has_value()) {
      LOGDEBUG("Terminating CometBFT process");
      LOGXTRACE("Term 1");
      // terminate the process
      pid_t pid = process_->id();
      try {
        LOGXTRACE("Term 2");
        process_->terminate(); // SIGTERM (graceful termination, equivalent to terminal CTRL+C)
        LOGXTRACE("Term 3");
        LOGDEBUG("Process with PID " + std::to_string(pid) + " terminated");
        LOGXTRACE("Term 4");
        process_->wait();  // Ensure the process is fully terminated
        LOGXTRACE("Term 5");
        LOGDEBUG("Process with PID " + std::to_string(pid) + " joined");
        LOGXTRACE("Term 6");
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
    LOGXTRACE("10");
    // we need to ensure we got rid of any process_ instance in any case so we can start another
    process_.reset();
    LOGDEBUG("CometBFT process terminated");
    // -- End stop process_ if any


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
    LOGDEBUG("Waiting for grpcServer to finish");
    while (grpcServerRunning_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    LOGDEBUG("Finished grpcServer");


    // (3)
    // Now that cometbft should have disconnected from the gRPC server instance and it is
    //  thus inactive, then we should be able to shut it down without any problems.

    // -- Begin stop gRPC server if any
    if (grpcServer_) {
      LOGDEBUG("Shutting down grpcServer");
      // this makes the grpc server thread actually terminate so we can join it 
      grpcServer_->Shutdown();
      LOGDEBUG("Shutted down grpcServer");
    }
    LOGXTRACE("4");
    // Wait for the server thread to finish
    if (grpcServerThread_) {
      LOGXTRACE("5");
      grpcServerThread_->join();
      LOGXTRACE("6");
      grpcServerThread_.reset();
      LOGXTRACE("7");
    }
    LOGXTRACE("8");
    // get rid of the grpcServer since it is shut down
    grpcServer_.reset();
    LOGXTRACE("9");
    // Force-reset these for good measure
    grpcServerStarted_ = false;
    grpcServerRunning_ = false;
    // -- End stop gRPC server if any



    
    LOGXTRACE("11");
    this->setPauseState(); // must reset any pause state otherwise it won't ever finish
    this->loopFuture_.wait();
    this->loopFuture_.get();
    resetError(); // stop() clears any error status
    setState(CometState::STOPPED);
    LOGXTRACE("12");
  }
}

void Comet::grpcServerRun() {

  // Create the GRPC listen socket/endpoint that the external engine will connect to 
  // InsecureServerCredentials is probably correct, we're not adding a security bureaucracy to a local RPC. use firewalls.
  //
  // FIXME: need another node configuration parameter which is the listening port for GRPC
  //        for now, the port is hardcoded for initial testing
  //
  grpc::ServerBuilder builder;
  builder.AddListeningPort("127.0.0.1:26658", grpc::InsecureServerCredentials());
  builder.RegisterService(abciService_.get());
  grpcServer_ = builder.BuildAndStart();

  if (!grpcServer_) {
    // failed to start
    // set this to unlock the while (!grpcServerStarted_) barrier, but never set the grpcServerRunning_ flag since we 
    //   always were in a failed state if this is the case.
    grpcServerStarted_ = true;
    LOGERROR("grpcServerRun(): Failed to start the gRPC server!");
    return;
  }

  // need to set this before grpcServerStarted_ since that flag is used as the sync barrier in ExternalEngine::start() and
  //  after that point, detecting grpcServerRunning_ == false indicates that we are no longer running, i.e. it exited or failed.
  grpcServerRunning_ = true; // true when we know Wait() is going to be called

  // After this is set, other threads can wait a bit and then check grpcServerRunning_
  //   to guess whether everything is working as expected.
  grpcServerStarted_ = true;

  LOGDEBUG("grpcServerRun(): gRPC Server started");

  // This blocks until we call grpcServer_->Shutdown() from another thread
  try { 
    grpcServer_->Wait();
  } catch (std::exception ex) {
    setError("gRPC server error: " + std::string(ex.what()));
  }

  grpcServerRunning_ = false; // when past Wait(), we are no longer running

  LOGDEBUG("grpcServerRun(): gRPC Server stopped");
}
