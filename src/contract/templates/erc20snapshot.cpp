#include "erc20snapshot.h"

ERC20Snapshot::ERC20Snapshot(
  ContractManagerInterface& interface,
  const Address& address, const std::unique_ptr<DB>& db
) : DynamicContract(interface, address, db), ERC20(interface, address, db), _accountBalanceSnapshots(this),
    _totalSupplySnapshots(this), _currentSnapshotId(this) {

  /// Load from DB.
  auto accountBalanceSnapshots_ids = this->db->getBatch(this->getNewPrefix("_accountBalanceSnapshots_ids"));
  for (auto item : accountBalanceSnapshots_ids) {
    BytesArrView viewKey(item.key);
    Address address(viewKey.subspan(0, 20));
    this->_accountBalanceSnapshots[address].ids.push_back(Utils::fromBigEndian<uint256_t>(viewKey.subspan(20)));
  }

  auto accountBalanceSnapshots_values = this->db->getBatch(this->getNewPrefix("_accountBalanceSnapshots_values"));
  for (auto item : accountBalanceSnapshots_values) {
    BytesArrView viewKey(item.key);
    Address address(viewKey.subspan(0, 20));
    this->_accountBalanceSnapshots[address].values.push_back(Utils::fromBigEndian<uint256_t>(viewKey.subspan(20)));
  }

  auto totalSupplySnapshots_ids = this->db->getBatch(this->getNewPrefix("_totalSupplySnapshots_ids"));
  for (auto item : totalSupplySnapshots_ids) {
    this->_totalSupplySnapshots.ids.push_back(Utils::fromBigEndian<uint256_t>(item.key));
  }

  auto totalSupplySnapshots_values = this->db->getBatch(this->getNewPrefix("_totalSupplySnapshots_values"));
  for (auto item : totalSupplySnapshots_values) {
    this->_totalSupplySnapshots.values.push_back(Utils::fromBigEndian<uint256_t>(item.key));
  }

  auto currentSnapshotId = this->db->get(Utils::stringToBytes("_currentSnapshotId"), this->getDBPrefix());
  this->_currentSnapshotId.setCounter(Utils::fromBigEndian<uint64_t>(currentSnapshotId));

}

ERC20Snapshot::ERC20Snapshot(
  const std::string &erc20_name, const std::string &erc20_symbol,
  const uint8_t &erc20_decimals, const uint256_t &mintValue,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db
) : DynamicContract(interface, "ERC20Snapshot", address, creator, chainId, db),
    ERC20("ERC20Snapshot", erc20_name, erc20_symbol, erc20_decimals, mintValue, interface, address, creator, chainId, db),
    _accountBalanceSnapshots(this), _totalSupplySnapshots(this), _currentSnapshotId(this) {
  _accountBalanceSnapshots.commit();
  _totalSupplySnapshots.ids.commit();
  _totalSupplySnapshots.values.commit();

  this->registerContractFunctions();
}


ERC20Snapshot::ERC20Snapshot(
  const std::string &derivedTypeName,
  const std::string &erc20_name, const std::string &erc20_symbol,
  const uint8_t &erc20_decimals, const uint256_t &mintValue,
  ContractManagerInterface &interface,
  const Address &address, const Address &creator, const uint64_t &chainId,
  const std::unique_ptr<DB> &db)
  : DynamicContract(interface, derivedTypeName, address, creator, chainId, db),
    ERC20(derivedTypeName, erc20_name, erc20_symbol, erc20_decimals, mintValue, interface, address, creator, chainId, db),
    _accountBalanceSnapshots(this), _totalSupplySnapshots(this), _currentSnapshotId(this) {
  _accountBalanceSnapshots.commit();
  _totalSupplySnapshots.ids.commit();
  _totalSupplySnapshots.values.commit();

  this->registerContractFunctions();
}

/// Destructor.
ERC20Snapshot::~ERC20Snapshot() {
  /// We need to dump the following data to the database:
  DBBatch batchedOperations;

  for (auto it = _accountBalanceSnapshots.begin(); it != _accountBalanceSnapshots.end(); ++it) {
    const Address& address = it->first;
    const Snapshots& snapshots = it->second;
    for (uint64_t i = 0; i < snapshots.ids.size(); ++i) {
      Bytes key = address.asBytes();
      Utils::appendBytes(key, Utils::uint64ToBytes(i));
      batchedOperations.push_back(key, Utils::uintToBytes(snapshots.ids[i]), this->getNewPrefix("_accountBalanceSnapshots_ids"));
    }
    for (uint64_t i = 0; i < snapshots.values.size(); ++i) {
      Bytes key = address.asBytes();
      Utils::appendBytes(key, Utils::uint64ToBytes(i));
      batchedOperations.push_back(key, Utils::uintToBytes(snapshots.values[i]), this->getNewPrefix("_accountBalanceSnapshots_values"));
    }
  }

  for (uint64_t i = 0; i < _totalSupplySnapshots.ids.size(); ++i) {
    Bytes key;
    Utils::appendBytes(key, Utils::uint64ToBytes(i));
    batchedOperations.push_back(key, Utils::uintToBytes(_totalSupplySnapshots.ids[i]), this->getNewPrefix("_totalSupplySnapshots_ids"));
  }

  for (uint64_t i = 0; i < _totalSupplySnapshots.values.size(); ++i) {
    Bytes key;
    Utils::appendBytes(key, Utils::uint64ToBytes(i));
    batchedOperations.push_back(key, Utils::uintToBytes(_totalSupplySnapshots.values[i]), this->getNewPrefix("_totalSupplySnapshots_values"));
  }

  this->db->put(Utils::stringToBytes("_currentSnapshotId"), Utils::uintToBytes(_currentSnapshotId.current()), this->getDBPrefix());
  this->db->putBatch(batchedOperations);
}

