/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include <evmone/evmone.h>

#include "state.h"
#include "blockchain.h"

#include "../contract/contracthost.h" // contractmanager.h
#include "../contract/contractfactory.h" // ContractFactory
#include "../contract/customcontracts.h" // ContractTypes

#include "../utils/uintconv.h"
#include "bytes/random.h"
#include "libs/base64.hpp" // To decode the base64-encoded key strings

// Only need to register contract templates once
std::once_flag State::stateRegisterContractsFlag;
void State::registerContracts() {
  ContractFactory::registerContracts<ContractTypes>();
  ContractFactory::registerContracts<InterfaceTypes>();
}

State::State(Blockchain& blockchain) : blockchain_(blockchain), vm_(evmc_create_evmone()) {
  // Register all contract templates in this binary at the ContractFactory
  std::call_once(stateRegisterContractsFlag, &State::registerContracts);

  // Populate State::createContractFuncs_ with all contract templates in the binary
  ContractFactory::addAllContractFuncs<ContractTypes>(this->createContractFuncs_);

  // Set the machine state to an UNDEFINED state.
  // To set this State instance to a block height, must call either State::initChain()
  //   (genesis initialization) or State::loadSnapshot() (init from state snapshot).
  // This state is however read by the ABCI Info callback, since that is called before
  //   ABCI InitChain. Info will get height==0 from this resetState(), which seems to
  //   be correct (that will just trigger the following ABCI InitChain callback, which
  //   ultimately results in State::initChain() being called to set this State instance
  //   to the actual genesis state).
  resetState();
}

State::~State() { evmc_destroy(this->vm_); }

void State::initChain(
  uint64_t initialHeight, uint64_t initialTimeEpochSeconds,
  const std::vector<CometValidatorUpdate>& initialValidators,
  std::string genesisSnapshot
) {
  LOGTRACE("State::initChain(): Height (BDK, -1) = " + std::to_string(initialHeight));

  // Reset the State (accounts, vmstorage, contracts) and set the genesis
  // starting height and timeMicros (time-in-microseconds) for this State.
  resetState(initialHeight, initialTimeEpochSeconds * 1'000'000);

  // If the genesis snapshot file param is set, load it.
  if (genesisSnapshot != "") {
    // Genesis snapshots can be V1 snapshots without height_ and timeMicros_
    // If these metadata fields are present, they will overwrite the height/time
    //   params supplied to this call and that we set in resetState() above.
    loadSnapshot(genesisSnapshot, true);
  }

  // If set in options, give the chain owner its initial balance
  uint256_t chainOwnerBalance = this->blockchain_.opt().getChainOwnerInitialBalance();
  if (chainOwnerBalance > 0) this->setBalance(this->blockchain_.opt().getChainOwner(), chainOwnerBalance);

  // Initial system contract params (ctor params)
  std::vector<std::string> initialValidatorPubKeys;
  for (const auto& validator : initialValidators) {
    // SystemContract takes public keys as hex strings instead of PubKey, unfortunately.
    std::string validatorPubKeyStr = Hex::fromBytes(validator.publicKey).get();
    initialValidatorPubKeys.push_back(validatorPubKeyStr);
  }

  uint64_t initialNumSlots = initialValidatorPubKeys.size(); // initial numSlots will simply be the number of validators provided at genesis
  uint64_t maxSlots = 1'000; // Set this to the global maximum numslots for BDK chains

  LOGTRACE("Deploying SystemContract...");

  // Create SystemContract account
  auto& systemContractAcc = *this->accounts_[ProtocolContractAddresses.at("SystemContract")];
  systemContractAcc.nonce = 1;
  systemContractAcc.contractType = ContractType::CPP;

  // Create SystemContract contract
  // We can only construct the SystemContract on initChain() because we need to know the genesis parameters.
  this->contracts_[ProtocolContractAddresses.at("SystemContract")] =
    std::make_unique<SystemContract>(
      initialValidatorPubKeys, initialNumSlots, maxSlots,
      ProtocolContractAddresses.at("SystemContract"), blockchain_.opt().getChainOwner(), blockchain_.opt().getChainID()
    );

  // Set the initial validator set.
  // NOTE: The SystemContract has its own idea of what the validator set is (it stores it
  // in a pair of SafeVector, which is not as useful as the this->validators_ map). However,
  // both Blockchain and SystemContract should see the same validator set, as the SystemContract
  // will seed itself from Options::getCometBFT() (which must match &initialValidators).
  setValidators(initialValidators);
}

void State::setValidators(const std::vector<CometValidatorUpdate>& newValidatorSet) {
  // NOTE: stateMutex_ should already be locked by caller, if necessary

  // Normally, a validator set that is chosen in block H becomes active in H+2.
  // However, if we have no validator sets, that means we are getting the genesis validator set
  //   which is automatically active at the current State height.
  uint64_t activationHeight = height_;
  if (currentValidatorSet_ >= 0) {
    // The set we are pushing at the front will be the currentValidatorSet two blocks from now.
    activationHeight = height_ + 2;
    // Since we pushed a new element, the current set is deeper in the deque now.
    ++currentValidatorSet_;
  } else {
    // Point to the genesis set we are creating.
    currentValidatorSet_ = 0;
  }
  validatorSets_.emplace_front(
    ValidatorSet(
      activationHeight,
      newValidatorSet
    )
  );
}

Address State::validatorCometAddressToEthAddress(Address validatorCometAddress) {
  std::shared_lock<std::shared_mutex> lock(stateMutex_);
  if (currentValidatorSet_ < 0) {
    throw DynamicException("No validator set: " + std::to_string(validatorSets_.size()));
  }
  return validatorSets_[currentValidatorSet_].validatorCometAddressToEthAddress(validatorCometAddress);
}

// Returns the currently active validator set and the height in which it became active
void State::getValidatorSet(std::vector<CometValidatorUpdate>& validatorSet, uint64_t& height) {
  std::shared_lock<std::shared_mutex> lock(stateMutex_);
  if (currentValidatorSet_ < 0) {
    throw DynamicException("No validator set: " + std::to_string(validatorSets_.size()));
  }
  validatorSet = validatorSets_[currentValidatorSet_].getValidators();
  height = validatorSets_[currentValidatorSet_].getHeight();
}

std::string State::getLogicalLocation() const { return blockchain_.getLogicalLocation(); }

