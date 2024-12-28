/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"

#include "../../src/contract/templates/randomnesstest.h"

#include "../sdktestsuite.hpp"

/**
 * Randomness Solidity Contract:
 * // SPDX-License-Identifier: MIT
 * pragma solidity ^0.8.17;
 *
 * interface BDKPrecompile {
 *     function getRandom() external view returns (uint256);
 * }
 *
 * contract RandomnessTest {
 *     uint256 private randomValue_;
 *
 *     function setRandom() external {
 *         randomValue_ = BDKPrecompile(0x1000000000000000000000000000100000000001).getRandom();
 *     }
 *
 *     function getRandom() view external returns (uint256) {
 *         return randomValue_;
 *     }
 * }
 */
namespace TContractRandomness {
  TEST_CASE("Contract Randomness", "[contract][randomness]") {
    SECTION("CPP Randomness Test") {
      auto sdk = SDKTestSuite::createNewEnvironment("CPPContractRandomness");
      auto randomnessContractAddr = sdk.deployContract<RandomnessTest>();
      REQUIRE(sdk.callViewFunction(randomnessContractAddr, &RandomnessTest::getRandom) == 0);

      // The random value should be the first RandomGen operator 0
      // with the seed being Hash(blockRandomness + txIndex)
      // TxIndex is 0, so the seed should be the blockRandomness
      auto setRandomTx = sdk.callFunction(randomnessContractAddr, &RandomnessTest::setRandom);
      Hash randomnessSeed = sdk.getStorage().latest()->getBlockRandomness();
      RandomGen randomGen(randomnessSeed);
      REQUIRE(sdk.callViewFunction(randomnessContractAddr, &RandomnessTest::getRandom) == randomGen.operator()());
    }

    SECTION("EVM Randomness Test") {
      auto randomnessBytecode = Hex::toBytes("6080604052348015600e575f80fd5b506101b08061001c5f395ff3fe608060405234801561000f575f80fd5b5060043610610034575f3560e01c806353e7209e14610038578063aacc5a1714610042575b5f80fd5b610040610060565b005b61004a6100e8565b6040516100579190610108565b60405180910390f35b73100000000000000000000000000010000000000173ffffffffffffffffffffffffffffffffffffffff1663aacc5a176040518163ffffffff1660e01b8152600401602060405180830381865afa1580156100bd573d5f803e3d5ffd5b505050506040513d601f19601f820116820180604052508101906100e1919061014f565b5f81905550565b5f8054905090565b5f819050919050565b610102816100f0565b82525050565b5f60208201905061011b5f8301846100f9565b92915050565b5f80fd5b61012e816100f0565b8114610138575f80fd5b50565b5f8151905061014981610125565b92915050565b5f6020828403121561016457610163610121565b5b5f6101718482850161013b565b9150509291505056fea26469706673582212206ffd6a41e2097987a251d467fce209c21bde13b5a81c4123a0b5a0aa7f62153b64736f6c63430008190033");
      auto sdk = SDKTestSuite::createNewEnvironment("EVMContractRandomness");
      auto randomnessContractAddr = sdk.deployBytecode(randomnessBytecode);
      REQUIRE(sdk.callViewFunction(randomnessContractAddr, &RandomnessTest::getRandom) == 0);

      // The random value should be the first RandomGen operator 0
      // with the seed being Hash(blockRandomness + txIndex)
      // TxIndex is 0, so the seed should be the blockRandomness
      auto setRandomTx = sdk.callFunction(randomnessContractAddr, &RandomnessTest::setRandom);
      Hash randomnessSeed = sdk.getStorage().latest()->getBlockRandomness();
      RandomGen randomGen(randomnessSeed);
      REQUIRE(sdk.callViewFunction(randomnessContractAddr, &RandomnessTest::getRandom) == randomGen.operator()());
    }

    SECTION("Randomness Test DB Dump") {
      Address randomnessContractAddr;
      uint256_t randomNum;
      std::unique_ptr<Options> options;
      Hash randomnessSeed;
      {
        auto sdk = SDKTestSuite::createNewEnvironment("DumpContractRandomness");
        randomnessContractAddr = sdk.deployContract<RandomnessTest>();
        REQUIRE(sdk.callViewFunction(randomnessContractAddr, &RandomnessTest::getRandom) == 0);

        // The random value should be the first RandomGen operator 0
        // with the seed being Hash(blockRandomness + txIndex)
        // TxIndex is 0, so the seed should be the blockRandomness
        auto setRandomTx = sdk.callFunction(randomnessContractAddr, &RandomnessTest::setRandom);
        randomnessSeed = sdk.getStorage().latest()->getBlockRandomness();
        RandomGen randomGen(randomnessSeed);
        randomNum = randomGen.operator()();
        REQUIRE(sdk.callViewFunction(randomnessContractAddr, &RandomnessTest::getRandom) == randomNum);

        // Dump to database
        options = std::make_unique<Options>(sdk.getOptions());
        sdk.getState().saveToDB();
      }

      // SDKTestSuite should automatically load the state from the DB if we construct it with an Options object
      // (The createNewEnvironment DELETES the DB if any is found)
      SDKTestSuite sdk(*options);
      REQUIRE(sdk.callViewFunction(randomnessContractAddr, &RandomnessTest::getRandom) == randomNum);
    }
  }
}

