#include "state.h"

State::State(const std::unique_ptr<DB>& db,
             const std::unique_ptr<Storage>& storage,
             const std::unique_ptr<rdPoS>& rdpos,
             const std::unique_ptr<P2P::ManagerNormal>& p2pManager) :
             db(db),
             storage(storage),
             rdpos(rdpos),
             p2pManager(p2pManager) {
  std::unique_lock lock(this->stateMutex);
  auto accountsFromDB = db->getBatch(DBPrefix::nativeAccounts);
  if (accountsFromDB.empty()) {
    /// Initialize with 0x00dead00665771855a34155f5e7405489df2c3c6 with nonce 0.
    Address dev1(Hex::toBytes("0x00dead00665771855a34155f5e7405489df2c3c6"), true);
    /// See ~State for encoding
    uint256_t desiredBalance("1000000000000000000000");
    std::string value = Utils::uintToBytes(Utils::bytesRequired(desiredBalance)) + Utils::uintToBytes(desiredBalance) + '\x00';
    db->put(dev1.get(), value, DBPrefix::nativeAccounts);
    accountsFromDB = db->getBatch(DBPrefix::nativeAccounts);
  }

  for (auto const dbEntry : accountsFromDB) {
    std::string_view data(dbEntry.value);
    if (dbEntry.key.size() != 20) {
      Utils::logToDebug(Log::state, __func__, "Error when loading State from DB, address from DB size mismatch");
      throw std::runtime_error("Error when loading State from DB, address from DB size mismatch");
    }
    uint8_t balanceSize = Utils::fromBigEndian<uint8_t>(data.substr(0,1));
    if (data.size() + 1 < data.size()) {
      Utils::logToDebug(Log::state, __func__, "Error when loading State from DB, value from DB doesn't size mismatch on balanceSize");
      throw std::runtime_error("Error when loading State from DB, value from DB size mismatch on balanceSize");
    }

    uint256_t balance = Utils::fromBigEndian<uint256_t>(data.substr(1, balanceSize));
    uint8_t nonceSize = Utils::fromBigEndian<uint8_t>(data.substr(1 + balanceSize, 1));

    if (2 + balanceSize + nonceSize != data.size()) {
      Utils::logToDebug(Log::state, __func__, "Error when loading State from DB, value from DB doesn't size mismatch on nonceSize");
      throw std::runtime_error("Error when loading State from DB, value from DB size mismatch on nonceSize");
    }
    uint64_t nonce = Utils::fromBigEndian<uint64_t>(data.substr(2 + balanceSize, nonceSize));

    this->accounts.insert({Address(dbEntry.key, true), Account(std::move(balance), std::move(nonce))});
  }
}

State::~State() {
  /// DB is stored as following
  /// Under the DBPrefix::nativeAccounts
  /// Each key == Address
  /// Each Value == Balance + uint256_t (not exact bytes)
  /// Value == 1 Byte (Balance Size) + N Bytes (Balance) + 1 Byte (Nonce Size) + N Bytes (Nonce).
  /// Max size for Value = 32 Bytes, Max Size for Nonce = 8 Bytes.
  /// If the nonce equals to 0, it will be *empty*
  DBBatch accountsBatch;
  std::unique_lock lock(this->stateMutex);
  for (const auto& [address, account] : this->accounts) {
    // Serialize Balance.
    std::string serializedStr = (account.balance == 0) ? std::string(1, 0x00) : Utils::uintToBytes(Utils::bytesRequired(account.balance)) + Utils::uintToBytes(account.balance);
    // Serialize Account.
    serializedStr += (account.nonce == 0) ? std::string(1, 0x00) : Utils::uintToBytes(Utils::bytesRequired(account.nonce)) + Utils::uintToBytes(account.nonce);
    accountsBatch.puts.emplace_back(DBEntry(address.get(), std::move(serializedStr)));
  }

  this->db->putBatch(accountsBatch, DBPrefix::nativeAccounts);
}

const uint256_t State::getNativeBalance(const Address &addr) const {
  std::shared_lock lock(this->stateMutex);
  auto it = this->accounts.find(addr);
  if (it == this->accounts.end()) return 0;
  return it->second.balance;
}

const uint256_t State::getNativeNonce(const Address& addr) const {
  std::shared_lock lock(this->stateMutex);
  auto it = this->accounts.find(addr);
  if (it == this->accounts.end()) return 0;
  return it->second.nonce;
}