/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "systemcontract.h"

#include "../../utils/strconv.h"

#include <set>

// REVIEW: Should we make a PubKey an actual ABI type instead? (that is, a
// type that can be used as a parameter type for registerd CPP contract methods)
PubKey SystemContract::pubKeyFromString(const std::string& pubKeyStr) {
  Bytes pubKeyBytes = Hex::toBytes(pubKeyStr);
  if (pubKeyBytes.size() != 33) {
    throw DynamicException("Invalid PubKey");
  }
  return PubKey(pubKeyBytes);
}

// Tie-breaks validators with the exact same number of votes by their public key bytes
struct ValidatorVotes {
  PubKey    validator;
  uint64_t  votes;
  bool operator<(const ValidatorVotes& other) const {
    if (votes != other.votes) {
      return votes > other.votes; // Must actually sort biggest votes first
    }
    for (size_t i = 0; i < 33; ++i) { // 33 = PubKey byte size
      if (validator[i] != other.validator[i]) {
        return validator[i] < other.validator[i];
      }
    }
    return false;
  }
};

void SystemContract::recordDelegationDelta(const PubKey& validator, const uint64_t& delta, const bool& positive) {
  // check that the current validator votes + the current delegation delta + the new delta won't
  // end outside of uint64_t range. if OK, save as intermediary int256_t at delegationDeltas_.
  int256_t checker = 0;
  for (int i = 0; i < validators_.size(); ++i) {
    // if validator not found, current delegated amount is 0, meaning no base value
    // to consider in overflow calculation (checker == 0).
    if (validators_[i] == validator) {
      checker = validatorVotes_[i];
      break;
    }
  }
  // consider the vote deltas we have already accumulated in previous delegate/undelegate
  // txs processed before in this current block
  auto dit = delegationDeltas_.find(validator);
  if (dit != delegationDeltas_.end()) {
    checker += dit->second;
  }
  // consider the present vote delta being applied by the caller
  if (positive) {
    checker += delta;
  } else {
    checker -= delta;
  }
  // the resulting new ranking value for the validator must fit in
  // **int64_t** (signed) because CometBFT uses int64_t for voting power.
  if (checker > std::numeric_limits<int64_t>::max() || checker < 0) {
    throw DynamicException("Delegation amount limit exceeded");
  }
  uint64_t targetVotes = checker.convert_to<uint64_t>();
  // All OK, so record it
  if (positive) {
    delegationDeltas_[validator] += delta;
  } else {
    delegationDeltas_[validator] -= delta;
  }
}

SystemContract::SystemContract(
  const Address& address, const DB& db
) : DynamicContract(address, db)
{
  LOGTRACE("Loading SystemContract...");

  this->numSlots_ = Utils::fromBigEndian<uint64_t>(db.get(std::string("numSlots_"), this->getDBPrefix()));
  this->numSlots_.commit();
  this->maxSlots_ = Utils::fromBigEndian<uint64_t>(db.get(std::string("maxSlots_"), this->getDBPrefix()));
  this->maxSlots_.commit();
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("targetSlots_"))) {
    this->targetSlots_[PubKey(dbEntry.key)] = Utils::fromBigEndian<uint64_t>(dbEntry.value);
  }
  this->targetSlots_.commit();
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("stakes_"))) {
    this->stakes_[Address(dbEntry.key)] = Utils::fromBigEndian<uint64_t>(dbEntry.value);
  }
  this->stakes_.commit();
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("delegations_"))) {
    View<Bytes> valueView(dbEntry.value);
    this->delegations_[Address(dbEntry.key)][PubKey(valueView.subspan(0, 33))] = Utils::fromBigEndian<uint64_t>(valueView.subspan(33));
  }
  this->delegations_.commit();
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("validators_"))) {
    this->validators_.push_back(PubKey(dbEntry.value));
  }
  this->validators_.commit();
  for (const auto& dbEntry : db.getBatch(this->getNewPrefix("validatorVotes_"))) {
    this->validatorVotes_.push_back(Utils::fromBigEndian<uint64_t>(dbEntry.value));
  }
  this->validatorVotes_.commit();
  delegationDeltas_.clear();
  delegationDeltas_.commit();
  doRegister();
}

