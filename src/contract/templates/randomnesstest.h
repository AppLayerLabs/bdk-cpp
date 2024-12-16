/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef RANDOMNESSTEST_H
#define RANDOMNESSTEST_H

#include "../dynamiccontract.h"
#include "../../utils/utils.h"

/**
 * RandomnessTest is a simple contract that tests the randomness capabilities of BDK.
 * It is used to test the the RandomGen class and the randomness of the BDK.
 * The contract is equivalent to the following solidity contract:
 *  // SPDX-License-Identifier: MIT
 *  pragma solidity ^0.8.17;
 *  interface BDKPrecompile {
 *      function getRandom() external view returns (uint256);
 *  }
 *
 *  contract RandomnessTest {
 *      uint256 private randomValue_;
 *
 *      function setRandom() external {
 *          randomValue_ = BDKPrecompile(0x1000000000000000000000000000100000000001).getRandom();
 *      }
 *
 *      function getRandom() view external returns (uint256) {
 *          return randomValue_;
 *      }
 *  }
 */
class RandomnessTest : public DynamicContract {
  private:
    SafeUint256_t randomValue_; ///< The random value.
    void registerContractFunctions() override; ///< Register the contract functions.

  public:
    using ConstructorArguments = std::tuple<>; ///< The constructor arguments type.

    /**
    * Constructor from create. Create contract and save it to database.
    * @param address The address of the contract.
    * @param creator The address of the creator of the contract.
    * @param chainId The chain ID.
    */
    RandomnessTest(const Address& address,
      const Address& creator, const uint64_t& chainId
    );

    /**
    * Constructor from load. Load contract from database.
    * @param address The address of the contract.
    * @param db The database to use.
    */
//    RandomnessTest(const Address& address, const DB& db);

    ~RandomnessTest() override; ///< Destructor.

    uint256_t setRandom(); ///< Set the random value.

    uint256_t getRandom() const; ///< Get the random value.

    /**
    * Register the contract structure.
    */
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        RandomnessTest, const Address&, const Address&, const uint64_t&//, DB&
      >(
        std::vector<std::string>{},
        std::make_tuple("setRandom", &RandomnessTest::setRandom, FunctionTypes::NonPayable, std::vector<std::string>{}),
        std::make_tuple("getRandom", &RandomnessTest::getRandom, FunctionTypes::View, std::vector<std::string>{})
      );
    }

    /// Dump method
    //DBBatch dump() const override;
};

#endif  // THROWTESTB_H
