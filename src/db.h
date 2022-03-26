#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include "json.hpp"
#include <boost/filesystem.hpp>
#include <leveldb/db.h>
#include <boost/algorithm/string.hpp>
#include "utils.h"

using json = nlohmann::ordered_json;

class Database {
  private:
    std::string databaseName;
    boost::filesystem::path databasePath;
    bool openDB();
    bool closeDB();
    leveldb::DB* db;
    leveldb::Options dbOpts;
    leveldb::Status dbStatus;
    std::string tmpValue;

  public:
    
    void setAndOpenDB(std::string name) {
      boost::replace_all(name, "/", "");
      databaseName = name;
      databasePath = boost::filesystem::current_path().string() + std::string("/") + name;
      openDB();
      return;
    }
    // Functions.
    bool cleanCloseDB() { return closeDB(); };
    bool isDBOpen() { return (this->db != NULL); }
    bool keyExists(std::string &key);
    std::string getKeyValue(std::string key);
    bool putKeyValue(std::string key, std::string value);
    bool deleteKeyValue(std::string &key);
    std::vector<std::string> getAllKeys();
    std::vector<std::string> getAllValues();
    bool isEmpty() {
      return (this->getAllKeys().size() == 0);
    }
    std::map<std::string, std::string> getAllPairs();
    void dropDatabase();
};

#endif  // DATABASE_H