void State::contractSanityCheck(const Address& addr, const Account& acc) {
  switch (acc.contractType) {
    case ContractType::CPP: {
      if (this->contracts_.find(addr) == this->contracts_.end()) {
        LOGFATAL_THROW(
          "Contract " + addr.hex().get() + " is marked as C++ contract but doesn't have code"
        );
      }
      break;
    }
    case ContractType::EVM: {
      if (acc.code.empty()) {
        LOGFATAL_THROW(
          "Contract " + addr.hex().get() + " is marked as EVM contract but doesn't have code"
        );
      }
      break;
    }
    case ContractType::NOT_A_CONTRACT: {
      if (!acc.code.empty()) {
        LOGFATAL_THROW(
          "Contract " + addr.hex().get() + " is marked as not a contract but has code"
        );
      }
      break;
    }
    default:
      // TODO: this is a placeholder, contract types should be revised.
      // Also we can NOT remove NOT_A_CONTRACT for now because tests will complain about it.
      LOGFATAL_THROW("Invalid contract type");
      break;
  }
}

size_t State::getUserContractsSize() {
  std::shared_lock lock(this->stateMutex_);
  size_t count = contracts_.size();
  // Subtract each protocol contract that we find from the user-deployed contracts count.
  for (const auto& pca : ProtocolContractAddresses) {
    const Address& protocolContractAddress = pca.second;
    if (this->contracts_.find(protocolContractAddress) != this->contracts_.end()) {
      --count;
    }
  }
  return count;
}

SystemContract* State::getSystemContract() {
  std::shared_lock<std::shared_mutex> lock(stateMutex_);
  return getSystemContractInternal();
}

SystemContract* State::getSystemContractInternal() {
  if (this->contracts_.find(ProtocolContractAddresses.at("SystemContract")) == this->contracts_.end()) {
    throw DynamicException("SystemContract not found");
  }
  const auto& uniquePtr = this->contracts_[ProtocolContractAddresses.at("SystemContract")];
  if (uniquePtr.get() == nullptr) {
    throw DynamicException("SystemContract not found (null)");
  }
  // dynamic_cast is not needed here; we know this is a SystemContract
  return static_cast<SystemContract*>(uniquePtr.get());
}

void State::resetState(uint64_t height, uint64_t timeMicros) {
  std::unique_lock<std::shared_mutex> lock(stateMutex_);

  // Init state metadata
  height_ = height;
  timeMicros_ = timeMicros;

  // Ensure all machine state is wiped out
  this->contracts_.clear();
  this->vmStorage_.clear();
  this->accounts_.clear();

  // Wipe the validator set schedule
  validatorSets_.clear();
  currentValidatorSet_ = -1;

  // Create ContractManager account
  auto& contractManagerAcc = *this->accounts_[ProtocolContractAddresses.at("ContractManager")];
  contractManagerAcc.nonce = 1;
  contractManagerAcc.contractType = ContractType::CPP;

  // Create ContractManager contract
  this->contracts_[ProtocolContractAddresses.at("ContractManager")] =
    std::make_unique<ContractManager>(
      this->contracts_, this->createContractFuncs_, blockchain_.opt()
    );

  // Reset the ContractGlobals to some default values (these are set
  //  before the ContractHost runs anything, so these values are only
  //  ever accessed erroneously)
  ContractGlobals::coinbase_ = Address();
  ContractGlobals::blockHash_ = Hash();
  ContractGlobals::blockHeight_ = 0;
  ContractGlobals::blockTimestamp_ = 0;
}

void State::saveSnapshot(const std::string& where) {
  LOGINFO("Saving snapshot to: " + where);

  // Create DB directory for output
  DB out(where);

  // Lock state while writing snapshot.
  std::shared_lock<std::shared_mutex> lock(stateMutex_);

  // Write snapshot header
  std::unique_ptr<DBBatch> metaBatch = std::make_unique<DBBatch>();
  metaBatch->push_back(
    StrConv::stringToBytes("height_"),
    UintConv::uint64ToBytes(height_),
    DBPrefix::snapshotMetadata
  );
  metaBatch->push_back(
    StrConv::stringToBytes("timeMicros_"),
    UintConv::uint64ToBytes(timeMicros_),
    DBPrefix::snapshotMetadata
  );
  out.putBatch(*metaBatch);
  metaBatch.reset();

  // Write the validator set schedule (all validator sets that State is keeping track of).
  // We need this when restoring a machine state because the SystemContract only keeps
  // the latest elected validator set (in other words, State does not delegate this
  // responsibility to the governance contract; the governance contract can be changed
  // to handle chain governance differently with a reduced risk of affecting the machine
  // plus consensus engine environment in an unintended way).
  std::unique_ptr<DBBatch> validatorBatch = std::make_unique<DBBatch>();
  for (int i = 0; i < validatorSets_.size(); ++i) {
    const std::vector<CometValidatorUpdate>& validators = validatorSets_[i].getValidators();
    const uint64_t& height = validatorSets_[i].getHeight();
    Bytes serializedValidatorSet;
    for (int j = 0; j < validators.size(); ++j) {
      Utils::appendBytes(serializedValidatorSet, validators[i].publicKey);
      Utils::appendBytes(serializedValidatorSet, IntConv::int64ToBytes(validators[i].power));
    }
    validatorBatch->push_back(
      UintConv::uint64ToBytes(height),
      serializedValidatorSet,
      DBPrefix::validatorSets
    );
  }
  out.putBatch(*validatorBatch);
  validatorBatch.reset();

  size_t count;

  // Write accounts_ in batches of 1,000; uses a bit more memory but should be faster
  // than just a loop of db.put(k,v).
  count = 0;
  std::unique_ptr<DBBatch> accBatch = std::make_unique<DBBatch>();
  for (const auto& [address, account] : this->accounts_) {
    accBatch->push_back(address, account->serialize(), DBPrefix::nativeAccounts);
    if (++count >= 1'000) {
      count = 0;
      out.putBatch(*accBatch);
      accBatch = std::make_unique<DBBatch>();
    }
  }
  out.putBatch(*accBatch);
  accBatch.reset();

  // Write vmStorage_ in batches of 1,000; uses a bit more memory but should be faster
  // than just a loop of db.put(k,v).
  count = 0;
  std::unique_ptr<DBBatch> vmBatch = std::make_unique<DBBatch>();
  for (const auto& [storageKey, storageValue] : this->vmStorage_) {
    vmBatch->push_back(
      Utils::makeBytes(bytes::join(storageKey.first, storageKey.second)),
      storageValue,
      DBPrefix::vmStorage
    );
    if (++count >= 1'000) {
      count = 0;
      out.putBatch(*vmBatch);
      vmBatch = std::make_unique<DBBatch>();
    }
  }
  out.putBatch(*vmBatch);
  vmBatch.reset();

  // Write contracts_
  for (const auto& [address, baseContractPtr] : this->contracts_) {
    // REVIEW: Snapshotting should be done by a dedicated snapshotter node, and it may
    // make sense to add parallelization here (if the snapshotter will be using a dedicated
    // machine, for example). In that case, the number of snapshotter threads should be
    // a command-line, process-wide option e.g. --snapshot_threads <N>.
    out.putBatch(baseContractPtr->dump());
  }

  // Write an __END__ metadata key so that the loader will know the snapshot is not truncated
  metaBatch = std::make_unique<DBBatch>();
  metaBatch->push_back(
    StrConv::stringToBytes("__END__"),
    Bytes(),
    DBPrefix::snapshotMetadata
  );
  out.putBatch(*metaBatch);
  metaBatch.reset();
}

