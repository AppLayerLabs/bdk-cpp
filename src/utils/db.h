/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef DB_H
#define DB_H

#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>

#include "utils.h"
#include "dynamicexception.h"

/// Namespace for accessing database prefixes.
namespace DBPrefix {
  const Bytes blocks =            { 0x00, 0x01 }; ///< "blocks" = "0001"
  const Bytes heightToBlock =     { 0x00, 0x02 }; ///< "heightToBlock" = "0002"
  const Bytes nativeAccounts =    { 0x00, 0x03 }; ///< "nativeAccounts" = "0003"
  const Bytes txToBlock =         { 0x00, 0x04 }; ///< "txToBlock" = "0004"
  const Bytes rdPoS =             { 0x00, 0x05 }; ///< "rdPoS" = "0005"
  const Bytes contracts =         { 0x00, 0x06 }; ///< "contracts" = "0006"
  const Bytes contractManager =   { 0x00, 0x07 }; ///< "contractManager" = "0007"
  const Bytes events =            { 0x00, 0x08 }; ///< "events" = "0008"
  const Bytes vmStorage =         { 0x00, 0x09 }; ///< "evmHost" = "0009"
  const Bytes txToAddr =          { 0x00, 0x0A }; ///< "txToAddr" = "000A"
  const Bytes tagToBlock =        { 0x00, 0x0B }; ///< "tagToBlock" = "000B"
  const Bytes txToGasUsed =       { 0x00, 0x0C }; ///< "txToGasUsed" = "000C"
  const Bytes txToCallTrace =     { 0x00, 0x0D }; ///< "txToCallTrace" = "000D"
};

/// Struct for a database connection/endpoint.
struct DBServer {
  std::string host;     ///< Database host/address.
  std::string version;  ///< Database version.

  /**
   * Constructor.
   * @param host The database's host/address.
   * @param version The database's version string.
   */
  DBServer(const std::string& host, const std::string& version) : host(host), version(version) {};
};

/// Struct for a database entry (key/value).
struct DBEntry {
  Bytes key;    ///< Entry key.
  Bytes value;  ///< Entry value.

  /**
   * Constructor.
   * @param key The entry's key.
   * @param value The entry's value.
   */
  DBEntry(const Bytes& key, const Bytes& value) : key(key), value(value) {};

  ///@{
  /**
   * Move constructor.
   * @param key The entry's key.
   * @param value The entry's value.
   */
  DBEntry(Bytes&& key, Bytes&& value) : key(std::move(key)), value(std::move(value)) {};
  DBEntry(const Bytes& key, Bytes&& value) : key(key), value(std::move(value)) {};
  DBEntry(Bytes&& key, const Bytes& value) : key(std::move(key)), value(value) {};
  ///@}
};

/**
 * Abstraction of a database batch request.
 * Several requests can be grouped here to be issued at once.
 * Automatically adds the appropriate prefix to keys and creates the respective
 * slices for the internal database object referencing the inner vectors.
 */
class DBBatch {
  private:
    std::vector<DBEntry> puts_; ///< List of entries to insert.
    std::vector<Bytes> dels_; ///< List of entries to delete.

  public:
    DBBatch() = default; ///< Default constructor.

  /**
   * Add a put entry to the batch.
   * @param key The entry's key.
   * @param value The entry's value.
   * @param prefix The entry's prefix.
   */
  void push_back(const bytes::View key, const bytes::View value, const Bytes& prefix) {
    Bytes tmp = prefix;
    tmp.reserve(prefix.size() + key.size());
    tmp.insert(tmp.end(), key.begin(), key.end());
    puts_.emplace_back(std::move(tmp), Bytes(value.begin(), value.end()));
  }

    /**
     * Add a DBEntry to the batch.
     * @param entry The entry to add.
     */
    void push_back(const DBEntry& entry) { puts_.push_back(entry); }

