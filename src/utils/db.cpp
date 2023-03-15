#include "db.h"

DB::DB(const std::string path) {
  this->opts.create_if_missing = true;
  std::filesystem::path fullPath = std::filesystem::current_path().string() + std::string("/") + path;
  if (!std::filesystem::exists(fullPath)) { // LevelDB is dumb as a brick so we need this
    std::filesystem::create_directories(fullPath);
  }
  auto status = leveldb::DB::Open(this->opts, fullPath.string(), &this->db);
  if (!status.ok()) {
    Utils::logToDebug(Log::db, __func__, "Failed to open DB: " + status.ToString());
  }
}

bool DB::has(const std::string& key, const std::string& pfx) {
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(pfx + key); it->Valid(); it->Next()) {
    if (it->key().ToString() == pfx + key) { delete it; return true; }
  }
  delete it;
  return false;
}

std::string DB::get(const std::string& key, const std::string& pfx) const {
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(pfx + key); it->Valid(); it->Next()) {
    if (it->key().ToString() == pfx + key) {
      std::string value = it->value().ToString();
      delete it;
      return value;
    }
  }
  delete it;
  return "";
}

bool DB::put(const std::string& key, const std::string& value, const std::string& pfx) const {
  auto status = this->db->Put(leveldb::WriteOptions(), pfx + key, value);
  if (!status.ok()) {
    Utils::logToDebug(Log::db, __func__, "Failed to put key: " + key);
    return false;
  }
  return true;
}

bool DB::del(const std::string& key, const std::string& pfx) const {
  auto status = this->db->Delete(leveldb::WriteOptions(), pfx + key);
  if (!status.ok()) {
    Utils::logToDebug(Log::db, __func__, "Failed to delete key: " + key);
    return false;
  }
  return true;
}

bool DB::putBatch(const DBBatch& batch, const std::string& pfx) const {
  std::lock_guard lock(batchLock);
  for (const DBEntry entry : batch.puts) {
    auto status = this->db->Put(leveldb::WriteOptions(), pfx + entry.key, entry.value);
    if (!status.ok()) return false;
  }
  for (const std::string key : batch.dels) {
    auto status = this->db->Delete(leveldb::WriteOptions(), pfx + key);
    if (!status.ok()) return false;
  }
  return true;
}

std::vector<DBEntry> DB::getBatch(
  const std::string& pfx, const std::vector<std::string>& keys
) const {
  std::lock_guard lock(batchLock);
  std::vector<DBEntry> ret;
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());

  // Search for all entries
  if (keys.empty()) {
    for (it->Seek(pfx); it->Valid(); it->Next()) {
      if (it->key().ToString().substr(0, 2) == pfx) {
        DBEntry entry(this->stripPrefix(it->key().ToString()), it->value().ToString());
        ret.push_back(entry);
      }
    }
    delete it;
    return ret;
  }

  // Search for specific entries from keys
  for (it->Seek(pfx); it->Valid(); it->Next()) {
    if (it->key().ToString().substr(0, 2) == pfx) {
      std::string strippedKey = this->stripPrefix(it->key().ToString());
      for (const std::string& key : keys) {
        if (strippedKey == key) {
          DBEntry entry(strippedKey, it->value().ToString());
          ret.push_back(entry);
        }
      }
    }
  }
  delete it;
  return ret;
}