void State::loadSnapshot(const std::string& where, bool genesisSnapshot) {
  // NOTE: This method is called in a loop that tries the most recent snapshot first and catches exceptions,
  // then tries the second most recent one and so on, so it's fine to just throw on error.

  LOGINFO("Loading snapshot from: " + where);

  // DB directory & instance (uncompressed)
  DB in(where);

  // Wipe the state (keeps ContractManager only)
  // NOTE: already locks stateMutex_ internally
  resetState();

  std::unique_lock<std::shared_mutex> lock(stateMutex_);

  // Load snapshot metadata.
  // We tolerate snapshots in the old format which did not have snapshot metadata.
  // Missing metadata fields are simply left unchanged.
  if (!in.has(std::string("__END__"), DBPrefix::snapshotMetadata)) {
    throw DynamicException("Read corrupt snapshot; missing __END__ field (snapshot is likely truncated).");
  }
  if (in.has(std::string("height_"), DBPrefix::snapshotMetadata)) {
    Bytes heightBytes = in.get(std::string("height_"), DBPrefix::snapshotMetadata);
    height_ = UintConv::bytesToUint64(heightBytes);
    LOGXTRACE("Snapshot height: " + std::to_string(height_));
  } else {
    if (genesisSnapshot) {
      LOGWARNING("Snapshot has no height field, leaving State height at " + std::to_string(height_));
    } else {
      throw DynamicException("Read corrupt snapshot; missing height_ field.");
    }
  }
  if (in.has(std::string("timeMicros_"), DBPrefix::snapshotMetadata)) {
    Bytes timeMicrosBytes = in.get(std::string("timeMicros_"), DBPrefix::snapshotMetadata);
    timeMicros_ = UintConv::bytesToUint64(timeMicrosBytes);
    LOGXTRACE("Snapshot timeMicros: " + std::to_string(timeMicros_));
  } else {
    if (genesisSnapshot) {
      LOGWARNING("Snapshot has no timeMicros field, leaving State timeMicros at " + std::to_string(timeMicros_));
    } else {
      throw DynamicException("Read corrupt snapshot; missing timeMicros_ field.");
    }
  }

  // Load the validator set schedule
  uint64_t vsetCount = 0;
  auto vsetsFromDB = in.getBatch(DBPrefix::validatorSets);
  currentValidatorSet_ = vsetsFromDB.size();
  for (const auto& dbEntry : vsetsFromDB) {
    uint64_t height = UintConv::bytesToUint64(dbEntry.key);
    const Bytes& validatorSetBytes = dbEntry.value;
    // A serialized Secp256k1 PubKey is 33 bytes + 8 bytes int64_t voting power = 41
    if (validatorSetBytes.size() % 41 != 0) {
      throw DynamicException("Read corrupt snapshot; a serialized validator set is truncated.");
    }
    int validatorCount = validatorSetBytes.size() / 41;
    std::vector<CometValidatorUpdate> validators;
    for (int i = 0; i < validatorCount; ++i) {
      int offset = 41 * i;
      Bytes validator(validatorSetBytes.begin() + offset, validatorSetBytes.begin() + offset + 33);
      Bytes votesBytes(validatorSetBytes.begin() + offset + 33, validatorSetBytes.begin() + offset + 41);
      int64_t votes = IntConv::bytesToInt64(votesBytes);
      validators.push_back(
        CometValidatorUpdate{
          validator,
          votes
        }
      );
    }
    validatorSets_.push_front(ValidatorSet(height, validators)); // DB is a sorted container, so push oldest height first
    if (height <= height_) {
      // This validator set is not yet one that is in the future, so it is the next candidate.
      --currentValidatorSet_;
    }
    ++vsetCount;
  }
  if (currentValidatorSet_ == vsetsFromDB.size()) {
    throw DynamicException("Read corrupt snapshot: no validator set is current");
  }
  LOGXTRACE("Validator sets size: " + std::to_string(vsetCount));

  // Load accounts_
  uint64_t accCount = 0;
  auto accountsFromDB = in.getBatch(DBPrefix::nativeAccounts);
  for (const auto& dbEntry : accountsFromDB) {
     this->accounts_.emplace(Address(dbEntry.key), dbEntry.value);
     ++accCount;
  }
  LOGXTRACE("Snapshot accounts size: " + std::to_string(accCount));

  // Load vmStorage_
  uint64_t vmCount = 0;
  for (const auto& dbEntry : in.getBatch(DBPrefix::vmStorage)) {
    Address addr(dbEntry.key | std::views::take(ADDRESS_SIZE));
    Hash hash(dbEntry.key | std::views::drop(ADDRESS_SIZE));
    this->vmStorage_.emplace(StorageKeyView(addr, hash), dbEntry.value);
    ++vmCount;
  }
  LOGXTRACE("Snapshot vmStorage size: " + std::to_string(vmCount));

  // Load contracts_
  auto& baseContractPtr = this->contracts_[ProtocolContractAddresses.at("ContractManager")];
  ContractManager* cmPtr = dynamic_cast<ContractManager*>(baseContractPtr.get());
  uint64_t ctCount = 0;
  for (const DBEntry& contract : in.getBatch(DBPrefix::contractManager)) {
    Address address(contract.key);
    if (!cmPtr->loadFromDB<PersistedContractTypes>(contract, address, in)) {
      throw DynamicException("Read corrupt snapshot; unknown contract: " + StrConv::bytesToString(contract.value));
    }
    ++ctCount;
  }
  LOGXTRACE("Snapshot contracts size: " + std::to_string(ctCount));

  // State sanity check
  // Check if all found contracts in the accounts_ map really have code or are C++ contracts
  for (const auto& [addr, acc] : this->accounts_) {
    contractSanityCheck(addr, *acc);
  }

  // SystemContract must be present in non-genesis snapshots.
  // SystemContract must not be present in genesis snapshots.
  bool hasSystemContract = (
    this->contracts_.find(ProtocolContractAddresses.at("SystemContract")) != this->contracts_.end()
    && this->accounts_.find(ProtocolContractAddresses.at("SystemContract")) != this->accounts_.end()
  );
  if (genesisSnapshot && hasSystemContract) {
    // REVIEW: We could also just explicitly ignore (skip) the SystemContract when loading a genesis snapshot,
    //         instead of considering the whole snapshot invalid.
    throw DynamicException("Invalid genesis snapshot (has SystemContract).");
  }
  if (!genesisSnapshot) {
    if (!hasSystemContract) {
      throw DynamicException("Invalid snapshot (missing SystemContract).");
    }
    // Make sure it actually loaded
    if (this->contracts_[ProtocolContractAddresses.at("SystemContract")].get() == nullptr) {
      throw DynamicException("Invalid snapshot (missing SystemContract: nullptr).");
    }
  }
}