    /**
     * Add a delete entry to the batch.
     * @param key The entry's key.
     * @param prefix The entry's prefix.
     */
    void delete_key(const bytes::View key, const Bytes& prefix) {
      Bytes tmp = prefix;
      tmp.reserve(prefix.size() + key.size());
      tmp.insert(tmp.end(), key.begin(), key.end());
      dels_.emplace_back(std::move(tmp));
    }

    /**
     * Add a delete entry (with a simple Bytes key) to the batch.
     * @param bytes The entry's key.
     */
    void delete_key(const Bytes& bytes) { dels_.push_back(bytes); }

    /// Get the list of put entries.
    inline const std::vector<DBEntry>& getPuts() const { return puts_; }

    /// Get the list of delete entries.
    inline const std::vector<Bytes>& getDels() const { return dels_; }
};

/**
 * Abstraction of a [Speedb](https://github.com/speedb-io/speedb) database (Speedb is a RocksDB drop-in replacement).
 * Keys begin with prefixes that separate entries in several categories. @see DBPrefix
 */
class DB {
  private:
    rocksdb::DB* db_;               ///< Pointer to the database object itself.
    rocksdb::Options opts_;         ///< Struct with options for managing the database.
    mutable std::mutex batchLock_;  ///< Mutex for managing read/write access to batch operations.

  public:
    /**
     * Constructor. Automatically creates the database if it doesn't exist.
     * @param path The database's filesystem path (relative to the binary's current working directory).
     * @throw DynamicException if database opening fails.
     */
    explicit DB(const std::filesystem::path& path);

    /// Destructor. Automatically closes the database so it doesn't leave a LOCK file behind.
    ~DB() { this->close(); delete this->db_; this->db_ = nullptr; }

    /// Close the database connection.
    inline bool close() const { this->db_->Close(); return true; }