void ERC20Snapshot::registerContractFunctions() {
  this->registerContract();
  this->registerMemberFunction("balanceOfAt", &ERC20Snapshot::balanceOfAt, this);
  this->registerMemberFunction("totalSupplyAt", &ERC20Snapshot::totalSupplyAt, this);
}

uint256_t ERC20Snapshot::_snapshot() {
  this->_currentSnapshotId.increment();
  uint256_t currentId = this->_getCurrentSnapshotId();
  return currentId;
}

uint256_t ERC20Snapshot::_getCurrentSnapshotId() const {
  return this->_currentSnapshotId.current();
}

void ERC20Snapshot::_update(const Address& from, const Address& to, const uint256_t& value) {
  /// Accordingly to the old ERC20Snapshot standard (_beforeTokenTransfer)
  /// We can only call the ERC20::_update function AFTER the transfer has been made.
  if (from == Address()) {
    /// Mint
    this->_updateAccountSnapshot(to);
    this->_updateTotalSupplySnapshot();
  } else if (to == Address()) {
    /// Burn
    this->_updateAccountSnapshot(from);
    this->_updateTotalSupplySnapshot();
  } else {
    /// Transfer
    this->_updateAccountSnapshot(from);
    this->_updateAccountSnapshot(to);
  }

  if (typeid(*this) == typeid(ERC20Snapshot)) {
    ERC20::_update(from, to, value);
  } /// If deriving from ERC20Snapshot, then then the derived class will call ERC20::_update
}

template <typename SnapshotType>
std::pair<bool, uint256_t> ERC20Snapshot::_valueAt(const uint256_t& snapshotId, const SnapshotType& snapshots) const {
  if (snapshotId == 0) {
    throw std::runtime_error("ERC20Snapshot::_valueAt: snapshotId cannot be 0");
  }

  if (snapshotId > this->_getCurrentSnapshotId()) {
    throw std::runtime_error("ERC20Snapshot::_valueAt: snapshotId cannot be greater than current snapshot id");
  }
  uint64_t index;
  for (auto it = snapshots.ids.cbegin(); it != snapshots.ids.cend(); ++it) {
    if (*it == snapshotId) {
      index = it - snapshots.ids.cbegin();
      break;
    }
    index = snapshots.ids.size();
  }
  if (index == snapshots.ids.size()) {
    return std::make_pair(false, 0);
  } else {
    return std::make_pair(true, snapshots.values[index]);
  }
}

void ERC20Snapshot::_updateAccountSnapshot(const Address& account) {
  this->_updateSnapshot(_accountBalanceSnapshots[account], this->balanceOf(account));
}

void ERC20Snapshot::_updateTotalSupplySnapshot() {
  this->_updateSnapshot(this->_totalSupplySnapshots, this->totalSupply());
}

template <typename SnapshotsType>
void ERC20Snapshot::_updateSnapshot(SnapshotsType& snapshots, const uint256_t& currentValue) {
  uint256_t currentId = this->_getCurrentSnapshotId();
  if (this->_lastSnapshotId(snapshots.ids) < currentId) {
    snapshots.ids.push_back(currentId);
    snapshots.values.push_back(currentValue);
  }
}

template <template <typename> class VectorType>
uint256_t ERC20Snapshot::_lastSnapshotId(const VectorType<uint256_t>& ids) const {
  if (ids.size() == 0) {
    return 0;
  } else {
    return ids[ids.size() - 1];
  }
}

uint256_t ERC20Snapshot::balanceOfAt(const Address& account, const uint256_t& snapshotId) const {
  auto [snapshotted, value] = this->_valueAt(snapshotId, this->_accountBalanceSnapshots.at(account));
  return snapshotted ? value : this->balanceOf(account);
}

uint256_t ERC20Snapshot::totalSupplyAt(const uint256_t& snapshotId) const {
  auto [snapshotted, value] = this->_valueAt(snapshotId, this->_totalSupplySnapshots);
  return snapshotted ? value : this->totalSupply();
}