#include "db.h"

std::string DBService::removeKeyPrefix(const std::string &key) {
  return key.substr(4);
}

bool DBService::has(std::string key, std::string prefix) {
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(prefix+key); it->Valid(); it->Next()) {
    if (it->key().ToString() == prefix+key) {
      delete it;
      return true;
    }
  }
  delete it;
  return false;
}

std::string DBService::get(std::string key, std::string prefix) {
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(prefix+key); it->Valid(); it->Next()) {
    if (it->key().ToString() == prefix+key) {
      std::string value = it->value().ToString();
      delete it;
      return value;
    }
  }
  delete it;
  return "";
}

bool DBService::put(std::string key, std::string value, std::string prefix) {
  auto status = this->db->Put(leveldb::WriteOptions(), prefix+key, value);
  if (!status.ok()) {
    Utils::LogPrint(Log::db, __func__, "Failed to put key: " + key);
    return false;
  }
  return true;
}

bool DBService::del(std::string key, std::string prefix) {
  auto status = this->db->Delete(leveldb::WriteOptions(), prefix+key);
  if (!status.ok()) {
    Utils::LogPrint(Log::db, __func__, "Failed to delete key: " + key);
    return false;
  }
  return true;
}

bool DBService::close() {
  delete this->db;
  this->db = NULL;
  return true;
}

bool DBService::writeBatch(WriteBatchRequest &request, std::string prefix) {
  batchLock.lock();
  for (auto &entry : request.puts) {
    auto status = this->db->Put(leveldb::WriteOptions(), prefix+entry.key, entry.value);
    if (!status.ok()) return false;
  }
  for (std::string &key : request.dels) {
    auto status = this->db->Delete(leveldb::WriteOptions(), prefix+key);
    if (status.ok()) return false;
  }
  batchLock.unlock();
  return true;
}

std::vector<DBEntry> DBService::readBatch(std::string const prefix) {
  batchLock.lock();
  std::vector<DBEntry> entries;
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(prefix); it->Valid(); it->Next()) {
    if (it->key().ToString().substr(0, 4) == prefix) {
      DBEntry entry(removeKeyPrefix(it->key().ToString()), it->value().ToString());
      entries.push_back(entry);
    }
  }
  delete it;
  batchLock.unlock();
  return entries;
}

std::vector<DBEntry> DBService::readBatch(const std::vector<std::string>& keys, const std::string prefix) {
  batchLock.lock();
  std::vector<DBEntry> ret;
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(prefix); it->Valid(); it->Next()) {
    if (it->key().ToString().substr(0, 4) == prefix) {
      std::string strippedKey = removeKeyPrefix(it->key().ToString());
      for (const std::string& key : keys) {
        if (strippedKey == key) {
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