void State::setBalance(const Address& addr, const uint256_t& balance) {
  std::unique_lock lock(this->stateMutex_);
  this->accounts_[addr]->balance = balance;
}

Bytes State::ethCall(EncodedStaticCallMessage& msg) {
  // We actually need to lock uniquely here
  // As the contract host will modify (reverting in the end) the state.
  std::unique_lock lock(this->stateMutex_);
  const auto& accIt = this->accounts_.find(msg.to());
  if (accIt == this->accounts_.end()) {
    return {};
  }
  const auto& acc = accIt->second;
  try {
    if (acc->isContract()) {
      ExecutionContext context = ExecutionContext::Builder{}
      .storage(this->vmStorage_)
      .accounts(this->accounts_)
      .contracts(this->contracts_)
      .blockHash(Hash())
      .txHash(Hash())
      .txOrigin(msg.from())
      .blockCoinbase(ContractGlobals::getCoinbase())
      .txIndex(0)
      .blockNumber(ContractGlobals::getBlockHeight())
      .blockTimestamp(ContractGlobals::getBlockTimestamp())
      .blockGasLimit(10'000'000)
      .txGasPrice(0)
      .chainId(blockchain_.opt().getChainID())
      .build();
      // As we are simulating, the randomSeed can be anything
      const Hash randomSeed = bytes::random();

      return ContractHost(
        this->vm_,
        this->blockchain_.storage(),
        randomSeed,
        context
      ).execute(msg);
    } else {
      return {};
    }
  } catch (VMExecutionError& e) {
    throw;
  } catch (std::exception& e) {
    throw VMExecutionError(-32603, std::string("Internal error: ") + e.what(), Bytes());
  }
}

int64_t State::estimateGas(EncodedMessageVariant msg) {
  std::unique_lock lock(this->stateMutex_);
  auto latestBlock = this->blockchain_.latest();
  try {
    std::unique_ptr<ExecutionContext> context;
    const EncodedCallMessage* callMessage = std::get_if<EncodedCallMessage>(&msg);
    const EncodedCreateMessage* createMessage = nullptr;
    if (callMessage) {
      context = ExecutionContext::Builder{}
        .storage(this->vmStorage_)
        .accounts(this->accounts_)
        .contracts(this->contracts_)
        .blockHash(latestBlock->getHash())
        .txHash(Hash())
        .txOrigin(callMessage->from())
        .blockCoinbase(latestBlock->getProposerAddr())
        .txIndex(0)
        .blockNumber(latestBlock->getNHeight())
        .blockTimestamp(latestBlock->getTimestamp())
        .blockGasLimit(10'000'000)
        .txGasPrice(0)
        .chainId(blockchain_.opt().getChainID())
        .buildPtr();
    } else {
      createMessage = std::get_if<EncodedCreateMessage>(&msg);
      if (createMessage == nullptr) {
        throw DynamicException("Invalid message type for gas estimation");
      }
      context = ExecutionContext::Builder{}
        .storage(this->vmStorage_)
        .accounts(this->accounts_)
        .contracts(this->contracts_)
        .blockHash(latestBlock->getHash())
        .txHash(Hash())
        .txOrigin(createMessage->from())
        .blockCoinbase(latestBlock->getProposerAddr())
        .txIndex(0)
        .blockNumber(latestBlock->getNHeight())
        .blockTimestamp(latestBlock->getTimestamp())
        .blockGasLimit(10'000'000)
        .txGasPrice(0)
        .chainId(blockchain_.opt().getChainID())
        .buildPtr();
    }

    const Hash randomSeed = bytes::random();
    ContractHost host(
      this->vm_,
      this->blockchain_.storage(),
      randomSeed,
      *context
    );

    return std::visit([&host] (auto&& msg) {
      const Gas& gas = msg.gas();
      const int64_t initialGas(gas);
      host.simulate(std::forward<decltype(msg)>(msg));
      return int64_t((initialGas - int64_t(gas)) * 1.15);
    }, std::move(msg));
  } catch (VMExecutionError& e) {
    throw;
  } catch (std::exception& e) {
    throw VMExecutionError(-32603, std::string("Internal error: ") + e.what(), Bytes());
  }
}

std::vector<std::pair<std::string, Address>> State::getCppContracts() const {
  std::shared_lock lock(this->stateMutex_);
  std::vector<std::pair<std::string, Address>> contracts;
  for (const auto& [address, contract] : this->contracts_) {
    contracts.emplace_back(contract->getContractName(), address);
  }
  return contracts;
}

std::vector<Address> State::getEvmContracts() const {
  std::shared_lock lock(this->stateMutex_);
  std::vector<Address> contracts;
  for (const auto& [addr, acc] : this->accounts_) {
    if (acc->contractType == ContractType::EVM) contracts.emplace_back(addr);
  }
  return contracts;
}

