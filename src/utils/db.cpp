#include "db.h"

DB::DB(const std::string path) {
  this->opts.create_if_missing = true;
  if (!std::filesystem::exists(path)) { // LevelDB is dumb as a brick so we need this
    std::filesystem::create_directories(path);
  }
  auto status = leveldb::DB::Open(this->opts, path, &this->db);
  if (!status.ok()) {
    Utils::logToDebug(Log::db, __func__, "Failed to open DB: " + status.ToString());
    throw std::runtime_error("Failed to open DB: " + status.ToString());
  }
}

bool DB::has(const std::string& key, const std::string& pfx) {
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  std::string strForSlice = std::string(pfx + key);
  leveldb::Slice slice(strForSlice);
  for (it->Seek(slice); it->Valid(); it->Next()) {
    if (it->key() == slice) { delete it; return true; }
  }
  delete it;
  return false;
}

std::string DB::get(const std::string& key, const std::string& pfx) const {
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  std::string strForSlice = std::string(pfx + key);
  leveldb::Slice slice(strForSlice);
  for (it->Seek(slice); it->Valid(); it->Next()) {
    if (it->key().ToString() == slice) {
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
  leveldb::WriteBatch wb;
  for (const std::string key : batch.dels) wb.Delete(pfx + key);
  for (const DBEntry entry : batch.puts) wb.Put(pfx + entry.key, entry.value);
  leveldb::Status s = this->db->Write(leveldb::WriteOptions(), &wb);
  return s.ok();
}

std::vector<DBEntry> DB::getBatch(
  const leveldb::Slice& pfx, const std::vector<std::string>& keys
) const {
  std::lock_guard lock(batchLock);
  std::vector<DBEntry> ret;
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());

  // Search for all entries
  if (keys.empty()) {
    for (it->Seek(pfx); it->Valid(); it->Next()) {
      if (it->key().starts_with(pfx)) {
        auto keySlice = it->key();
        keySlice.remove_prefix(pfx.size());
        ret.emplace_back(keySlice.ToString(), it->value().ToString());
      }
    }
    delete it;
    return ret;
  }

  // Search for specific entries from keys
  for (it->Seek(pfx); it->Valid(); it->Next()) {
    if (it->key().starts_with(pfx)) {
      auto keySlice = it->key();
      keySlice.remove_prefix(pfx.size());
      for (const std::string& key : keys) {
        if (keySlice == leveldb::Slice(key)) {
          ret.emplace_back(keySlice.ToString(), it->value().ToString());
        }
      }
    }
  }
  delete it;
  return ret;
}

