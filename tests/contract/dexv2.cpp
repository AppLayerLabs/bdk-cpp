#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/contractmanager.h"
#include "../../src/contract/dexv2pair.h"
#include "../../src/contract/dexv2factory.h"
#include "../../src/contract/dexv2router02.h"
#include "../../src/contract/abi.h"
#include "../../src/utils/db.h"
#include "../../src/utils/options.h"
#include "../../src/core/rdpos.h"


/// THIS TEST ONLY INCLUDES TOKEN -> TOKEN CURRENTLY
/// AS CONTRACTMANAGER HAS A NULLPTR TO STATE!!!
PrivKey ownerPrivKey(Hex::toBytes("0xe89ef6409c467285bcae9f80ab1cfeb3487cfe61ab28fb7d36443e1daa0c2867"));

void initialize(std::unique_ptr<Options>& options,
                std::unique_ptr<DB>& db,
                std::unique_ptr<ContractManager> &contractManager,
                const std::string& dbName,
                bool deleteDB = true) {
  if (deleteDB) {
    if (std::filesystem::exists(dbName)) {
      std::filesystem::remove_all(dbName);
    }
  }

  options = std::make_unique<Options>(Options::fromFile(dbName));
  db = std::make_unique<DB>(dbName);
  std::unique_ptr<rdPoS> rdpos;
  contractManager = std::make_unique<ContractManager>(nullptr, db, rdpos, options);

}



namespace TDEXV2 {
  TEST_CASE("DEXV2Pair isolated") {



  }
}