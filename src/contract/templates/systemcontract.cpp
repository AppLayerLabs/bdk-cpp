/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "systemcontract.h"

#include "../../utils/strconv.h"

#include <set>

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

void SystemContract::recordDelegationDelta(const PubKey& validator, const uint64_t& delta, const bool& positive) {
  // check that the current validator votes + the current delegation delta + the new delta won't
  // end outside of uint64_t range. if it is OK, record it in the intermediary int256_t.
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
  // the resulting new ranking value for the validator must fit in uint64_t
  if (checker > std::numeric_limits<uint64_t>::max() || checker < 0) {
    throw DynamicException("Delegation amount limit exceeded");
  }
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
  // TODO read state from DB
  //this->owner_ = Address(db.get(std::string("owner_"), this->getDBPrefix()));
  //this->owner_.commit();

  doRegister();
}

SystemContract::SystemContract(
  const std::vector<PubKey>& initialValidators,
  const uint64_t& initialNumSlots, const uint64_t& maxSlots,
  const Address& address, const Address& creator, const uint64_t& chainId
) : DynamicContract("SystemContract", address, creator, chainId)
{
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
  /*
  this->registerMemberFunction("onlyOwner", &Ownable::onlyOwner, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("owner", &Ownable::owner, FunctionTypes::View, this);
  this->registerMemberFunction("renounceOwnership", &Ownable::renounceOwnership, FunctionTypes::NonPayable, this);
  this->registerMemberFunction("transferOwnership", &Ownable::transferOwnership, FunctionTypes::NonPayable, this);
  */
}

// TODO/REVIEW: rewrite as solidity deposit / fallback method?
void SystemContract::stake() {
  // Tx native token value transferred is the staking amount, and tx sender is the depositor.
  stakes_[this->getCaller()] += encodeAmount(this->getValue());
}

void SystemContract::unstake(const uint256_t& amount) {
  // Tx native token value transferred is the staking amount, and tx sender is the depositor.
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

void SystemContract::delegate(const PubKey& validator, const uint256_t& amount) {
  const auto& caller = this->getCaller();
  if (stakes_.find(caller) == stakes_.end()) {
    throw DynamicException("No stake");
  }
  const uint64_t amount64 = encodeAmount(amount);
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
  recordDelegationDelta(validator, amount64, true);
}

void SystemContract::undelegate(const PubKey& validator, const uint256_t& amount) {
  const auto& caller = this->getCaller();
  if (delegations_.find(caller) == delegations_.end()) {
    throw DynamicException("No delegations");
  }
  if (delegations_[caller].find(validator) == delegations_[caller].end()) {
    throw DynamicException("No delegation to validator");
  }
  const uint64_t amount64 = encodeAmount(amount);
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
  stakes_[caller] += effectiveAmount;
  recordDelegationDelta(validator, amount64, false);
}

void SystemContract::voteslots(const PubKey& validator, const uint64_t& slots) {
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
void SystemContract::processBlock() {
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
      int256_t delta = delegationDeltas_[validator];
      if (delta >= 0) {
        vv.votes += delta.convert_to<uint64_t>();
      } else {
        delta = -delta;
        vv.votes -= delta.convert_to<uint64_t>();
      }
      sorted.insert(vv);
    }
  }

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
  
  // Save oldNumSlots since numSlots_ may change
  uint64_t oldNumSlots = numSlots_.get();

  // Evaluate targetSlots_ to see if numSlots_ changes.
  //2/3+1 of numSlots have a lower or greater number.
  //foreach elected
  // signed int incvote = int max
  // signed int decvote = int min
  // if vote > numslots   incvote = min(vote, incvote)   incvotecount++
  // if vote < numslots   decvote = max(vote, decvote)   decvotecount++
  // if (incvotecount > 2/3+1 of slots && incvote != numslots && incvote <= maxslots) numslots = incvote
  // if (decvotecount > 2/3+1 of slots && decvote != numslots && decvote > 0) decslots = incvote

  int64_t incVote = std::numeric_limits<int64_t>::max();
  int64_t decVote = std::numeric_limits<int64_t>::min();
  uint64_t incVoteCount = 0;
  uint64_t decVoteCount = 0;
  int vIdx = 0;
  auto sit = sorted.begin();
  uint64_t electedValidatorCount = 0;
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
  if (incVoteCount >= quorum) {
    numSlots_ = static_cast<uint64_t>(incVote);
    LOGXTRACE("Increased numSlots_: " + std::to_string(numSlots_.get()));
  } else if (decVoteCount >= quorum) {
    numSlots_ = static_cast<uint64_t>(decVote);
    LOGXTRACE("Decreased numSlots_: " + std::to_string(numSlots_.get()));
  }

  // Finally, compute the CometBFT validator updates

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
      // TODO: return another validator update:
      //   validator: oldvv.validator
      //   voting power: newVote
      LOGXTRACE("Validator update (existing): " + Hash(oldvv.validator.asBytes()).hex().get());
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
      // TODO: return another validator update
      //   validator: validator
      //   voting power: votes
    }
  }
}

DBBatch SystemContract::dump() const {
  DBBatch batch = BaseContract::dump();

  // TODO: write state to db
  //batch.push_back(StrConv::stringToBytes("owner_"), this->owner_.get().asBytes(), this->getDBPrefix());

  return batch;
}
