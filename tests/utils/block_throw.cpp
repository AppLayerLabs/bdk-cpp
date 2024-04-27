/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/tx.h"
#include "../../src/utils/mutableblock.h"

#include "../../src/utils/ecdsa.h"

using Catch::Matchers::Equals;

namespace TBlock {
  TEST_CASE("Block Class (Throw)", "[utils][block][throw]") {
    SECTION("Block with invalid size") {
      bool catched = false;
      Bytes bytes(Hex::toBytes(
        "0x9890a27da5231bd842529fa107a6e137e807fb8086f6c740d39a37681e1394317e2b38f540f3a9ed7f0b4f6835fc67613dcb52d2e8b3afa193840441902cc030f2febfaa0a1edd774318d1fe6e3bf1aec16082457f7a66f7fd4bef8ddded9b76d7b9da8a2d15d02eae1743ddcfb9e34fe0374ceaec6e96fb8489d16c6886441697610af9744109384ae774b20eb22cce3677a4c836f57ca30eafc308af2d04cf93ada88ad0fb6968ce6ea1556cc24af1234b8b2d93a0e37a417f53148662659ccdbaa2ed5233d712a2ea93ea0a08e360c72018fa10a8d7"
      ));
      REQUIRE(bytes.size() < 217);
      try { MutableBlock b(bytes, 8080); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }

    SECTION("Block with invalid Validator tx height") {
      PrivKey blockValidatorPrivKey(Hex::toBytes("0x77ec0f8f28012de474dcd0b0a2317df22e188cec0a4cb0c9b760c845a23c9699"));
      PrivKey txValidatorPrivKey(Hex::toBytes("53f3b164248c7aa5fe610208c0f785063e398fcb329a32ab4fbc9bd4d29b42db"));
      Hash nPrevBlockHash(Hex::toBytes("0x7c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e64b"));
      // Invalid height data
      std::string byteData = 
          "0x08a729ad45bf86d7760cd9a9556e4893037b45c266715af66da65bb232aaf2ff"
          "130195933cc4c936bab3d0236d5030f5e044fa3bff3f05cd400bde00b5cb7954"
          "007c9efc59d7bec8e79499a49915e0a655a3fff1d0609644d98791893afc67e6"
          "4bc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a4"
          "70c5ed82dcce5c8261bab3a34490d3dea3951b72d13c5a7f7ba4e5bc51630936"
          "de9a7b98506c454820a1bc5b46b70a5489c09808ad3dbc927835a40b83791f1a"
          "1b0005f68de069ea1e0000000013c49ffb00000000000001540000007702f874"
          "821f9080849502f900849502f900825208942e951aa58c8b9b504a97f597bbb2"
          "765c011a8802880de0b6b3a764000080c001a0f56fe87778b4420d3b0f8eba91"
          "d28093abfdbea281a188b8516dd8411dc223d7a05c2d2d71ad3473571ff63790"
          "7d72e6ac399fe4804641dbd9e2d863586c57717d00000071f86fa4cfffe7464f"
          "5aeeb0c4b8207f79fa76eaf72f035bf1fc5fddd31ff47c5d98848bc8bc860084"
          "13c49ffc823f43a0fa74fcf0786ce73e4bf09ad4bab7694277c482b3f333f6c5"
          "f7180384e502543fa04e8d5647638cc88e6a17c891d541c6dcf818da6928f870"
          "1a3ebf971378b14aab";

      Bytes bytes = Hex::toBytes(byteData);

      bool catched = false;
      try { MutableBlock b(bytes, 8080); } catch (std::exception& e) { catched = true; }
      REQUIRE(catched == true);
    }
  

    SECTION("Try to append a block transaction to a already deserialized block") {
      PrivKey validatorPrivKey(Hex::toBytes("0x4d5db4107d237df6a3d58ee5f70ae63d73d765d8a1214214d8a13340d0f2750d"));
      Hash nPrevBlockHash(Hex::toBytes("22143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0f6"));

      auto dataBytes = 
        "0x18395ff0c8ee38a250b9e7aeb5733c437fed8d6ca2135fa634367bb288a3830a"
        "3c624e33401a1798ce09f049fb6507adc52b085d0a83dacc43adfa519c1228e7"
        "0122143e16db549af9ccfd3b746ea4a74421847fa0fe7e0e278626a4e7307ac0"
        "f600000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "000000186c872a48300000000057de95400000000000000d9";

      Bytes bytes = Hex::toBytes(dataBytes);

      TxBlock txB(Hex::toBytes("0x02f874821f9080849502f900849502f900825208942e951aa58c8b9b504a97f597bbb2765c011a8802880de0b6b3a764000080c001a0f56fe87778b4420d3b0f8eba91d28093abfdbea281a188b8516dd8411dc223d7a05c2d2d71ad3473571ff637907d72e6ac399fe4804641dbd9e2d863586c57717d"), 8080);
      TxValidator txV(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"), 8080);

      MutableBlock newBlock(bytes, 8080);
      REQUIRE(!newBlock.appendTx(txB));
      REQUIRE(!newBlock.appendTxValidator(txV));
    }
  }
}

