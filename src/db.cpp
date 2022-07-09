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

// TODO: fix segfault when starting subnet and terminating it with SIGTERM
bool DBService::close() {
  delete this->db;
  this->db = NULL;
  return true;
}

std::vector<DBEntry> DBService::readBatch(std::string prefix) {
  std::vector<DBEntry> entries;
  leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->Seek(prefix); it->Valid(); it->Next()) {
    if (it->key().ToString().substr(0, 4) == prefix) {
      DBEntry entry(removeKeyPrefix(it->key().ToString()), it->value().ToString());
      entries.push_back(entry);
    }
  }
  delete it;
  return entries;
}

// TODO: implement proper batch functions
bool DBService::writeBatch(WriteBatchRequest &request, std::string prefix) {
  for (auto &entry : request.puts) {
    if (!this->put(entry.key, entry.value, prefix)) {
      return false;
    }
  }
  for (auto &entry : request.dels) {
    if (!this->del(entry.key, prefix)) {
      return false;
    }
  }
  return true;
}

std::vector<DBEntry> DBService::readBatch(std::vector<DBKey>& keys, std::string prefix) {
  std::vector<DBEntry> ret;
  for (auto &key : keys) {
    ret.push_back({key.key, get(key.key, prefix)});
  }
  return ret;
}