    /**
     * Check if a key exists in the database.
     * @tparam BytesContainer Any container that stores Bytes.
     * @param key The key to search for.
     * @param pfx (optional) The prefix to search for. Defaults to none.
     * @return `true` if the key exists, `false` otherwise.
     */
    template <typename BytesContainer> bool has(const BytesContainer& key, const Bytes& pfx = {}) const {
      std::unique_ptr<rocksdb::Iterator> it(this->db_->NewIterator(rocksdb::ReadOptions()));
      Bytes keyTmp = pfx;
      keyTmp.reserve(pfx.size() + key.size());
      keyTmp.insert(keyTmp.end(), key.begin(), key.end());
      rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyTmp.data()), keyTmp.size());
      for (it->Seek(keySlice); it->Valid(); it->Next()) {
        if (it->key() == keySlice) { it.reset(); return true; }
      }
      it.reset();
      return false;
    }

    /**
     * Get a value from a given key in the database.
     * @tparam BytesContainer Any container that stores Bytes.
     * @param key The key to search for.
     * @param pfx (optional) The prefix to search for. Defaults to none.
     * @return The requested value, or an empty Bytes object if the key doesn't exist.
     */
    template <typename BytesContainer> Bytes get(const BytesContainer& key, const Bytes& pfx = {}) const {
      std::unique_ptr<rocksdb::Iterator> it(this->db_->NewIterator(rocksdb::ReadOptions()));
      Bytes keyTmp = pfx;
      keyTmp.reserve(pfx.size() + key.size());
      keyTmp.insert(keyTmp.end(), key.cbegin(), key.cend());
      rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyTmp.data()), keyTmp.size());
      for (it->Seek(keySlice); it->Valid(); it->Next()) {
        if (it->key().ToString() == keySlice) {
          Bytes value(it->value().data(), it->value().data() + it->value().size());
          it.reset();
          return value;
        }
      }
      it.reset();
      return {};
    }

    /**
     * Insert an entry into the database.
     * @tparam BytesContainerKey Any container that stores Bytes (for the key).
     * @tparam BytesContainerValue Any container that stores Bytes (for the value).
     * @param key The key to insert.
     * @param value The value to insert.
     * @param pfx (optional) The prefix to insert the key into. Defaults to none.
     * @return `true` if the insert is successful, `false` otherwise.
     */
    template <typename BytesContainerKey, typename BytesContainerValue>
    bool put(const BytesContainerKey& key, const BytesContainerValue& value, const Bytes& pfx = {}) {
      Bytes keyTmp = pfx;
      keyTmp.reserve(pfx.size() + key.size());
      keyTmp.insert(keyTmp.end(), key.begin(), key.end());
      rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyTmp.data()), keyTmp.size());
      rocksdb::Slice valueSlice(reinterpret_cast<const char*>(value.data()), value.size());
      auto status = this->db_->Put(rocksdb::WriteOptions(), keySlice, valueSlice);
      if (!status.ok()) {
        LOGERROR("Failed to put key: " + Hex::fromBytes(keyTmp).get());
        return false;
      }
      return true;
    }

    /**
     * Delete an entry from the database (overload for Bytes).
     * @tparam BytesContainer Any container that stores Bytes.
     * @param key The key to delete.
     * @param pfx (optional) The prefix to delete the key from. Defaults to none.
     * @return `true` if the deletion is successful, `false` otherwise.
     */
    template <typename BytesContainer> bool del(const BytesContainer& key, const Bytes& pfx = {}) {
      auto keyTmp = pfx;
      keyTmp.reserve(pfx.size() + key.size());
      keyTmp.insert(keyTmp.end(), key.begin(), key.end());
      rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyTmp.data()), keyTmp.size());
      auto status = this->db_->Delete(rocksdb::WriteOptions(), keySlice);
      if (!status.ok()) {
        LOGERROR("Failed to delete key: " + Hex::fromBytes(keyTmp).get());
        return false;
      }
      return true;
    }

    static Bytes makeNewPrefix(Bytes prefix, const std::string& newPrefix) {
      prefix.reserve(prefix.size() + newPrefix.size());
      prefix.insert(prefix.end(), newPrefix.cbegin(), newPrefix.cend());
      return prefix;
    }

    /**
     * Delete an entry from the database (overload for C-style strings).
     * @param key The key to delete.
     * @param pfx (optional) The prefix to delete the key from. Defaults to none.
     * @return `true` if the deletion is successful, `false` otherwise.
     */
    bool del(const char* key, const Bytes& pfx = {}) { return this->del(std::string(key), pfx); }

    /**
     * Do several put and/or delete operations in one go.
     * Prefix is already included in DBBatch keys.
     * @param batch The batch object with the put/del operations to be done.
     * @return `true` if all operations were successful, `false` otherwise.
     */
    bool putBatch(const DBBatch& batch);

    /**
     * Get all entries from a given prefix.
     * @param bytesPfx The prefix to search for.
     * @param keys (optional) A list of keys to search for. Defaults to an empty list.
     * @return A list of found entries.
     */
    std::vector<DBEntry> getBatch(
      const Bytes& bytesPfx, const std::vector<Bytes>& keys = {}
      ) const;

    /**
     * Get all keys from a given prefix.
     * Ranges can be used to mitigate very expensive operations
     * (e.g. a query that returns millions of entries at once).
     * Prefix is automatically added to the queries themselves internally.
     * @param pfx The prefix to search keys from.
     * @param start (optional) The first key to start searching from. Defaults to none.
     * @param end (optional) The last key to end searching at. Defaults to none.
     * @return A list of found keys, WITHOUT their prefixes.
     */
    std::vector<Bytes> getKeys(const Bytes& pfx, const Bytes& start = {}, const Bytes& end = {}) const;

    /**
     * Create a Bytes container from a string.
     * @param str The string to convert.
     * @return The Bytes container.
     */
    inline static Bytes keyFromStr(const std::string& str) { return Bytes(str.begin(), str.end()); }
};

#endif // DB_H
