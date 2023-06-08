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

/**
 * Namespace for accessing database prefixes. Values are:
 * - blocks = "0001"
 * - blockHeightMaps = "0002"
 * - nativeAccounts = "0003"
 * - txToBlocks = "0004"
 * - rdPoS = "0005"
 * - contracts = "0006"
 * - contractManager = "0007"
 */
namespace DBPrefix {
  const Bytes blocks = { 0x00, 0x01 };
  const Bytes blockHeightMaps = { 0x00, 0x02 };
  const Bytes nativeAccounts = { 0x00, 0x03 };
  const Bytes txToBlocks = { 0x00, 0x04 };
  const Bytes rdPoS = { 0x00, 0x05 };
  const Bytes contracts = { 0x00, 0x06 };
  const Bytes contractManager = { 0x00, 0x07 };
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
  DBEntry(Bytes&& key, Bytes&& value) : key(std::move(key)), value(std::move(value)) {};
  DBEntry(const Bytes& key, Bytes&& value) : key(key), value(std::move(value)) {};
  DBEntry(Bytes&& key, const Bytes& value) : key(std::move(key)), value(value) {};
};

/**
 * Class for a database batch request.
 * Several requests can be grouped here to be issued at once.
 * Requests grouped within DBBatch will automatically add the appropriate prefix to their keys.
 * Automatically create respective slices for RocksDB referencing the inner vectors.
 */
class DBBatch {
  private:
    std::vector<DBEntry> puts;      ///< List of entries to insert.
    std::vector<Bytes> dels;        ///< List of entries to delete.
    std::vector<std::pair<rocksdb::Slice, rocksdb::Slice>> putsSlices; ///< List of slices to insert. (key/value)
    std::vector<rocksdb::Slice> delsSlices; ///< List of slices to delete. (key)
  public:
    DBBatch() = default; ///< Default constructor.

    /**
     * Add an puts entry to the batch.
     * @param key The entry's key.
     * @param value The entry's value.
     */
    void push_back(const BytesArrView key, const BytesArrView value, const Bytes& prefix) {
      Bytes tmp = prefix;
      tmp.reserve(prefix.size() + key.size());
      tmp.insert(tmp.end(), key.begin(), key.end());
      puts.emplace_back(std::move(tmp), Bytes(value.begin(), value.end()));
      putsSlices.emplace_back(rocksdb::Slice(reinterpret_cast<const char*>(puts.back().key.data()), puts.back().key.size()),
                              rocksdb::Slice(reinterpret_cast<const char*>(puts.back().value.data()), puts.back().value.size()));
    }

    /**
     * Add an delete entry to the batch.
     * @param key The entry's key.
     * @param value The entry's value.
     */
    void delete_key(const BytesArrView key, const Bytes& prefix) {
      Bytes tmp = prefix;
      tmp.reserve(prefix.size() + key.size());
      tmp.insert(tmp.end(), key.begin(), key.end());
      dels.emplace_back(std::move(tmp));
      delsSlices.emplace_back(rocksdb::Slice(reinterpret_cast<const char*>(dels.back().data()), dels.back().size()));
    }

    /**
     * Get the list of puts entries.
     * @return The list of puts entries.
     */
    inline const std::vector<DBEntry>& getPuts() const { return puts; }

    /**
     * Get the list of delete entries.
     * @return The list of delete entries.
     */
    inline const std::vector<Bytes>& getDels() const { return dels; }

    /**
     * Get the list of puts slices.
     * @return The list of puts slices.
     */
    inline const std::vector<std::pair<rocksdb::Slice, rocksdb::Slice>>& getPutsSlices() const { return putsSlices; }

    /**
     * Get the list of delete slices.
     * @return The list of delete slices.
     */
    inline const std::vector<rocksdb::Slice>& getDelsSlices() const { return delsSlices; }
};

/**
 * Abstraction of a [RocksDB](https://github.com/facebook/rocksdb) database.
 * Keys begin with prefixes that separate entries in several categories (see DBPrefix).
 */
class DB {
  private:
    rocksdb::DB* db;              ///< Pointer to the database object itself.
    rocksdb::Options opts;        ///< Struct with options for managing the database.
    mutable std::mutex batchLock; ///< Mutex for managing read/write access to batch operations.

  public:
    /**
     * Constructor. Automatically creates the database if it doesn't exist. Throws on error.
     * @param path The database's filesystem path (relative to the binary's current working directory).
     */
    DB(const std::string path);

    /**
     * Destructor. Automatically closes the database so it doesn't leave a LOCK file behind.
     */
    ~DB() { this->close(); }

    /**
     * Close the database. "Closing" a RocksDB database is just deleting its object.
     * @return `true` if the database is closed successfully, `false` otherwise.
     */
    inline bool close() { delete this->db; this->db = nullptr; return (this->db == nullptr); }

