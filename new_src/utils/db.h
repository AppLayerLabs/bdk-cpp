#ifndef DB_H
#define DB_H

#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

#include <leveldb/db.h>

/**
 * Namespace for accessing database prefixes. Values are:
 * - blocks = "0001"
 * - blockHeightMaps = "0002"
 * - transactions = "0003"
 * - nativeAccounts = "0004"
 * - erc20Tokens = "0005"
 * - erc721Tokens = "0006"
 * - txToBlocks = "0007"
 * - validators = "0008"
 */
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

/**
 * Struct for a database connection/endpoint.
 */
struct DBServer {
  std::string host;     ///< Database host/address.
  std::string version;  ///< Database version.

  /**
   * Constructor.
   * @param host The database's host/address.
   * @param version The database's version string.
   */
  DBServer(std::string host, std::string version) : host(host), version(version) {};
};

/**
 * Struct for a database entry (key/value).
 */
struct DBEntry {
  std::string key;    ///< Entry key.
  std::string value;  ///< Entry value.

  /**
   * Constructor.
   * @param key The entry's key.
   * @param value The entry's value.
   */
  DBEntry(std::string key, std::string value) : key(key), value(value) {};
};

/**
 * Struct for a database batch request.
 * Several requests can be grouped here to be issued at once.
 */
struct DBBatch {
  std::vector<DBEntry> puts;      ///< List of entries to insert.
  std::vector<std::string> dels;  ///< List of entries to delete.
  uint64_t id;                    ///< Unique identifier for the batch request.
  bool continues; ///< Indicates if writing will continue after gRPC request limit of *2^32 bytes*.
};

/**
 * Abstraction of a [LevelDB](https://github.com/google/leveldb) database.
 * As subnets are meant to be run inside a sandbox, we can't create our own DB.
 * We have to use the DB that AvalancheGo provides for us through gRPC.
 * Keys can begin with prefixes that separate entries in several categories (see %DBPrefix).
 */
class DB {
  private:
    leveldb::DB* db;            ///< Pointer to the database object itself.
    leveldb::Options opts;      ///< Options for managing the database.
    std::mutex batchLock;       ///< Mutex for managing read/write access to batch operations.
    std::filesystem::path path; ///< Database filesystem path.

  public:
    /**
     * Constructor.
     * @param path The database's filesystem path.
     */
    DB(std::string path);

    /**
     * Close the database. "Closing" a LevelDB database is just deleting its object.
     * @return `true` if the database is closed successfully, `false` otherwise.
     */
    inline bool close() { delete this->db; this->db = nullptr; return true; }

    /**
     * Check if a key exists in the database.
     * @param key The key to search for.
     * @param pfx (optional) The prefix to search for. Defaults to an empty string.
     * @return `true` if the key exists, `false` otherwise.
     */
    bool has(const std::string& key, const std::string& pfx = "");

    /**
     * Get a value from a given key in the database.
     * @param key The key to search for.
     * @param pfx (optional) The prefix to search for. Defaults to an empty string.
     * @return The requested value, or an empty string if the key doesn't exist.
     */
    std::string get(const std::string& key, const std::string& pfx = "");

    /**
     * Insert an entry into the database.
     * @param key The key to insert.
     * @param value The value to insert.
     * @param pfx (optional) The prefix to insert the key into.
     * @return `true` if the insert is successful, `false` otherwise.
     */
    bool put(const std::string& key, const std::string& value, const std::string& pfx = "");

    /**
     * Delete an entry from the database.
     * @param key The key to delete.
     * @param pfx (optional) The prefix to delete the key from.
     * @return `true` if the deletion is successful, `false` otherwise.
     */
    bool del(const std::string& key, const std::string& pfx = "");

    /**
     * Do several operations in one go.
     * @param batch The batch object with the operations to be done.
     * @param pfx (optional) The prefix to operate on.
     * @return `true` if all operations were successful, `false` otherwise.
     */
    bool putBatch(DBBatch& batch, const std::string& pfx = "");

    /**
     * Get all entries from a given prefix.
     * @param pfx The prefix to get entries from.
     * @param keys (optional) A list of keys to filter values from.
     *             Defaults to an empty list (same as "get all entries").
     * @return The list of database entries.
     */
    std::vector<DBEntry> getBatch(const std::string& pfx, const std::vector<std::string>& keys = {});

    /**
     * Remove the database prefix (first 4 chars) from a key.
     * @param key The key to remove the prefix from.
     * @return The stripped key.
     */
    inline std::string stripPrefix(const std::string& key) { return key.substr(4); }
};

#endif // DB_H