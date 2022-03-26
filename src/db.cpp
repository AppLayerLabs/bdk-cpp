#include "db.h"

bool Database::openDB() {
  this->dbOpts.create_if_missing = true;
  this->dbStatus = leveldb::DB::Open(this->dbOpts, databasePath.string(), &db);
  if (!this->dbStatus.ok()) {
    throw std::string("Error opening ") + databaseName + "database! " + this->dbStatus.ToString();
  }
  return this->dbStatus.ok();
}

bool Database::closeDB() {
  delete this->db;
  this->db = NULL;
  return true;
}

bool Database::keyExists(std::string &key) {
  leveldb::Iterator* it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    if (it->key().ToString() == key) {
      delete it;
      return true;
    }
  }
  delete it;
  return false;
}

std::string Database::getKeyValue(std::string key) {
  this->dbStatus = this->db->Get(leveldb::ReadOptions(), key, &this->tmpValue);
  if (!this->dbStatus.ok()) {
    throw std::string("Error reading at ") + databaseName + " database at key "
      + key + this->dbStatus.ToString();
  }
  return this->tmpValue;
}

bool Database::putKeyValue(std::string key, std::string value) {
  this->dbStatus = this->db->Put(leveldb::WriteOptions(), key, value);
  if (!this->dbStatus.ok()) {
    throw std::string("Error writing at ") + databaseName + " database at key "
      + key + this->dbStatus.ToString();
  }
  return this->dbStatus.ok();
}

bool Database::deleteKeyValue(std::string &key) {
  this->dbStatus = this->db->Delete(leveldb::WriteOptions(), key);
  if(!this->dbStatus.ok()) {
    throw std::string("Error deleting at ") + databaseName + " database at key "
      + key + this->dbStatus.ToString();
  }
  return this->dbStatus.ok();
}

std::vector<std::string> Database::getAllKeys() {
  Utils::logToFile("gettingall...");
  std::vector<std::string> ret;
  leveldb::Iterator* it = this->db->NewIterator(leveldb::ReadOptions());
  Utils::logToFile("gettingall2...");
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    ret.push_back(it->key().ToString());
  }
  Utils::logToFile("gettingall3s...");
  delete it;
  return ret;
}

std::vector<std::string> Database::getAllValues() {
  std::vector<std::string> ret;
  leveldb::Iterator* it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    ret.push_back(it->value().ToString());
  }
  delete it;
  return ret;
}

std::map<std::string, std::string> Database::getAllPairs() {
  std::map<std::string, std::string> ret;
  leveldb::Iterator* it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    ret.emplace(it->key().ToString(), it->value().ToString());
  }
  delete it;
  return ret;
}

void Database::dropDatabase() {
  leveldb::Iterator* it = this->db->NewIterator(leveldb::ReadOptions());
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    this->db->Delete(leveldb::WriteOptions(), it->key().ToString());
  }
  delete it;
}