Bytes State::getContractCode(const Address &addr) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->accounts_.find(addr);
  if (it == this->accounts_.end()) {
    return {};
  }
  // If its a PRECOMPILE contract, we need to return "PrecompileContract-CONTRACTNAME"
  // yes, inside a Bytes object, not a string object.
  if (it->second->contractType == ContractType::CPP) {
    auto contractIt = this->contracts_.find(addr);
    if (contractIt == this->contracts_.end()) {
      return {};
    }
    std::string precompileContract = "PrecompileContract-";
    precompileContract.append(contractIt->second->getContractName());
    return {precompileContract.begin(), precompileContract.end()};
  }
  return it->second->code;
}

bool State::validateTransaction(const TxBlock& tx, bool affectsMempool, MempoolModel *mm) {
  std::unique_lock lock(this->stateMutex_);
  return validateTransactionInternal(tx, affectsMempool, mm);
}

void State::removeTxFromMempoolModel(const TxBlock& tx) {
  // ALWAYS operates on mempoolModel_, irrespective of whether validateTransactionInternal()
  // is called with a custom/temporary mempool model.
  auto fromIt = mempoolModel_.find(tx.getFrom());
  if (fromIt == mempoolModel_.end()) {
    return;
  }
  uint64_t nonce64 = static_cast<uint64_t>(tx.getNonce());
  auto& nonceMap = fromIt->second;
  auto nonceIt = nonceMap.find(nonce64);
  if (nonceIt == nonceMap.end()) {
    return;
  }
  auto& hashMap = nonceIt->second;
  auto hashIt = hashMap.find(tx.hash());
  if (hashIt != hashMap.end()) {
    hashMap.erase(hashIt);
  }
  if (hashMap.empty()) {
    nonceMap.erase(nonceIt);
  }
  if (nonceMap.empty()) {
    mempoolModel_.erase(fromIt);
  }
}

void State::removeTxFromMempoolModel(const TxBlock& tx, MempoolModelIt& fromIt, MempoolModelNonceIt& nonceIt, MempoolModelHashIt& hashIt) {
  // ALWAYS operates on mempoolModel_, irrespective of whether validateTransactionInternal()
  // is called with a custom/temporary mempool model.
  auto& nonceMap = fromIt->second;
  auto& hashMap = nonceIt->second;
  hashMap.erase(hashIt);
  if (hashMap.empty()) {
    nonceMap.erase(nonceIt);
  }
  if (nonceMap.empty()) {
    mempoolModel_.erase(fromIt);
  }
}