SystemContract::SystemContract(
  const std::vector<std::string>& initialValidatorPubKeys,
  const uint64_t& initialNumSlots, const uint64_t& maxSlots,
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("SystemContract", address, creator, chainId)
{
  LOGTRACE("Creating SystemContract...");

  std::vector<PubKey> initialValidators;
  for (const auto& pubKeyStr : initialValidatorPubKeys) {
    initialValidators.push_back(pubKeyFromString(pubKeyStr));
  }
  // numSlots cannot be less than the size of the initial validator set.
  uint64_t effectiveNumSlots = std::min(initialNumSlots, initialValidators.size());
  if (effectiveNumSlots > maxSlots) {
    throw DynamicException("FATAL: effective validator numSlots exceeds provided maxSlots");
  }
  this->numSlots_ = effectiveNumSlots;
  this->numSlots_.commit();
  this->maxSlots_ = maxSlots;
  this->maxSlots_.commit();
  // The contract creator votes with 0 tokens on each initial validator key.
  // This is the only instance where a delegation of 0 is allowed; it is needed here so that
  //  when a default (genesis) validator is unvoted, that will cause the reimbursement of
  //  0 tokens to the creator.
  // The validatorVotes_ are set to 0. This can cause the validator set to choose randomly
  //  which validators from the initial set are used to fill the remaining slots every time
  //  the validator set is reevaluated. This is fine, since the initial validators are
  //  supposed to be replaced as soon as possible anyway, or at least receive actual votes.
  for (const auto& validatorPubKey : initialValidators) {
    this->delegations_[creator][validatorPubKey] = 0;
    this->validators_.push_back(validatorPubKey);
    this->validatorVotes_.push_back(0);
  }
  this->delegations_.commit();
  this->validators_.commit();
  this->validatorVotes_.commit();

  doRegister();
}

void SystemContract::registerContractFunctions() {
  SystemContract::registerContract();
  this->registerMemberFunction("stake", &SystemContract::stake, FunctionTypes::Payable, this);
  this->registerMemberFunction("unstake", &SystemContract::unstake, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("delegate", &SystemContract::delegate, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("undelegate", &SystemContract::undelegate, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("voteSlots", &SystemContract::voteSlots, FunctionTypes::NonPayable, this);
}

// TODO/REVIEW: rewrite as solidity deposit / fallback method?
void SystemContract::stake() {
  // Tx native token value transferred is the staking amount, and tx sender is the depositor.
  uint64_t amount = encodeAmount(this->getValue());
  if (amount == 0) {
    throw DynamicException("cannot deposit dust or zero");
  }
  stakes_[this->getCaller()] += amount;
}

void SystemContract::unstake(const uint256_t& amount) {
  // tx sender (caller) is withdrawing amount native token value
  const Address& caller = this->getCaller();
  if (stakes_.find(caller) == stakes_.end()) {
    throw DynamicException("No stake");
  }
  const uint64_t amount64 = encodeAmount(amount);
  if (amount64 == 0) {
    throw DynamicException("Cannot unstake zero tokens");
  }
  if (stakes_[caller] < amount64) {
    throw DynamicException("Insufficient balance");
  }
  stakes_[caller] -= amount64;
  if (stakes_[caller] == 0) {
    stakes_.erase(caller);
  }
  const uint256_t amount256 = decodeAmount(amount64);
  this->sendTokens(caller, amount256);
}

void SystemContract::delegate(const std::string& validatorPubKey, const uint256_t& amount) {
  PubKey validator = pubKeyFromString(validatorPubKey);
  const Address& caller = this->getCaller();
  if (stakes_.find(caller) == stakes_.end()) {
    throw DynamicException("No stake");
  }
  uint64_t amount64 = encodeAmount(amount);
  if (amount64 == 0) {
    throw DynamicException("Cannot delegate zero tokens");
  }
  if (stakes_[caller] < amount64) {
    throw DynamicException("Insufficient balance");
  }
  stakes_[caller] -= amount64;
  if (stakes_[caller] == 0) {
    stakes_.erase(caller);
  }
  Address vAddr = Secp256k1::toAddress(validator); // Generate validator's eth address from validator pub key
  if (delegations_.find(vAddr) == delegations_.end() || delegations_[vAddr].find(validator) == delegations_[vAddr].end()) {
    // It is only possible to delegate to a validator key if that validator key has a delegation to itself already.
    // Since the validator does not currently have a delegation to itself, this delegation must be from self, that is, caller must be vAddr.
    if (vAddr != caller) {
      throw DynamicException("Unregistered validator");
    }
  }
  delegations_[caller][validator] += amount64;
  recordDelegationDelta(validator, amount64, true);
}

void SystemContract::undelegate(const std::string& validatorPubKey, const uint256_t& amount) {
  PubKey validator = pubKeyFromString(validatorPubKey);
  const Address& caller = this->getCaller();
  if (delegations_.find(caller) == delegations_.end()) {
    throw DynamicException("No delegations");
  }
  if (delegations_[caller].find(validator) == delegations_[caller].end()) {
    throw DynamicException("No delegation to validator");
  }
  uint64_t amount64 = encodeAmount(amount);
  // To undelegate initial validators (with total delegation amount == 0), the contract
  // creator can undelegate any positive amount so this check will pass.
  // (It is also possible to remove the initial validators if any voter account simply
  // delegates and undelegates any positive amount).
  if (amount64 == 0) {
    throw DynamicException("Cannot undelegate zero tokens");
  }
  // If the undelegate amount is too large, then just undelegate everything
  uint64_t effectiveAmount = std::min(delegations_[caller][validator], amount64);
  delegations_[caller][validator] -= amount64;
  if (delegations_[caller][validator] == 0) {
    delegations_[caller].erase(validator);
    if (delegations_[caller].empty()) {
      delegations_.erase(caller);
    }
  }
  if (effectiveAmount > 0) {
    stakes_[caller] += effectiveAmount;
  }
  recordDelegationDelta(validator, amount64, false);
}

void SystemContract::voteSlots(const std::string& validatorPubKey, const uint64_t& slots) {
  // Validate numslots choice
  if (slots < 1 || slots > maxSlots_.get()) {
    throw DynamicException("Proposed slot count is invalid");
  }
  // Authorize caller
  PubKey validator = pubKeyFromString(validatorPubKey);
  Address vAddr = Secp256k1::toAddress(validator); // Generate validator's eth address from validator pub key
  if (vAddr != this->getCaller()) {
    throw DynamicException("Caller is not the validator");
  }
  // Find elected validator
  bool found = false;
  for (int i = 0; i < numSlots_.get(); ++i) {
    if (validators_[i] == validator) {
      found = true;
      break;
    }
  }
  if (!found) {
    throw DynamicException("Validator not elected");
  }
  // Save numslots choice
  targetSlots_[validator] = slots;
  targetSlotsModified_ = true;
}

void SystemContract::finishBlock(std::vector<std::pair<PubKey, uint64_t>>& validatorDeltas) {
  // if no delegation deltas and no numslots-change votes,
  // then it is guraanteed that there's nothing to do
  bool changedDelegations = !delegationDeltas_.empty();
  if (!changedDelegations && !targetSlotsModified_) {
    return;
  }

  // stores a copy of the validators_ & validatorVotes_ vector
  std::vector<ValidatorVotes> old;

  // complete working map of all voted validators and their vote totals
  std::map<PubKey, int256_t> votedValidators;

  // This iterates over ALL validators that have votes (delegations).
  // (including 0 votes, which is the special case for genesis validators).
  for (int i = 0; i < validators_.size(); ++i) {

    // Working backup of validators_ and validatorVotes_
    // `old` is used as basis to compute the validator update list that is sent to cometbft
    const auto& validator = validators_[i];
    ValidatorVotes vv;
    vv.validator = validator;
    vv.votes = validatorVotes_[i];
    old.push_back(vv);

    // intialize the global vote map with all validators that already had votes
    votedValidators[validator] = validatorVotes_[i];
  }

  // Apply all the vote deltas for this block on top of the voting totals that were already in effect.
  // If a validator did not have any votes, a new key will be inserted in votedValidators.
  auto dit = delegationDeltas_.cbegin();
  while (dit != delegationDeltas_.cend()) {
    votedValidators[dit->first] += dit->second;
    ++dit;
  }

  // Use votedValidators to build the `sorted` set, which will sort the
  // validators based on voting power with a total order, breaking ties
  // on the validator public key content.
  std::set<ValidatorVotes> sorted;
  for (const auto& [validator, votes] : votedValidators) {
    sorted.insert(
      ValidatorVotes{
        validator,
        static_cast<uint64_t>(votes)
      }
    );
  }

  // If delegations are unchanged, no need to fix validators_ & validatorVotes_
  if (changedDelegations) {

    // done using delegation deltas, so clear them for the next block
    delegationDeltas_.clear();

    // rebuild validators_ & validatorVotes_
    // computes an ancillary `elected` set for targetSlots_ recalculation
    validators_.clear();
    validatorVotes_.clear();
    std::set<PubKey> elected;
    int i = 0;
    for (const auto& vv : sorted) {
      validators_.push_back(vv.validator);
      validatorVotes_.push_back(vv.votes);
      if (i < numSlots_.get()) {
        elected.insert(vv.validator);
      }
      ++i;
    }

    // clear irrelevant targetSlots_ entries created by unelected validators.
    // NOTE: it = erase(it) is not compiling, so we do a deferred erase-by-key loop.
    // NOTE: deleting targetSlots_ entries does not need to set targetSlotsModified_ = true, since you can't cause
    //       a numSlots_ change by removing targetSlots_ entries.
    // FIXME: targetSlots_.begin() (vs. cbegin()) causes a crash here, but it should work the same; must investigate.
    //        NOTE: SafeUnorderedMap::iterator should be custom; should not expose the internal impl iterator.
    std::vector<PubKey> keysToErase;
    auto it = targetSlots_.cbegin();
    while (it != targetSlots_.cend()) {
      if (!elected.contains(it->first)) {
        keysToErase.push_back(it->first);
      }
      ++it;
    }
    for (const auto& key : keysToErase) {
      targetSlots_.erase(key);
    }
  }

  bool changedSlots = false;
  uint64_t oldNumSlots = numSlots_.get(); // Save oldNumSlots since numSlots_ may change
  if (targetSlotsModified_) {
    targetSlotsModified_ = false;
    targetSlotsModified_.commit(); // This is not a contract/machine call (not in a transaction context) so we need to force the change.

    // Reevaluate targetSlots_ to see if numSlots_ changes.
    std::multiset<int64_t> incVoteSet;
    std::multiset<int64_t> decVoteSet;
    int vIdx = 0;
    auto sit = sorted.begin();
    uint64_t electedValidatorCount = 0;
    while (sit != sorted.end() && vIdx < numSlots_.get()) {
      ++electedValidatorCount;
      auto tit = targetSlots_.find(sit->validator);
      if (tit != targetSlots_.end()) {
        int64_t slotsVote = static_cast<int64_t>(tit->second); // cast is OK since slotsVote < maxSlots_
        if (slotsVote > numSlots_.get()) {
          slotsVote = std::min(slotsVote, static_cast<int64_t>(maxSlots_.get())); // ensure whatever maxSlots_ is cannot be exceeded
          incVoteSet.insert(slotsVote);
        } else if (slotsVote < numSlots_.get()) {
          decVoteSet.insert(slotsVote);
        }
      }
      ++sit;
      ++vIdx;
    }
    uint64_t quorum = ((electedValidatorCount * 2) / 3) + 1;
    if (incVoteSet.size() >= quorum) {
      auto qit = incVoteSet.rbegin(); //smallest number to increase to is prioritized
      for (int i = 1; i < quorum; ++i) {
        ++qit;
        if (qit == incVoteSet.rend()) {
          throw DynamicException("SystemContract fatal error while calculating new validator slot count");
        }
      }
      numSlots_ = static_cast<uint64_t>(*qit);
      changedSlots = true;
    } else if (decVoteSet.size() >= quorum) {
      auto qit = decVoteSet.begin(); //largest number to decrease to is prioritized
      for (int i = 1; i < quorum; ++i) {
        ++qit;
        if (qit == decVoteSet.end()) {
          throw DynamicException("SystemContract fatal error while calculating new validator slot count");
        }
      }
      numSlots_ = static_cast<uint64_t>(*qit);
      changedSlots = true;
    }

    if (changedSlots) {
      // Remove all slots votes that were precisely satisfied
      std::set<PubKey> keysToRemove;
      auto tit = targetSlots_.cbegin();
      while (tit != targetSlots_.cend()) {
        if (tit->second == numSlots_.get()) {
          keysToRemove.insert(tit->first);
        }
        ++tit;
      }
      for (const auto& key : keysToRemove) {
        targetSlots_.erase(key);
      }
    }
  }

  // Finally, compute the CometBFT validator updates if any.
  // If neither delegations nor slots changed, then no validator set changes are possible.
  if (changedSlots || changedDelegations) {

    // For each validator that was elected previously, we may generate an update
    for (int i = 0; i < old.size() && i < oldNumSlots; ++i) {
      const auto& oldvv = old[i];

      // For each old elected validator, figure out its *effective* new voting power
      uint64_t newVote = 0; // (the effective new voting power is zero if the validator is sorted past the numSlots_ limit).
      int j = 0;
      auto it = sorted.begin();
      while (it != sorted.end() && j < numSlots_.get()) {
        // also note that if a validator candidate is fully undelegated (receives 0 votes)
        // it will simply disappear from validators_ / validatorVotes_ / sorted / etc.
        // and so this will also guaranteed not match, resulting in newVote == 0 and
        // if that's different than the voting power it had before, it will generate a
        // validatorDeltas update below with a voting power of 0 (power 0 update is
        // interpreted by CometBFT as "remove this validator from consensus").
        if (it->validator == oldvv.validator) {
          newVote = it->votes;
          break;
        }
        ++it;
        ++j;
      }

      // If the new effective voting power for the validator changes, generate an update
      if (newVote != oldvv.votes) {
        validatorDeltas.push_back({oldvv.validator, newVote});
      }
    }

    // Generate updates for fresh elected validators (validators that were not elected
    //  before this block was processed, but are now elected for whatever reason).
    int k = 0;
    for (const auto& vv : sorted) {
      // ensure vv is an elected validator
      if (k >= numSlots_.get()) {
        // exceeded numSlots_, reached the voted but unelected validator range
        break;
      }
      ++k;

      // ensure vv, which at this point we know is elected presently, was not elected before in old
      bool wasAlreadyElected = false;
      int l = 0;
      for (const auto& oldvv : old) {
        if (l >= oldNumSlots) {
          // exceeded oldNumSlots, reached the voted but unelected validator range of old
          break;
        }
        ++l;
        if (oldvv.validator == vv.validator) {
          wasAlreadyElected = true;
          break;
        }
      }
      if (!wasAlreadyElected) {
        // vv.validator is elected now, but was not elected before
        // remember that CometBFT validator updates with positive power mean
        // that we do include the validator in the active validator set.
        validatorDeltas.push_back({vv.validator, vv.votes});
      }
    }
  }
}

DBBatch SystemContract::dump() const {
  LOGTRACE("Saving SystemContract...");

  DBBatch batch = BaseContract::dump();
  batch.push_back(StrConv::stringToBytes("numSlots_"), UintConv::uint64ToBytes(this->numSlots_.get()), this->getDBPrefix());
  batch.push_back(StrConv::stringToBytes("maxSlots_"), UintConv::uint64ToBytes(this->maxSlots_.get()), this->getDBPrefix());
  for (auto it = this->targetSlots_.cbegin(); it != this->targetSlots_.cend(); ++it) {
    batch.push_back(it->first.asBytes(), UintConv::uint64ToBytes(it->second), this->getNewPrefix("targetSlots_"));
  }
  for (auto it = this->stakes_.cbegin(); it != this->stakes_.cend(); ++it) {
    batch.push_back(it->first.asBytes(), UintConv::uint64ToBytes(it->second), this->getNewPrefix("stakes_"));
  }
  for (auto it = this->delegations_.cbegin(); it != this->delegations_.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      const auto& key = it->first; // Address (delegator)
      Bytes value = it2->first.asBytes(); // PubKey (validator) = 33 bytes
      Utils::appendBytes(value, UintConv::uint64ToBytes(it2->second)); // uint64_t (votes) = 8 bytes
      batch.push_back(key, value, this->getNewPrefix("delegations_"));
    }
  }
  for (uint32_t i = 0; i < this->validators_.size(); ++i) {
    batch.push_back(UintConv::uint32ToBytes(i), this->validators_[i].asBytes(), this->getNewPrefix("validators_"));
  }
  for (uint32_t i = 0; i < this->validatorVotes_.size(); ++i) {
    batch.push_back(UintConv::uint32ToBytes(i), UintConv::uint64ToBytes(this->validatorVotes_[i]), this->getNewPrefix("validatorVotes_"));
  }
  if (!delegationDeltas_.empty()) {
    // delegationDeltas_ *must* be empty both at the start and at the end of block processing.
    // There is *never* a valid reason to save an inconsistent state snapshot in the middle of block processing.
    throw DynamicException("System contract is in an inconsistent state during snapshotting (delegationDeltas_ is not empty).");
  }
  return batch;
}
