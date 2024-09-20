/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "comet.h"
#include "../utils/logger.h"
#include "../libs/toml.hpp"

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

    // ...

    // --------------------------------------------------------------------------------------
    // Stop cometbft inspect server.

    // ...

    setState(CometState::INSPECTED_COMET);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Start our cometbft application gRPC server; make sure it is started.

    setState(CometState::STARTING_ABCI);

    // ...

    setState(CometState::STARTED_ABCI);

    // --------------------------------------------------------------------------------------
    // Check if quitting
    if (stop_) break;

    // --------------------------------------------------------------------------------------
    // Run cometbft start, passing the socket address of our gRPC server as a parameter.
    // TODO: ensure cometbft is terminated if we are killed (prctl()?)

    setState(CometState::STARTING_COMET);

    // ...

    setState(CometState::STARTED_COMET);

    // --------------------------------------------------------------------------------------
    // Test the gRPC connection (e.g. run echo). If this does not work, then there's
    //   a fatal problem somewhere, so just halt the entire node / exit the process.
    // TODO: ensure cometbft is terminated if we are killed (prctl()?)

    setState(CometState::TESTING_COMET);

    // ...

    setState(CometState::TESTED_COMET);

    // --------------------------------------------------------------------------------------
    // Main loop.
    // If there are queued requests, send them to the comet process.
    // Monitor cometbft integration health until node is shutting down.
    // If something goes wrong, terminate cometbft and restart this worker
    //   (i.e. continue; this outer loop) and it will reconnect/restart comet.

    setState(CometState::RUNNING);

    // ...

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
    resetError(); // ensure error status is off
    setState(CometState::STARTED);
    this->loopFuture_ = std::async(std::launch::async, &Comet::workerLoop, this);
  }
}

void Comet::stop() {
  if (this->loopFuture_.valid()) {
    this->stop_ = true;
    this->setPauseState(); // must reset any pause state otherwise it won't ever finish
    this->loopFuture_.wait();
    this->loopFuture_.get();
    resetError(); // stop() clears any error status
    setState(CometState::STOPPED);
  }
}