bool State::validateTransactionInternal(const TxBlock& tx, bool affectsMempool, MempoolModel *mm) {
  // NOTE: Caller must have stateMutex_ locked.
  // NOTE: Signature of the originating account and other constraints are checked in TxBlock().

  // Set mm to &mempoolModel_ if mm == nullptr, so the code can use mm below.
  // All mempool model operations will be using the mm pointer, except for removeTxFromMempoolModel()
  //  and ejectTxFromMempoolModel() which *always* operate on mempoolModel_ and not the given temporary.
  bool customMM = true;
  if (mm == nullptr) {
    customMM = false;
    mm = &mempoolModel_;
  }

  // Iterators specific for looking up the tx into the state's mempoolModel_
  // (and NOT the custom MM, if any)
  MempoolModelIt stateFromIt = mempoolModel_.find(tx.getFrom());
  MempoolModelNonceIt stateNonceIt;
  MempoolModelHashIt stateHashIt;
  bool* txEjectFlagPtr = nullptr;

  // Verify if transaction already exists within the mempoolModel_.
  // If in mempoolModel_ and flagged as ejected (verified before and flagged as failed) then
  //   we have to fail it going forward. A tx flagged as "ejected" will return false on the
  //   next validateTransaction() that is called by the CheckTx ABCI callback, which will
  //   cause it to be removed from the actual mempool in the CometBFT process. That means
  //   it's dead; it will not be included in a block regardless, not until we can purge it
  //   from the CometBFT mempool, at which point we remove the tx entry (and thus the
  //   eject flag==true for the tx) from the mempoolModel_, at which point it can be resent
  //   in the future, get an actual validity check that isn't aborted due to the eject flag.
  // Tx ejection is always checked against mempoolModel_, because ejection doesn't make sense
  //   when applied to a temporary mempool. The temp mempool is for checking what happens when
  //   the State is presented transactions in a given sequence; it doesn't know which transactions
  //   have already been killed in the State's mempoolModel_ and will thus fail validation when
  //   it is time to actually process a proposed block for example.
  bool alreadyInMempool = false; // Flag to indicate if we found the tx in mempool and it's valid
  if (stateFromIt != mempoolModel_.end()) {
    uint64_t nonce64 = static_cast<uint64_t>(tx.getNonce());
    stateNonceIt = stateFromIt->second.find(nonce64);
    if (stateNonceIt != stateFromIt->second.end()) {
      auto& hashMap = stateNonceIt->second;
      stateHashIt = hashMap.find(tx.hash());
      if (stateHashIt != hashMap.end()) {
        // Found an entry for (from, nonce, txHash)
        std::pair<uint256_t, bool>& txEntry = stateHashIt->second;
        if (txEntry.second) {
          // Tx is marked as ejected
          // LOGXTRACE(
          //   "Transaction: " + tx.hash().hex().get() +
          //   " is already in mempool but marked ejected. Failing immediately."
          // );
          // If rejecting the transaction will affect the mempool, then we can
          // update our internal mempool model to exclude it.
          if (affectsMempool) {
            removeTxFromMempoolModel(tx, stateFromIt, stateNonceIt, stateHashIt);
          }
          return false;
        }
        // Tx not ejected, so just keep going to re-check tx validity
        // LOGXTRACE(
        //   "Transaction: " + tx.hash().hex().get() +
        //   " is already in mempool and not ejected."
        // );
        alreadyInMempool = true;
        txEjectFlagPtr = &(txEntry.second);
      }
    }
  }

  // NOTE that if !alreadInMempool, then we don't ever need to create an entry with
  //   the ejected flag set to true for it, because if it isn't in mempoolModel_ at
  //   all then it means that finding it invalid in a temporary mempoolModel (mm)
  //   generates no need at all to let the State's mempoolModel_ know in advance
  //   that the tx in question is invalid, since if it isn't in the mempoolModel_
  //   then it means we never got CheckTx for it from the ABCI, meaning CometBFT
  //   doesn't know about it in the first place.
  // This could only ever be a false prerequisite if CometBFT somehow persisted
  //   its mempool on disk and the BDK node process didn't. But it doesn't, so
  //   if it isn't in mempoolModel_, then CometBFT must not have it in its mempool.

  // The transaction is invalid if the originator account simply doesn't exist.
  auto accountIt = this->accounts_.find(tx.getFrom());
  if (accountIt == this->accounts_.end()) {
    // LOGXTRACE("Account doesn't exist (0 balance and 0 nonce)");
    if (affectsMempool) {
       if (alreadyInMempool) {
        removeTxFromMempoolModel(tx, stateFromIt, stateNonceIt, stateHashIt);
      }
    } else {
      if (alreadyInMempool) {
        *txEjectFlagPtr = true;
      }
    }
    return false;
  }
  const uint256_t accBalance = accountIt->second->balance;
  const uint256_t accNonce = accountIt->second->nonce;

  const uint256_t txNonce = tx.getNonce();
  const uint64_t txNonceUint64 = static_cast<uint64_t>(txNonce);

  // If a block has multiple transactions for the same account within it, then
  //   transactions for nonces [account_nonce], [account_nonce+1], [account_nonce+2], etc.
  //   should all be considered individually valid.
  //   In other words, when checking if a *block* is valid, each transaction that is validated
  //   in sequence will create a speculative nonce cache that is specific to the block validation
  //   loop, which affects the result of checking whether a transaction is valid or not.
  // And finally, when preparing a block proposal (PrepareProposal), transactions should be
  //   reordered so that (a) [account_nonce], [account_nonce+1], [account_nonce+2] is respected
  //   and (b) if a nonce is skipped in a sequence, e.g. [account_nonce], [account_nonce+2], the
  //   proposal will exclude the transactions that have no way of being valid in that block (in
  //   that case, [account_nonce+2].

  // The transaction is invalid if the tx nonce is in the past.
  if (txNonce < accNonce) {
    // LOGXTRACE(
    //   "Transaction: " + tx.hash().hex().get() + " nonce " + txNonce.str() +
    //   " is in the past; current: " + accNonce.str()
    // );
    if (affectsMempool) {
       if (alreadyInMempool) {
        removeTxFromMempoolModel(tx, stateFromIt, stateNonceIt, stateHashIt);
      }
    } else {
      if (alreadyInMempool) {
        *txEjectFlagPtr = true;
      }
    }
    return false;
  }

  // If the transaction is in the future, we need to find intermediary nonces already in
  // the RAM-based mempool model, otherwise we just reject them (client must just [re]send
  // the transactions with the missing nonces first).
  // Also, we need the sum of the costs of all these transactions that needs to be added to
  // the cost of *this* transaction to see if it can be afforded.
  uint256_t costOfAllPreviousTxs = 0;
  if (txNonce > accNonce) {
    auto fromIt = mm->find(tx.getFrom()); // Check sequencing in mm (may be a custom mm)
    if (fromIt == mm->end()) {
      // LOGXTRACE(
      //   "Transaction: " + tx.hash().hex().get() + " nonce " + txNonce.str() +
      //   " is in the future but no mempool entries exist for that account; current: " + accNonce.str()
      // );
      if (affectsMempool) {
        if (alreadyInMempool) {
          removeTxFromMempoolModel(tx, stateFromIt, stateNonceIt, stateHashIt);
        }
      } else {
        if (alreadyInMempool) {
          *txEjectFlagPtr = true;
        }
      }
      return false;
    }

    // For each intermediary nonce, collect the maximum cost among all non-ejected txs
    const uint64_t accNonceUint64 = static_cast<uint64_t>(accNonce);
    for (uint64_t inonce = accNonceUint64; inonce < txNonceUint64; ++inonce) {
      // LOGXTRACE(
      //   "Transaction: " + tx.hash().hex().get() + " nonce " + txNonce.str() +
      //  " checking inonce " + std::to_string(inonce)
      // );
      auto nonceIt = fromIt->second.find(inonce);
      // Shouldn't need to check for empty map if we are doing maintenance correctly at removeTxFromMempoolModel()
      if (nonceIt == fromIt->second.end() /*|| nonceIt->second.size() == 0*/) {
        // No transaction for this nonce => can't fill the gap
        // LOGXTRACE(
        //   "Transaction: " + tx.hash().hex().get() + " nonce " + txNonce.str() +
        //   " is missing intermediate nonce " + std::to_string(inonce)
        // );
        if (affectsMempool) {
          if (alreadyInMempool) {
            removeTxFromMempoolModel(tx, stateFromIt, stateNonceIt, stateHashIt);
          }
        } else {
          if (alreadyInMempool) {
            *txEjectFlagPtr = true;
          }
        }
        return false;
      } else {
        //LOGXTRACE("Transaction: " + tx.hash().hex().get() + " nonce " + txNonce.str() + " found entries for inonce " + std::to_string(inonce));
      }

      const auto& hashMap = nonceIt->second;
      //LOGXTRACE("inonce entries size: " + std::to_string(hashMap.size())); // should never be 0
      bool foundValidTx = false;
      uint256_t maxCostForNonce = 0;

      // Among all txs for this (from, inonce), pick the max cost of any non-ejected one
      // This is a worst-case estimation: we assume the tx that will be picked will be
      // the worst in terms of cost.
      // REVIEW: instead, we could try to guess which one has the largest fee and thus
      // would be the actually prioritized one, and then use the cost (which includes
      // transfer value) when estimating the total cost of all txs within a nonce sequence.
      for (const auto& [hash, costEjectPair] : hashMap) {
        const auto& [cost, isEjected] = costEjectPair;
        if (!isEjected) {
          foundValidTx = true;
          if (cost > maxCostForNonce) {
            maxCostForNonce = cost;
          }
        }
      }

      if (!foundValidTx) {
        // All transactions for that nonce are ejected => no valid coverage for the gap
        // LOGXTRACE(
        //   "Transaction: " + tx.hash().hex().get() + " nonce " + txNonce.str() +
        //   " cannot be validated because nonce " + std::to_string(inonce) +
        //   " is present, but all are ejected."
        // );
        if (affectsMempool) {
          if (alreadyInMempool) {
            removeTxFromMempoolModel(tx, stateFromIt, stateNonceIt, stateHashIt);
          }
        } else {
          if (alreadyInMempool) {
            *txEjectFlagPtr = true;
          }
        }
        return false;
      }

      costOfAllPreviousTxs += maxCostForNonce;
    }
  }

  // The transaction is invalid if the originator account cannot pay for the transaction
  // and all earlier transactions with earlier nonces for the same from account, if any.
  // (considering the worst case total cost at each nonce: value transferred + max fee).
  const uint256_t txWithFees = tx.getValue() + (tx.getGasLimit() * tx.getMaxFeePerGas());
  if ((txWithFees + costOfAllPreviousTxs) > accBalance) {
    // LOGXTRACE(
    //   "Transaction sender: " + tx.getFrom().hex().get() +
    //   " doesn't have enough balance to send transaction. Required: " +
    //   (txWithFees + costOfAllPreviousTxs).str() + ", has: " + accBalance.str()
    // );
    if (affectsMempool) {
       if (alreadyInMempool) {
        removeTxFromMempoolModel(tx, stateFromIt, stateNonceIt, stateHashIt);
      }
    } else {
      if (alreadyInMempool) {
        *txEjectFlagPtr = true;
      }
    }
    return false;
  }

  // We are going to accept it, so make sure it is added to the mempool model.
  // If custoMM == true, we need to ensure it is inserted in the custom mm-> since
  //   alreadyInMempool is tracking only membership in the State's mempoolModel_
  if (!alreadyInMempool || customMM) {
    auto& nonceMap = (*mm)[tx.getFrom()];
    auto& hashMap = nonceMap[txNonceUint64];
    hashMap[tx.hash()] = std::make_pair(txWithFees, false);
    // LOGXTRACE(
    //   "Transaction: " + tx.hash().hex().get() +
    //   " added to mempool model (custom = " + std::to_string(customMM) + "; from=" + tx.getFrom().hex().get() +
    //   ", nonce=" + txNonce.str() +
    //   ", cost=" + txWithFees.str() + ")."
    // );
  }
  // else {
  //  LOGXTRACE(
  //    "Transaction: " + tx.hash().hex().get() +
  //    " already in mempool is valid (custom = " + std::to_string(customMM) + "; from=" + tx.getFrom().hex().get() +
  //     ", nonce=" + txNonce.str() +
  //     ", cost=" + txWithFees.str() + ")."
  //   );
  // }

  // If the originating account can pay for it and the nonce is valid, then it is valid (that is,
  // it can be included in a block because it does not invalidate it).
  return true;
}

