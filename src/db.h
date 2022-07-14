#ifndef DB_H
#define DB_H

#include <iostream>

#include "utils.h"

#include <leveldb/db.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

/**
 * As subnets are meant to be run inside a sandbox, we can't create our own DB.
 * We have to use the DB that AvalancheGo provides for us through gRPC.
 * It's a simple key/value store database, similar to LevelDB, but it allows
 * for writing in batch and reading all keys using a given prefix.
 * Database structure is as follows:
 * 0001 -- Key: Block Hash            Value: Block
 * 0002 -- Key: Block nHeight         Value: Block Hash
 * 0003 -- Key: Tx Hash               Value: Transactions
 * 0004 -- Key: Address               Value: Native Balance + nNonce
 * 0005 -- ERC20 Tokens/State
 * 0006 -- ERC721 Tokens/State
 * 0007 -- Key: Tx Hash               Value: Block Hash
 */

namespace DBPrefix {
  const std::string blocks = "0001";
  const std::string blockHeightMaps = "0002";
  const std::string transactions = "0003";
  const std::string nativeAccounts = "0004";
  const std::string erc20Tokens = "0005";
  const std::string erc721Tokens = "0006";
  const std::string TxToBlocks = "0007";
}

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

struct WriteBatchRequest {
  std::vector<DBEntry> puts;
  std::vector<std::string> dels;
  int64_t id;
  bool continues;
};

class DBService {
  private:
    leveldb::DB* db;
    leveldb::Options options;
    std::mutex batchLock;
    boost::filesystem::path dbPath;

  public:
    DBService(std::string path) {
      boost::replace_all(path, "/", "");
      options.create_if_missing = true;
      dbPath = boost::filesystem::current_path().string() + std::string("/") + path;
      auto status = leveldb::DB::Open(this->options, dbPath.string(), &db);
      if (!status.ok()) {
        Utils::LogPrint(Log::db, __func__, "Failed to open DB: " + status.ToString());
      }
    }

    bool has(std::string key, std::string prefix = "");
    std::string get(std::string key, std::string prefix = "");
    bool put(std::string key, std::string data, std::string prefix = "");
    bool del(std::string key, std::string prefix = "");
    bool close();
    bool writeBatch(WriteBatchRequest &request, std::string prefix = "");

    // Read all keys starting with prefix and start.
    std::vector<DBEntry> readBatch(std::string prefix);

    // Read all keys from key vector.
    std::vector<DBEntry> readBatch(std::vector<std::string>& keys, std::string prefix);

    // Remove the first 4 chars (the key) from a string.
    std::string removeKeyPrefix(const std::string &key);
};

#endif  // DB_H
