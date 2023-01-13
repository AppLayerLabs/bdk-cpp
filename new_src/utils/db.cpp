#include "db.h"
#include "utils.h"

bool DB::has(const std::string& key, const std::string& pfx) {
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(pfx+key); it->Valid(); it->Next()) {
    if (it->key().ToString() == pfx+key) {
      delete it;
      return true;
    }
  }
  delete it;
  return false;
}

std::string DB::get(const std::string& key, const std::string& pfx) {
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(pfx+key); it->Valid(); it->Next()) {
    if (it->key().ToString() == pfx+key) {
      std::string value = it->value().ToString();
      delete it;
      return value;
    }
  }
  delete it;
  return "";
}

bool DB::put(const std::string& key, const std::string& value, const std::string& pfx) {
  auto status = this->db->Put(leveldb::WriteOptions(), pfx+key, value);
  if (!status.ok()) {
    Utils::logToDebug(Log::db, __func__, "Failed to put key: " + key);
    return false;
  }
  return true;
}

bool DB::del(const std::string& key, const std::string& pfx) {
  auto status = this->db->Delete(leveldb::WriteOptions(), pfx+key);
  if (!status.ok()) {
    Utils::logToDebug(Log::db, __func__, "Failed to delete key: " + key);
    return false;
  }
  return true;
}

std::vector<DBEntry> DB::getBatch(const std::string& pfx, const std::vector<std::string>& keys) {
  batchLock.lock();
  std::vector<DBEntry> ret;
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());

  if(keys.empty()){
    for (it->Seek(pfx); it->Valid(); it->Next()) {
      if (it->key().ToString().substr(0, 4) == pfx) {
        DBEntry entry(stripPrefix(it->key().ToString()), it->value().ToString());
        ret.push_back(entry);
      }
    }   
  delete it;
  batchLock.unlock();
  return ret;
  }

  for (it->Seek(pfx); it->Valid(); it->Next()) {
    if (it->key().ToString().substr(0, 4) == pfx) {
      std::string strippedKey = stripPrefix(it->key().ToString());
      for (const std::string& key : keys) {
        if
         (strippedKey == key){
          DBEntry entry(strippedKey, it->value().ToString());
          ret.push_back(entry);
          }
        }
      }
    }
  delete it;
  batchLock.unlock();
  return ret;
}


bool DB::putBatch(DBBatch& request, const std::string& pfx) {
  batchLock.lock();
  for (auto &entry : request.puts) {
    auto status = this->db->Put(leveldb::WriteOptions(), pfx+entry.key, entry.value);
    if (!status.ok()) return false;
  }
  for (std::string& key : request.dels) {
    auto status = this->db->Delete(leveldb::WriteOptions(), pfx+key);
    if (status.ok()) return false;
  }
  batchLock.unlock();
  return true;
}