void State::processBlock(
  const FinalizedBlock& block, std::vector<bool>& succeeded, std::vector<uint64_t>& gasUsed,
  std::vector<CometValidatorUpdate>& validatorUpdates
) {
  // NOTE: validatorCometAddressToEthAddress() locks the stateMutex_, so call it before we lock it again below.
  //       (easier than creating a xxxInternal() version of it...)
  // The coinbase address that gets all the block fees, etc. is the block proposer.
  // Address derivation schemes (from the same Secp256k1 public key) differ between CometBFT and Eth.
  // So we need to map CometBFT Address to CometValidatorUpdate (a validator public key)
  //   and then use the validator public key to compute the correct Eth Address.
  Address proposerEthAddr = blockchain_.state().validatorCometAddressToEthAddress(block.getProposerAddr());

  std::unique_lock lock(this->stateMutex_);

  // NOTE: Block should already have been validated by the caller.

  // Although block validation should have already been done, ensure
  // that the block height is the expected one.
  if (block.getNHeight() != height_ + 1) {
    throw DynamicException(
      "State::processBlock(): current height is " +
      std::to_string(height_) + " and block height is " +
      std::to_string(block.getNHeight())
    );
  }

  const Hash randomHash = block.getRandomness();
  const Hash blockHash = block.getHash();
  const uint64_t blockHeight = block.getNHeight();

  // Update contract globals based on (now) latest block
  ContractGlobals::blockHash_ = blockHash;
  ContractGlobals::blockHeight_ = blockHeight;
  ContractGlobals::blockTimestamp_ = block.getTimestamp();
  ContractGlobals::coinbase_ = proposerEthAddr;
  // LOGTRACE("Coinbase set to: " + proposerEthAddr.hex().get() + " (CometBFT Addr: " + block.getProposerAddr().hex().get() + ")");

  // Process transactions of the block within the current state
  uint64_t txIndex = 0;
  for (auto const& txPtr : block.getTxs()) {
    const TxBlock& tx = *txPtr;
    bool txSucceeded;
    uint64_t txGasUsed;
    this->processTransaction(tx, txIndex, blockHash, randomHash, txSucceeded, txGasUsed);
    succeeded.push_back(txSucceeded);
    gasUsed.push_back(txGasUsed);
    txIndex++;
  }

  // Update the state height after processing
  height_ = block.getNHeight();

  // Check if SystemContract has validator set updates for this block
  std::vector<std::pair<PubKey, uint64_t>> validatorDeltas;
  SystemContract* systemContractPtr = getSystemContractInternal();
  systemContractPtr->finishBlock(validatorDeltas); // Collect any validator changes accumulated in the singleton system contract...
  if (!validatorDeltas.empty()) {
    // Apply validator set deltas
    Bytes validatorDbLog;
    for (const auto& validatorDelta : validatorDeltas) {
      CometValidatorUpdate validatorUpdate;
      validatorUpdate.publicKey = validatorDelta.first.asBytes();
      validatorUpdate.power = static_cast<int64_t>(validatorDelta.second);
      validatorUpdates.push_back(validatorUpdate); // ...and return them to CometBFT via the `validatorUpdates` outparam...
      Utils::appendBytes(validatorDbLog, validatorUpdate.publicKey);
      Utils::appendBytes(validatorDbLog, IntConv::int64ToBytes(validatorUpdate.power));
    }
    blockchain_.storage().putValidatorUpdates(height_, validatorDbLog); // ...store all validator set changes into the local DB...

    // Update Blockchain's validator set view to match exactly whatever the SystemContract has
    // NOTE: Blockchain::setValidators() gets the given newValidatorSet below, and it understands that it is
    //       receiving that new validator set at the current state_.height_; and it also understands that
    //       the newValidatorSet will only become active in state_.height_ + 2. So it will actually queue this
    //       newValidatorSet for future use.
    //       This contrasts with the SystemContract's tracking of validator sets: it is only concerned about which
    //       validators are currently *elected*, as the SystemContract is a governance mechanism.
    SafeVector<PubKey>& scValidators = systemContractPtr->getValidators();
    SafeVector<uint64_t>& scValidatorVotes = systemContractPtr->getValidatorVotes();
    uint64_t scNumSlots = systemContractPtr->getNumSlots();
    if (scValidators.size() != scValidatorVotes.size() || scValidators.size() < scNumSlots) {
      throw DynamicException("SystemContract has inconsistent validator set data");
    }
    std::vector<CometValidatorUpdate> newValidatorSet;
    for (int i = 0; i < scNumSlots; ++i) {
      newValidatorSet.emplace_back(
        CometValidatorUpdate{
          scValidators[i].asBytes(),
          static_cast<int64_t>(scValidatorVotes[i]) // nobody is getting > 9.2 billion native token votes
        }
      );
    }
    setValidators(newValidatorSet);
  }

  // Since we have advanced the simulation height, we may need to prune validatorSets_
  // Reverse search for the first validator set that is pending, and for each iteration,
  //   add 1 to the count of old validatorSets_ entries to prune.
  // Example: if no pending validator sets exist (all are active), will prune all but the front.
  // Example: [pending pending active active active] should prune the last 2.
  int pruneCount = -1; // Start at -1 so the last active is preserved.
  for (auto it = validatorSets_.rbegin(); it != validatorSets_.rend(); ++it) {
    if (it->getHeight() > height_) {
      break; // found first pending validator set, so stop
    }
    ++pruneCount;
  }
  while (pruneCount > 0) {
    validatorSets_.pop_back();
    --pruneCount;
  }

  // After pruning validatorSets_, the currentValidatorSet_ is simply the last one.
  currentValidatorSet_ = validatorSets_.size() - 1;
}

