#ifndef DB_H
#define DB_H

#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include <leveldb/db.h>

namespace DBPrefix {
  const std::string blocks = "0001";
  const std::string blockHeightMaps = "0002";
  const std::string transactions = "0003";
  const std::string nativeAccounts = "0004";
  const std::string erc20Tokens = "0005";
  const std::string erc721Tokens = "0006";
  const std::string txToBlocks = "0007";
  const std::string validators = "0008";
};

struct DBServer {
  std::string host;
  std::string version;
  DBServer(std::string host, std::string version) : host(host), version(version) {};
};

struct DBEntry {
  std::string key;
  std::string value;
  DBEntry(std::string key, std::string value) : key(key), value(value) {};
};

struct DBBatch {
  std::vector<DBEntry> puts;
  std::vector<std::string> dels;
  uint64_t id;
  bool continues;
};

class DB {
  private:
    leveldb::DB* db;
    leveldb::Options opts;
    std::mutex batchLock;
    std::filesystem::path path;

  public:
    DB(std::string path);
    inline bool close() { delete this->db; this->db = nullptr; return true; }
    bool has(const std::string& key, const std::string& pfx = "");
    std::string get(const std::string& key, const std::string& pfx = "");
    bool put(const std::string& key, const std::string& value, const std::string& pfx = "");
    bool del(const std::string& key, const std::string& pfx = "");
    bool putBatch(DBBatch& batch, const std::string& pfx = "");
    std::vector<DBEntry> getBatch(const std::string& pfx, const std::vector<std::string>& keys = {});
    inline std::string stripPrefix(const std::string& key) { return key.substr(4); }
};

#endif // DB_H
