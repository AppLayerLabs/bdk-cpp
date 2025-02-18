/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "systemcontract.h"

#include "../../utils/strconv.h"

#include <set>

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
      return votes < other.votes;
    }
    for (size_t i = 0; i < 33; ++i) { // 33 = PubKey byte size
      if (validator[i] != other.validator[i]) {
        return validator[i] < other.validator[i];
      }
    }
    return false;
  }
};

bool SystemContract::recordDelegationDelta(const PubKey& validator, const uint64_t& delta, const bool& positive) {
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
  checker += delegationDeltas_[validator];
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
  // No validator can have exactly the same voting power as any other
  // If we are violating this rule with the new voting power, return false so the
  //   caller can retry with another amount
  for (int i = 0; i < validatorVotes_.size(); ++i) {
    if (validatorVotes_[i] == targetVotes) {
      return false;
    }
  }
  // All OK, so record it
  if (positive) {
    delegationDeltas_[validator] += delta;
  } else {
    delegationDeltas_[validator] -= delta;
  }
  return true;
}

SystemContract::SystemContract(
  const Address& address, const DB& db
) : DynamicContract(address, db)
{
  LOGDEBUG("Loading SystemContract...");

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

  LOGDEBUG("Loaded SystemContract.");
}

SystemContract::SystemContract(
  const std::vector<std::string>& initialValidatorPubKeys,
  const uint64_t& initialNumSlots, const uint64_t& maxSlots,
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("SystemContract", address, creator, chainId)
{
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
  stakes_[this->getCaller()] += encodeAmount(this->getValue());
}

void SystemContract::unstake(const uint256_t& amount) {
  // tx sender (caller) is withdrawing amount native token value
  const auto& caller = this->getCaller();
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
  const auto& caller = this->getCaller();
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
  if (delegations_.find(caller) == delegations_.end()) {
    // It is only possible to delegate to a key if that key has a delegation to itself already.
    // FIXME/TODO: here, deriving the address of validator must yield the caller address
    // otherwise throw DynamicException("Unregistered validator")
  }
  delegations_[caller][validator] += amount64;
  // Loop avoiding collision in delegation amount (which we do not support)
  while (!recordDelegationDelta(validator, amount64, true)) {
    if (amount64 == 1) {
      throw DynamicException("Cannot delegate this amount (try a larger amount)");
    }
    --amount64;
    --delegations_[caller][validator];
  }
}

void SystemContract::undelegate(const std::string& validatorPubKey, const uint256_t& amount) {
  PubKey validator = pubKeyFromString(validatorPubKey);
  const auto& caller = this->getCaller();
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
  // Loop avoiding collision in delegation amount (which we do not support)
  while (!recordDelegationDelta(validator, amount64, false)) {
    if (amount64 == 1) {
      throw DynamicException("Cannot undelegate this amount (try a larger amount)");
    }
    --amount64;
    ++delegations_[caller][validator];
  }
}

void SystemContract::voteSlots(const std::string& validatorPubKey, const uint64_t& slots) {
  PubKey validator = pubKeyFromString(validatorPubKey);
  // FIXME/TODO: this->getCaller() address must match validator
  // i.e. deriving the address from validator (pubkey) must == this->getCaller();
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
  if (slots < 1 || slots > maxSlots_.get()) {
    throw DynamicException("Proposed slot count is invalid");
  }
  targetSlots_[validator] = slots;
}

// TODO: call this from incomingBlock()
void SystemContract::finishBlock(std::vector<std::pair<PubKey, uint64_t>>& validatorDeltas) {
  validatorDeltas.clear();

  // feed updated validators_ & validatorVotes_ into a sorted validator,votes set
  // save a copy of the validators_ & validatorVotes_ vector in old
  std::set<ValidatorVotes> sorted;
  std::vector<ValidatorVotes> old;
  for (int i = 0; i < validators_.size(); ++i) {
    const auto& validator = validators_[i];
    ValidatorVotes vv;
    vv.validator = validator;
    vv.votes = validatorVotes_[i];
    old.push_back(vv);
    if (validatorVotes_[i] > 0) {
      auto it = delegationDeltas_.find(validator);
      if (it != delegationDeltas_.end()) {
        int256_t delta = it->second;
        if (delta >= 0) {
          vv.votes += delta.convert_to<uint64_t>();
        } else {
          delta = -delta;
          vv.votes -= delta.convert_to<uint64_t>();
        }
      }
      sorted.insert(vv);
    }
  }

  bool changedDelegations = !delegationDeltas_.empty();

  // If delegations are unchanged, no need to fix validators_ & validatorVotes_
  if (changedDelegations) {

    // rebuild validators_ & validatorVotes_
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

    // clear delegation deltas
    delegationDeltas_.clear();

    // clear irrelevant targetSlots_ entries created by unelected validators.
    // NOTE: it = erase(it) is not compiling, so we do a deferred erase-by-key loop.
    std::vector<PubKey> keysToErase;
    auto it = targetSlots_.begin();
    while (it != targetSlots_.end()) {
      if (!elected.contains(it->first)) {
        keysToErase.push_back(it->first);
      }
      ++it;
    }
    for (const auto& key : keysToErase) {
      targetSlots_.erase(key);
    }
  }

  // Fully reevaluate targetSlots_ to see if numSlots_ changes.
  uint64_t oldNumSlots = numSlots_.get(); // Save oldNumSlots since numSlots_ may change
  int64_t incVote = std::numeric_limits<int64_t>::max();
  int64_t decVote = std::numeric_limits<int64_t>::min();
  uint64_t incVoteCount = 0;
  uint64_t decVoteCount = 0;
  int vIdx = 0;
  auto sit = sorted.begin();
  uint64_t electedValidatorCount = 0;
  bool changedSlots = false;
  while (sit != sorted.end() && vIdx < numSlots_.get()) {
    ++electedValidatorCount;
    int64_t slotsVote = static_cast<int64_t>(targetSlots_[sit->validator]); // cast is OK since slotsVote < maxSlots_
    if (slotsVote > numSlots_.get()) {
      incVote = std::min(incVote, slotsVote);
      incVote = std::min(incVote, static_cast<int64_t>(maxSlots_.get())); // ensure whatever maxSlots_ is cannot be exceeded
      ++incVoteCount;
    } else if (slotsVote < numSlots_.get()) {
      decVote = std::max(decVote, slotsVote);
      // already protected against slotsVote == 0 on vote submission
      ++decVoteCount;
    }
    ++sit;
    ++vIdx;
  }
  uint64_t quorum = ((electedValidatorCount * 2) / 3) + 1;
  if (incVoteCount >= quorum && incVote != numSlots_.get()) {
    numSlots_ = static_cast<uint64_t>(incVote);
    changedSlots = true;
    LOGXTRACE("Increased numSlots_: " + std::to_string(numSlots_.get()));
  } else if (decVoteCount >= quorum && decVote != numSlots_.get()) {
    numSlots_ = static_cast<uint64_t>(decVote);
    changedSlots = true;
    LOGXTRACE("Decreased numSlots_: " + std::to_string(numSlots_.get()));
  }

  // Finally, compute the CometBFT validator updates if any.
  // If neither delegations nor slots changed, then no validator set changes are possible.
  if (changedSlots || changedDelegations) {

    // For each validator that was elected previously, we may generate an update
    for (int i = 0; i < old.size() && i < oldNumSlots; ++i) {
      const auto& oldvv = old[i];

      // For each old elected validator, figure out its new voting power (which is zero if its rank/index
      //  is higher than the potentially new numSlots_ limit).
      uint64_t newVote = 0;
      int j = 0;
      auto it = sorted.begin();
      while (it != sorted.end() && j < numSlots_.get()) {
        if (it->validator == oldvv.validator) {
          newVote = it->votes;
          break;
        }
        ++it;
        ++j;
      }

      // If the new effective voting power for the validator change, generate an update
      if (newVote != oldvv.votes) {
        LOGXTRACE("Validator update (existing): " + Hash(oldvv.validator.asBytes()).hex().get());
        validatorDeltas.push_back({oldvv.validator, newVote});
      }
    }

    // Generate updates for fresh elected validators (validators that had
    // zero votes before this block and are now elected).
    int k = 0;
    for (const auto& vv : sorted) {
      if (k >= numSlots_.get()) {
        // exceeded numSlots_, reached the voted but unelected validator range
        break;
      }
      ++k;
      bool found = false;
      int l = 0;
      for (const auto& oldvv : old) {
        if (l >= oldNumSlots) {
          // exceeded oldNumSlots, reached the voted but unelected validator range
          break;
        }
        ++l;
        if (oldvv.validator == vv.validator) {
          found = true;
          break;
        }
      }
      if (!found) {
        // vv.validator has >0 power now, but had ==0 power before (not elected)
        LOGXTRACE("Validator update (new): " + Hash(vv.validator.asBytes()).hex().get());
        validatorDeltas.push_back({vv.validator, vv.votes});
      }
    }
  }
}

DBBatch SystemContract::dump() const {
  LOGDEBUG("Saving SystemContract...");

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

  LOGDEBUG("Saved SystemContract.");
  return batch;
}