void State::processTransaction(
  const TxBlock& tx, const uint64_t& txIndex, const Hash& blockHash, const Hash& randomnessHash,
  bool& succeeded, uint64_t& gasUsed
) {
  Account& accountFrom = *this->accounts_[tx.getFrom()];
  auto& fromNonce = accountFrom.nonce;
  auto& fromBalance = accountFrom.balance;
  if (fromBalance < (tx.getValue() + tx.getGasLimit() * tx.getMaxFeePerGas())) {
    LOGERROR("Transaction sender: " + tx.getFrom().hex().get() + " doesn't have balance to send transaction");
    throw DynamicException("Transaction sender doesn't have balance to send transaction");
    return;
  }
  if (fromNonce != tx.getNonce()) {
    LOGERROR("Transaction: " + tx.hash().hex().get() + " nonce mismatch, expected: "
      + std::to_string(fromNonce) + " got: " + tx.getNonce().str()
    );
    throw DynamicException("Transaction nonce mismatch");
    return;
  }

  Gas gas(uint64_t(tx.getGasLimit()));
  TxAdditionalData txData{.hash = tx.hash()};

  try {
    const Hash randomSeed(UintConv::uint256ToBytes((static_cast<uint256_t>(randomnessHash) + txIndex)));

    ExecutionContext context = ExecutionContext::Builder{}
      .storage(this->vmStorage_)
      .accounts(this->accounts_)
      .contracts(this->contracts_)
      .blockHash(blockHash)
      .txHash(tx.hash())
      .txOrigin(tx.getFrom())
      .blockCoinbase(ContractGlobals::getCoinbase())
      .txIndex(txIndex)
      .blockNumber(ContractGlobals::getBlockHeight())
      .blockTimestamp(ContractGlobals::getBlockTimestamp())
      .blockGasLimit(10'000'000)
      .txGasPrice(tx.getMaxFeePerGas())
      .chainId(blockchain_.opt().getChainID())
      .build();

    ContractHost host(
      this->vm_,
      blockchain_.storage(),
      randomSeed,
      context);

    std::visit([&] (auto&& msg) {
      if constexpr (concepts::CreateMessage<decltype(msg)>) {
        txData.contractAddress = host.execute(std::forward<decltype(msg)>(msg));
      } else {
        host.execute(std::forward<decltype(msg)>(msg));
      }
    }, tx.toMessage(gas));

    txData.succeeded = true;
  } catch (const std::exception& e) {
    txData.succeeded = false;
    LOGERRORP("Transaction: " + tx.hash().hex().get() + " failed to process, reason: " + e.what());
  }

  ++fromNonce;
  txData.gasUsed = uint64_t(tx.getGasLimit() - uint256_t(gas));

  succeeded = txData.succeeded;
  gasUsed = txData.gasUsed;

  if (blockchain_.storage().getIndexingMode() != IndexingMode::DISABLED) {
    blockchain_.storage().putTxAdditionalData(txData);
  }

  fromBalance -= (txData.gasUsed * tx.getMaxFeePerGas());

  // Since we processed a transaction (that is in a finalized block, naturally), that means
  // this transaction is now gone from the mempool. Update the mempool model.
  removeTxFromMempoolModel(tx);
}

uint256_t State::getNativeBalance(const Address &addr) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->accounts_.find(addr);
  if (it == this->accounts_.end()) return 0;
  return it->second->balance;
}

uint64_t State::getNativeNonce(const Address& addr) const {
  std::shared_lock lock(this->stateMutex_);
  auto it = this->accounts_.find(addr);
  if (it == this->accounts_.end()) return 0;
  return it->second->nonce;
}

#if defined(BUILD_TESTS) && defined(BUILD_BENCHMARK_TESTS)
// Adapted from State::processTransaction() as the process is the same
void State::benchCall(const TxBlock& tx) {
  std::unique_lock lock(this->stateMutex_);

  ExecutionContext context = ExecutionContext::Builder{}
    .storage(this->vmStorage_)
    .accounts(this->accounts_)
    .contracts(this->contracts_)
    .blockHash(Hash())
    .txHash(tx.hash())
    .txOrigin(tx.getFrom())
    .blockCoinbase(ContractGlobals::getCoinbase())
    .txIndex(0)
    .blockNumber(ContractGlobals::getBlockHeight())
    .blockTimestamp(ContractGlobals::getBlockTimestamp())
    .blockGasLimit(10'000'000)
    .txGasPrice(tx.getMaxFeePerGas())
    .chainId(blockchain_.opt().getChainID())
    .build();

  ContractHost host(this->vm_, blockchain_.storage(), Hash(), context);

  Gas gas(uint64_t(tx.getGasLimit()));

  std::visit([&] (auto&& msg) {
    host.execute(std::forward<decltype(msg)>(msg));
  }, tx.toMessage(gas));
};
#endif

