#include "../src/libs/catch2/catch_amalgamated.hpp"
#include "../src/utils/db.h"
#include "../src/utils/strings.h"

#include <filesystem>
#include <string>

using Catch::Matchers::Equals;

namespace TDB {
  TEST_CASE("DB Tests", "[utils]") {
    SECTION("Open and Close DB + Strip Prefix From Key") {
      DB db("testDB");
      REQUIRE(std::filesystem::exists(std::filesystem::current_path().string() + "/testDB"));
      REQUIRE(db.stripPrefix("0001a4a96085") == "a4a96085");
      REQUIRE(db.close());
    }

    SECTION("Simple CRUD (Create + Read + Update + Delete)") {
      // Open
      DB db("testDB");
      std::string key = "d41472b71899ccc0cf16c09ac97af95e";
      std::string value = "5ea04e91c96033ae312af0bb22ec3e370c7789dc28858ea0135966ee2966a616";
      std::string pfx = "0001";

      // Create
      REQUIRE(db.put(key, value, pfx));
      REQUIRE(db.has(key, pfx));

      // Read
      REQUIRE(db.get(key, pfx) == value);

      // Update
      std::string newValue = "f5ea6cbe8cddc3f73bc40e156ced5ef0f80d75bd6794ba18a457c46edaeee6a4";
      REQUIRE(db.put(key, newValue, pfx));
      REQUIRE(db.has(key, pfx));
      REQUIRE(db.get(key, pfx) == newValue);

      // Delete
      REQUIRE(db.del(key, pfx));
      REQUIRE(!db.has(key, pfx));

      // Close
      REQUIRE(db.close());
    }


    SECTION("Batched CRUD (Create + Read + Update + Delete)") {
      // Open
      DB db("testDB");
      std::string pfx = "0001";
      std::vector<DBEntry> putB;
      std::vector<std::string> delB;
      for (int i = 0; i < 32; i++) {
        putB.emplace_back(DBEntry(Hash::random().get(), Hash::random().get()));
        delB.emplace_back(putB[i].key);
      }

      // Create
      DBBatch batchP;
      batchP.puts = putB;
      REQUIRE(db.putBatch(batchP, pfx));
      for (DBEntry entry : batchP.puts) REQUIRE(db.has(entry.key, pfx));

      // Read
      std::vector<DBEntry> getB = db.getBatch(pfx);
      REQUIRE(!getB.empty());
      for (DBEntry getE : getB) {
        for (DBEntry putE : putB) {
          if (getE.key == putE.key) {
            REQUIRE(getE.value == putE.value);
          }
        }
      }

      // Update
      DBBatch newPutB;
      for (int i = 0; i < 32; i++) {
        newPutB.puts.emplace_back(DBEntry(putB[i].key, Hash::random().get()));
      }
      REQUIRE(db.putBatch(newPutB, pfx));
      for (DBEntry entry : newPutB.puts) REQUIRE(db.has(entry.key, pfx));
      std::vector<DBEntry> newGetB = db.getBatch(pfx);
      REQUIRE(!newGetB.empty());
      for (DBEntry newGetE : newGetB) {
        for (DBEntry newPutE : newPutB.puts) {
          if (newGetE.key == newPutE.key) {
            REQUIRE(newGetE.value == newPutE.value);
          }
        }
      }

      // Delete
      DBBatch batchD;
      batchD.dels = delB;
      REQUIRE(db.putBatch(batchD, pfx));
      for (std::string key : batchD.dels) REQUIRE(!db.has(key, pfx));

      // Close
      REQUIRE(db.close());
    }

    // Clean up last test so DB creation can be properly tested next time
    std::filesystem::remove_all(std::filesystem::current_path().string() + "/testDB");
  }
}

