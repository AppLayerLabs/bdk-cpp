/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/db.h"
#include "../../src/utils/strings.h"

#include <filesystem>
#include <string>

using Catch::Matchers::Equals;

namespace TDB {
  TEST_CASE("DB Tests", "[utils][db]") {
    SECTION("Open and Close DB (w/ throw)") {
      bool catched = false;
      DB db("testDB");
      REQUIRE(std::filesystem::exists(std::filesystem::current_path().string() + "/testDB"));
      try { DB db2("testDB"); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
      REQUIRE(db.close());
    }

    SECTION("Simple CRUD (Create + Read + Update + Delete)") {
      // Open
      DB db("testDB");
      std::string key = "d41472b71899ccc0cf16c09ac97af95e";
      std::string value = "5ea04e91c96033ae312af0bb22ec3e370c7789dc28858ea0135966ee2966a616";
      Bytes pfx{0x00, 0x01};

      // Create
      REQUIRE(db.put(key, value, pfx));
      REQUIRE(db.has(key, pfx));

      // Read
      REQUIRE(Utils::bytesToString(db.get(key, pfx)) == value);

      // Update
      std::string newValue = "f5ea6cbe8cddc3f73bc40e156ced5ef0f80d75bd6794ba18a457c46edaeee6a4";
      REQUIRE(db.put(key, newValue, pfx));
      REQUIRE(db.has(key, pfx));
      REQUIRE(Utils::bytesToString(db.get(key, pfx)) == newValue);

      // Delete
      REQUIRE(db.del(key, pfx));
      REQUIRE(!db.has(key, pfx));

      // Close
      REQUIRE(db.close());
    }

    SECTION("Batched CRUD (Create + Read + Update + Delete)") {
      // Open
      DB db("testDB");
      Bytes pfx = DBPrefix::blocks;
      DBBatch batchP;
      DBBatch batchD;
      std::vector<Bytes> keys;
      for (int i = 0; i < 32; i++) {
        batchP.push_back(Hash::random().asBytes(), Hash::random().asBytes(), pfx);
        batchD.delete_key(batchP.getPuts()[i].key, pfx);
      }

      // Create
      std::cout << "BatchPuts: " << batchP.getPuts().size() << std::endl;

      REQUIRE(db.putBatch(batchP));
      for (const DBEntry& entry : batchP.getPuts()) {
        /// No need to pass prefix as entry.key already contains it
        REQUIRE(db.has(entry.key));
      }

      // Read
      std::vector<DBEntry> getB = db.getBatch(pfx);
      REQUIRE(!getB.empty());
      for (const DBEntry& getE : getB) {
        for (const DBEntry& putE : batchP.getPuts()) {
          if (getE.key == putE.key) {
            REQUIRE(getE.value == putE.value);
          }
        }
      }

      // Update
      DBBatch newPutB;
      for (int i = 0; i < 32; i++) {
        newPutB.push_back(batchP.getPuts()[i].key, Hash::random().asBytes(), pfx);
      }
      REQUIRE(db.putBatch(newPutB));
      /// No need to pass prefix as entry.key already contains it
      for (const DBEntry& entry : newPutB.getPuts()) REQUIRE(db.has(entry.key));
      std::vector<DBEntry> newGetB = db.getBatch(pfx);

      REQUIRE(!newGetB.empty());
      for (const DBEntry& newGetE : newGetB) {
        for (const DBEntry& newPutE : newPutB.getPuts()) {
          if (newGetE.key == newPutE.key) {
            REQUIRE(newGetE.value == newPutE.value);
          }
        }
      }

      // Delete
      REQUIRE(db.putBatch(batchD));
      /// No need to pass prefix as key already contains it
      for (const Bytes& key : batchD.getDels()) REQUIRE(!db.has(key));

      // Close
      REQUIRE(db.close());
    }

    SECTION("Throws/Errors") {
      DB db("testDB");
      REQUIRE(!db.has(Utils::stringToBytes("dummy")));
      REQUIRE(db.get(Utils::stringToBytes("dummy")).empty());
      REQUIRE(db.getBatch(Utils::stringToBytes("0001"), {Utils::stringToBytes(("dummy"))}).empty());
      REQUIRE(db.close());
    }

    // Clean up last test so DB creation can be properly tested next time
    std::filesystem::remove_all(std::filesystem::current_path().string() + "/testDB");
  }
}