    /**
     * Check if a key exists in the database.
     * @param key The key to search for.
     * @param pfx (optional) The prefix to search for. Defaults to an empty string.
     * @return `true` if the key exists, `false` otherwise.
     */
    template <typename BytesContainer>
    bool has(const BytesContainer& key, const Bytes& pfx = {}) {
      rocksdb::Iterator *it = this->db->NewIterator(rocksdb::ReadOptions());
      Bytes keyTmp = pfx;
      keyTmp.reserve(pfx.size() + key.size());
      keyTmp.insert(keyTmp.end(), key.begin(), key.end());
      rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyTmp.data()), keyTmp.size());
      for (it->Seek(keySlice); it->Valid(); it->Next()) {
        if (it->key() == keySlice) { delete it; return true; }
      }
      delete it;
      return false;
    }

    /**
     * Get a value from a given key in the database.
     * @param key The key to search for.
     * @param pfx (optional) The prefix to search for. Defaults to an empty string.
     * @return The requested value, or an empty string if the key doesn't exist.
     */
    template <typename BytesContainer>
    Bytes get(const BytesContainer& key, const Bytes& pfx = {}) const {
      rocksdb::Iterator *it = this->db->NewIterator(rocksdb::ReadOptions());
      Bytes keyTmp = pfx;
      keyTmp.reserve(pfx.size() + key.size());
      keyTmp.insert(keyTmp.end(), key.begin(), key.end());
      rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyTmp.data()), keyTmp.size());
      for (it->Seek(keySlice); it->Valid(); it->Next()) {
        if (it->key().ToString() == keySlice) {
          Bytes value(it->value().data(), it->value().data() + it->value().size());
          delete it;
          return value;
        }
      }
      delete it;
      return {};
    }


    /**
     * Insert an entry into the database.
     * @param key The key to insert.
     * @param value The value to insert.
     * @param pfx (optional) The prefix to insert the key into. Defaults to an empty string.
     * @return `true` if the insert is successful, `false` otherwise.
     */
    template <typename BytesContainerTypeOne, typename BytesContainerTypeSecond>
    bool put(const BytesContainerTypeOne& key, const BytesContainerTypeSecond& value, const Bytes& pfx = {}) const {
      Bytes keyTmp = pfx;
      keyTmp.reserve(pfx.size() + key.size());
      keyTmp.insert(keyTmp.end(), key.begin(), key.end());
      rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyTmp.data()), keyTmp.size());
      rocksdb::Slice valueSlice(reinterpret_cast<const char*>(value.data()), value.size());
      auto status = this->db->Put(rocksdb::WriteOptions(), keySlice, valueSlice);
      if (!status.ok()) {
        Utils::logToDebug(Log::db, __func__, "Failed to put key: " + Hex::fromBytes(keyTmp).get());
        return false;
      }
      return true;
    }

    /**
     * Delete an entry from the database.
     * @param key The key to delete.
     * @param pfx (optional) The prefix to delete the key from. Defaults to an empty string.
     * @return `true` if the deletion is successful, `false` otherwise.
     */
    template <typename BytesContainer>
    bool del(const BytesContainer& key, const Bytes& pfx = {}) const {
      auto keyTmp = pfx;
      keyTmp.reserve(pfx.size() + key.size());
      keyTmp.insert(keyTmp.end(), key.begin(), key.end());
      rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyTmp.data()), keyTmp.size());
      auto status = this->db->Delete(rocksdb::WriteOptions(), keySlice);
      if (!status.ok()) {
        Utils::logToDebug(Log::db, __func__, "Failed to delete key: " + Hex::fromBytes(keyTmp).get());
        return false;
      }
      return true;
    }
    bool del(const char* key, const Bytes pfx = {}) const { return this->del(std::string(key), pfx); }

    /**
     * Do several put and/or delete operations in one go.
     * Pfx is already included in DBBatch keys.
     * @param batch The batch object with the operations to be done.
     * @return `true` if all operations were successful, `false` otherwise.
     */
    bool putBatch(const DBBatch& batch) const;

    /**
     * Get all entries from a given prefix.
     * @param pfx The prefix string to get entries from. Works with strings.
     * @param keys (optional) A list of keys to filter values from.
     *             Defaults to an empty list (same as "get all entries").
     * @return The list of database entries.
     */
    std::vector<DBEntry> getBatch(
      const Bytes& bytesPfx, const std::vector<Bytes>& keys = {}
    ) const;

    /**
     * Create a Bytes container from a string.
     * @param str The string to convert.
     * @return The Bytes container.
     */
    inline static Bytes keyFromStr(const std::string str) {
      return Bytes(str.begin(), str.end());
    }
};

#endif // DB_H
