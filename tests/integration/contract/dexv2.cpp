/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "libs/catch2/catch_amalgamated.hpp"

#include "contract/contractmanager.h"
#include "contract/templates/dexv2/dexv2pair.h"
#include "contract/templates/dexv2/dexv2factory.h"
#include "contract/templates/dexv2/dexv2router02.h"
#include "contract/templates/nativewrapper.h"
#include "contract/abi.h"

#include "utils/db.h"
#include "utils/options.h"

#include "core/state.h"

#include "../../sdktestsuite.hpp"

#include <utility>

  /*
   * // SPDX-License-Identifier: MIT
   * pragma solidity 0.8.30;
   * import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
   * contract NativeWrapper is ERC20 {
   *   constructor() ERC20("NativeWrapper", "NWP") {
   *   }
   *   function deposit() payable external {
   *     _mint(msg.sender, msg.value);
   *   }
   *   function withdraw(uint256 value) external {
   *     require(balanceOf(msg.sender) >= value);
   *     _burn(msg.sender, value);
   *     payable(msg.sender).transfer(value);
   *   }
   * }
   */

  Bytes nativeWrapperBytecode = Hex::toBytes("0x608060405234801561000f575f5ffd5b506040518060400160405280600d81526020017f4e617469766557726170706572000000000000000000000000000000000000008152506040518060400160405280600381526020017f4e57500000000000000000000000000000000000000000000000000000000000815250816003908161008b91906102e0565b50806004908161009b91906102e0565b5050506103af565b5f81519050919050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52604160045260245ffd5b7f4e487b71000000000000000000000000000000000000000000000000000000005f52602260045260245ffd5b5f600282049050600182168061011e57607f821691505b602082108103610131576101306100da565b5b50919050565b5f819050815f5260205f209050919050565b5f6020601f8301049050919050565b5f82821b905092915050565b5f600883026101937fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff82610158565b61019d8683610158565b95508019841693508086168417925050509392505050565b5f819050919050565b5f819050919050565b5f6101e16101dc6101d7846101b5565b6101be565b6101b5565b9050919050565b5f819050919050565b6101fa836101c7565b61020e610206826101e8565b848454610164565b825550505050565b5f5f905090565b610225610216565b6102308184846101f1565b505050565b5b81811015610253576102485f8261021d565b600181019050610236565b5050565b601f8211156102985761026981610137565b61027284610149565b81016020851015610281578190505b61029561028d85610149565b830182610235565b50505b505050565b5f82821c905092915050565b5f6102b85f198460080261029d565b1980831691505092915050565b5f6102d083836102a9565b9150826002028217905092915050565b6102e9826100a3565b67ffffffffffffffff811115610302576103016100ad565b5b61030c8254610107565b610317828285610257565b5f60209050601f831160018114610348575f8415610336578287015190505b61034085826102c5565b8655506103a7565b601f19841661035686610137565b5f5b8281101561037d57848901518255600182019150602085019450602081019050610358565b8683101561039a5784890151610396601f8916826102a9565b8355505b6001600288020188555050505b505050505050565b611023806103bc5f395ff3fe60806040526004361061009b575f3560e01c8063313ce56711610063578063313ce5671461019357806370a08231146101bd57806395d89b41146101f9578063a9059cbb14610223578063d0e30db01461025f578063dd62ed3e146102695761009b565b806306fdde031461009f578063095ea7b3146100c957806318160ddd1461010557806323b872dd1461012f5780632e1a7d4d1461016b575b5f5ffd5b3480156100aa575f5ffd5b506100b36102a5565b6040516100c09190610c71565b60405180910390f35b3480156100d4575f5ffd5b506100ef60048036038101906100ea9190610d22565b610335565b6040516100fc9190610d7a565b60405180910390f35b348015610110575f5ffd5b50610119610357565b6040516101269190610da2565b60405180910390f35b34801561013a575f5ffd5b5061015560048036038101906101509190610dbb565b610360565b6040516101629190610d7a565b60405180910390f35b348015610176575f5ffd5b50610191600480360381019061018c9190610e0b565b61038e565b005b34801561019e575f5ffd5b506101a76103f3565b6040516101b49190610e51565b60405180910390f35b3480156101c8575f5ffd5b506101e360048036038101906101de9190610e6a565b6103fb565b6040516101f09190610da2565b60405180910390f35b348015610204575f5ffd5b5061020d610440565b60405161021a9190610c71565b60405180910390f35b34801561022e575f5ffd5b5061024960048036038101906102449190610d22565b6104d0565b6040516102569190610d7a565b60405180910390f35b6102676104f2565b005b348015610274575f5ffd5b5061028f600480360381019061028a9190610e95565b6104fe565b60405161029c9190610da2565b60405180910390f35b6060600380546102b490610f00565b80601f01602080910402602001604051908101604052809291908181526020018280546102e090610f00565b801561032b5780601f106103025761010080835404028352916020019161032b565b820191905f5260205f20905b81548152906001019060200180831161030e57829003601f168201915b5050505050905090565b5f5f61033f610580565b905061034c818585610587565b600191505092915050565b5f600254905090565b5f5f61036a610580565b9050610377858285610599565b61038285858561062b565b60019150509392505050565b80610398336103fb565b10156103a2575f5ffd5b6103ac338261071b565b3373ffffffffffffffffffffffffffffffffffffffff166108fc8290811502906040515f60405180830381858888f193505050501580156103ef573d5f5f3e3d5ffd5b5050565b5f6012905090565b5f5f5f8373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f20549050919050565b60606004805461044f90610f00565b80601f016020809104026020016040519081016040528092919081815260200182805461047b90610f00565b80156104c65780601f1061049d576101008083540402835291602001916104c6565b820191905f5260205f20905b8154815290600101906020018083116104a957829003601f168201915b5050505050905090565b5f5f6104da610580565b90506104e781858561062b565b600191505092915050565b6104fc333461079a565b565b5f60015f8473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f205f8373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f2054905092915050565b5f33905090565b6105948383836001610819565b505050565b5f6105a484846104fe565b90507fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff81146106255781811015610616578281836040517ffb8f41b200000000000000000000000000000000000000000000000000000000815260040161060d93929190610f3f565b60405180910390fd5b61062484848484035f610819565b5b50505050565b5f73ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff160361069b575f6040517f96c6fd1e0000000000000000000000000000000000000000000000000000000081526004016106929190610f74565b60405180910390fd5b5f73ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff160361070b575f6040517fec442f050000000000000000000000000000000000000000000000000000000081526004016107029190610f74565b60405180910390fd5b6107168383836109e8565b505050565b5f73ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff160361078b575f6040517f96c6fd1e0000000000000000000000000000000000000000000000000000000081526004016107829190610f74565b60405180910390fd5b610796825f836109e8565b5050565b5f73ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff160361080a575f6040517fec442f050000000000000000000000000000000000000000000000000000000081526004016108019190610f74565b60405180910390fd5b6108155f83836109e8565b5050565b5f73ffffffffffffffffffffffffffffffffffffffff168473ffffffffffffffffffffffffffffffffffffffff1603610889575f6040517fe602df050000000000000000000000000000000000000000000000000000000081526004016108809190610f74565b60405180910390fd5b5f73ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff16036108f9575f6040517f94280d620000000000000000000000000000000000000000000000000000000081526004016108f09190610f74565b60405180910390fd5b8160015f8673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f205f8573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f208190555080156109e2578273ffffffffffffffffffffffffffffffffffffffff168473ffffffffffffffffffffffffffffffffffffffff167f8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925846040516109d99190610da2565b60405180910390a35b50505050565b5f73ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff1603610a38578060025f828254610a2c9190610fba565b92505081905550610b06565b5f5f5f8573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f2054905081811015610ac1578381836040517fe450d38c000000000000000000000000000000000000000000000000000000008152600401610ab893929190610f3f565b60405180910390fd5b8181035f5f8673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f2081905550505b5f73ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff1603610b4d578060025f8282540392505081905550610b97565b805f5f8473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f205f82825401925050819055505b8173ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef83604051610bf49190610da2565b60405180910390a3505050565b5f81519050919050565b5f82825260208201905092915050565b8281835e5f83830152505050565b5f601f19601f8301169050919050565b5f610c4382610c01565b610c4d8185610c0b565b9350610c5d818560208601610c1b565b610c6681610c29565b840191505092915050565b5f6020820190508181035f830152610c898184610c39565b905092915050565b5f5ffd5b5f73ffffffffffffffffffffffffffffffffffffffff82169050919050565b5f610cbe82610c95565b9050919050565b610cce81610cb4565b8114610cd8575f5ffd5b50565b5f81359050610ce981610cc5565b92915050565b5f819050919050565b610d0181610cef565b8114610d0b575f5ffd5b50565b5f81359050610d1c81610cf8565b92915050565b5f5f60408385031215610d3857610d37610c91565b5b5f610d4585828601610cdb565b9250506020610d5685828601610d0e565b9150509250929050565b5f8115159050919050565b610d7481610d60565b82525050565b5f602082019050610d8d5f830184610d6b565b92915050565b610d9c81610cef565b82525050565b5f602082019050610db55f830184610d93565b92915050565b5f5f5f60608486031215610dd257610dd1610c91565b5b5f610ddf86828701610cdb565b9350506020610df086828701610cdb565b9250506040610e0186828701610d0e565b9150509250925092565b5f60208284031215610e2057610e1f610c91565b5b5f610e2d84828501610d0e565b91505092915050565b5f60ff82169050919050565b610e4b81610e36565b82525050565b5f602082019050610e645f830184610e42565b92915050565b5f60208284031215610e7f57610e7e610c91565b5b5f610e8c84828501610cdb565b91505092915050565b5f5f60408385031215610eab57610eaa610c91565b5b5f610eb885828601610cdb565b9250506020610ec985828601610cdb565b9150509250929050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52602260045260245ffd5b5f6002820490506001821680610f1757607f821691505b602082108103610f2a57610f29610ed3565b5b50919050565b610f3981610cb4565b82525050565b5f606082019050610f525f830186610f30565b610f5f6020830185610d93565b610f6c6040830184610d93565b949350505050565b5f602082019050610f875f830184610f30565b92915050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52601160045260245ffd5b5f610fc482610cef565b9150610fcf83610cef565b9250828201905080821115610fe757610fe6610f8d565b5b9291505056fea2646970667358221220ae5050575971f414bc57fc944c5da15054eb9d90ba3bf56831fb500bc81acee464736f6c634300081e0033");

namespace TUQ112x112 {
  TEST_CASE("UQ112x112 Namespace Test", "[integration][contract][dexv2][uq112x112]") {
    SECTION("UQ112x112 Coverage") {
      // Q112 = 5192296858534827628530496329220096
      uint224_t enc = UQ112x112::encode(1024);
      REQUIRE(enc == uint224_t("5316911983139663491615228241121378304"));
      uint224_t div = UQ112x112::uqdiv(uint224_t("123456789000"), uint112_t("1234567890"));
      REQUIRE(div == uint224_t(100));
    }
  }
}

namespace TDEXV2 {
  TEST_CASE("DEXV2 Pair Test", "[integration][contract][dexv2][dexv2pair]") {
    SECTION("Deploy + Dump DEXV2Pair") {
      Address pair;
      Address tokenA;
      Address tokenB;
      Address chainOwner(bytes::hex("0x00dead00665771855a34155f5e7405489df2c3c6"));
      std::unique_ptr<Options> options;
      {
        SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testDEXV2Pair");
        pair = sdk.deployContract<DEXV2Pair>();
        tokenA = sdk.deployContract<ERC20>(
          std::string("TestTokenA"), std::string("TSTA"), uint8_t(18), uint256_t("1000000000000000000")
        );
        tokenB = sdk.deployContract<ERC20>(
          std::string("TestTokenB"), std::string("TSTB"), uint8_t(18), uint256_t("1000000000000000000")
        );
        sdk.callFunction(pair, &DEXV2Pair::initialize, tokenA, tokenB);
        REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::factory) == chainOwner);
        REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::token0) == tokenA);
        REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::token1) == tokenB);
        REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::price0CumulativeLast) == 0);
        REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::price1CumulativeLast) == 0);
        REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::kLast) == 0);
        std::tuple<uint256_t, uint256_t, uint256_t> reservesOutput = sdk.callViewFunction(pair, &DEXV2Pair::getReserves);
        REQUIRE(std::get<0>(reservesOutput) == 0);
        REQUIRE(std::get<1>(reservesOutput) == 0);
        REQUIRE(std::get<2>(reservesOutput) == 0);

        // Dump to database
        options = std::make_unique<Options>(sdk.getOptions());
        sdk.saveSnapshot();
      }

      // SDKTestSuite should automatically load the state from the DB if we construct it with an Options object
      // (The createNewEnvironment DELETES the DB if any is found)
      SDKTestSuite sdk(*options);
      REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::factory) == chainOwner);
      REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::token0) == tokenA);
      REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::token1) == tokenB);
      REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::price0CumulativeLast) == 0);
      REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::price1CumulativeLast) == 0);
      REQUIRE(sdk.callViewFunction(pair, &DEXV2Pair::kLast) == 0);
      std::tuple<uint256_t, uint256_t, uint256_t> reservesOutput = sdk.callViewFunction(pair, &DEXV2Pair::getReserves);
      REQUIRE(std::get<0>(reservesOutput) == 0);
      REQUIRE(std::get<1>(reservesOutput) == 0);
      REQUIRE(std::get<2>(reservesOutput) == 0);
    }
  }

  TEST_CASE("DEXV2 Router Test", "[integration][contract][dexv2][dexv2router]") {
    SECTION("Deploy + Dump DEXV2Router/Factory with a single pair") {
      Address tokenA;
      Address tokenB;
      Address wrapped;
      Address factory;
      Address router;
      Address pair;
      std::unique_ptr<Options> options;
      {
        SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testDEXV2RouterSinglePair");
        tokenA = sdk.deployContract<ERC20>(std::string("TokenA"), std::string("TKNA"), uint8_t(18), uint256_t("10000000000000000000000"));
        tokenB = sdk.deployContract<ERC20>(std::string("TokenB"), std::string("TKNB"), uint8_t(18), uint256_t("10000000000000000000000"));
        wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
        factory = sdk.deployContract<DEXV2Factory>(Address());
        router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
        sdk.callFunction(factory, &DEXV2Factory::createPair, tokenA, tokenB);
        pair = sdk.callViewFunction(factory, &DEXV2Factory::getPairByIndex, uint64_t(0));
        for (const auto& contract : sdk.getState().getCppContracts()) {
          if (contract.first == "TokenA") REQUIRE(contract.second == tokenA);
          if (contract.first == "TokenB") REQUIRE(contract.second == tokenB);
          if (contract.first == "NativeWrapper") REQUIRE(contract.second == wrapped);
          if (contract.first == "DEXV2Factory")  REQUIRE(contract.second == factory);
          if (contract.first == "DEXV2Router02") REQUIRE(contract.second == router);
        }
        // Dump to database
        options = std::make_unique<Options>(sdk.getOptions());
        sdk.saveSnapshot();
      }

      // SDKTestSuite should automatically load the state from the DB if we construct it with an Options object
      // (The createNewEnvironment DELETES the DB if any is found)
      SDKTestSuite sdk(*options);
      for (const auto& contract : sdk.getState().getCppContracts()) {
        if (contract.first == "TokenA") REQUIRE(contract.second == tokenA);
        if (contract.first == "TokenB") REQUIRE(contract.second == tokenB);
        if (contract.first == "NativeWrapper") REQUIRE(contract.second == wrapped);
        if (contract.first == "DEXV2Factory")  REQUIRE(contract.second == factory);
        if (contract.first == "DEXV2Router02") REQUIRE(contract.second == router);
      }

      // For coverage
      REQUIRE(sdk.callViewFunction(router, &DEXV2Router02::factory) == factory);
      REQUIRE(sdk.callViewFunction(router, &DEXV2Router02::wrappedNative) == wrapped);
      REQUIRE(sdk.callViewFunction(factory, &DEXV2Factory::feeTo) == Address());
      REQUIRE(sdk.callViewFunction(factory, &DEXV2Factory::feeToSetter) == Address());
      REQUIRE(sdk.callViewFunction(factory, &DEXV2Factory::allPairsLength) == 1);
      std::vector<Address> allPairs = sdk.callViewFunction(factory, &DEXV2Factory::allPairs);
      REQUIRE(allPairs.size() == 1);
      REQUIRE(allPairs[0] == pair);
      Address add(bytes::hex("0x1234567890123456789012345678901234567890"));
      sdk.callFunction(factory, &DEXV2Factory::setFeeTo, add);
      sdk.callFunction(factory, &DEXV2Factory::setFeeToSetter, add);
      REQUIRE(sdk.callViewFunction(factory, &DEXV2Factory::feeTo) == add);
      REQUIRE(sdk.callViewFunction(factory, &DEXV2Factory::feeToSetter) == add);
      REQUIRE(sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, factory) == Address());

      // For coverage (createPair)
      //REQUIRE_THROWS(sdk.callFunction(factory, &DEXV2Factory::createPair, pair, pair)); // Identical addresses
      //REQUIRE_THROWS(sdk.callFunction(factory, &DEXV2Factory::createPair, Address(), pair)); // Zero address
      //REQUIRE_THROWS(sdk.callFunction(factory, &DEXV2Factory::createPair, tokenA, tokenB)); // Pair exists
    }

    SECTION("Deploy DEXV2 and add/remove liquidity to token/token pair") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testDEXV2RouterLiqTokenTokenPair");
      Address tokenA = sdk.deployContract<ERC20>(std::string("TokenA"), std::string("TKNA"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address tokenB = sdk.deployContract<ERC20>(std::string("TokenB"), std::string("TKNB"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;
      for (const auto& contract : sdk.getState().getCppContracts()) {
        if (contract.first == "NativeWrapper") REQUIRE(contract.second == wrapped);
        if (contract.first == "DEXV2Factory")  REQUIRE(contract.second == factory);
        if (contract.first == "DEXV2Router02") REQUIRE(contract.second == router);
      }

      // Approve "router" so it can spend up to 10000 tokens from both sides
      // on behalf of "owner" (which already has the tokens).
      Hash approveATx = sdk.callFunction(tokenA, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      Hash approveBTx = sdk.callFunction(tokenB, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenA, &ERC20::allowance, owner, router) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenB, &ERC20::allowance, owner, router) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenB, &ERC20::balanceOf, owner) == uint256_t("10000000000000000000000"));

      // Add liquidity of 100 from A and 250 from B
      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;  // 60 seconds
      // tokenA, tokenB, amountADesired, amountBDesired, amountAMin, amountBMin, to, deadline
      Hash addLiquidityTx = sdk.callFunction(router, &DEXV2Router02::addLiquidity,
        tokenA, tokenB, uint256_t("100000000000000000000"), uint256_t("250000000000000000000"),
        uint256_t(0), uint256_t(0), owner, deadline
      );

      auto additionalTxData = sdk.getStorage().getTxAdditionalData(addLiquidityTx);

      // Check if operation worked sucessfully
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, tokenB);
      uint256_t ownerTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner);
      uint256_t ownerTknB = sdk.callViewFunction(tokenB, &ERC20::balanceOf, owner);
      uint256_t pairTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, pair);
      uint256_t pairTknB = sdk.callViewFunction(tokenB, &ERC20::balanceOf, pair);
      REQUIRE(ownerTknA == uint256_t("9900000000000000000000"));
      REQUIRE(ownerTknB == uint256_t("9750000000000000000000"));
      REQUIRE(pairTknA == uint256_t("100000000000000000000"));
      REQUIRE(pairTknB == uint256_t("250000000000000000000"));

      // Approve "pair" so it can allow up to 10000 liquidity tokens to be
      // withdrawn by the "owner" (which has much less than that)
      Hash approvePairTx = sdk.callFunction(pair, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(pair, &ERC20::allowance, owner, router) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(pair, &ERC20::balanceOf, owner) == uint256_t("158113883008418965599"));

      // Remove 50 liquidity tokens from the pair
      deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;  // 60 seconds
      // tokenA, tokenB, liquidity, amountAMin, amountBMin, to, deadline
      Hash removeLiquidityTx = sdk.callFunction(router, &DEXV2Router02::removeLiquidity,
        tokenA, tokenB, uint256_t("50000000000000000000"), uint256_t(0), uint256_t(0), owner, deadline
      );

      // Check if operation worked successfully
      pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, tokenB);
      ownerTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner);
      ownerTknB = sdk.callViewFunction(tokenB, &ERC20::balanceOf, owner);
      pairTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, pair);
      pairTknB = sdk.callViewFunction(tokenB, &ERC20::balanceOf, pair);
      REQUIRE(ownerTknA == uint256_t("9931622776601683793320"));
      REQUIRE(ownerTknB == uint256_t("9829056941504209483300"));
      REQUIRE(pairTknA == uint256_t("68377223398316206680"));
      REQUIRE(pairTknB == uint256_t("170943058495790516700"));

      // For coverage (ensure() and throws on removeLiquidity())
      /*REQUIRE_THROWS(sdk.callFunction(router, &DEXV2Router02::removeLiquidity,
        tokenA, tokenB, uint256_t("5000000000000000000"),
        uint256_t(0), uint256_t(0), owner, uint256_t(0) // deadline always expired
      ));
      REQUIRE_THROWS(sdk.callFunction(router, &DEXV2Router02::removeLiquidity,
        tokenA, tokenB, uint256_t("5000000000000000000"),
        uint256_t("500000000000000000000"), uint256_t(0), owner, deadline // insufficient amountA (500)
      ));
      REQUIRE_THROWS(sdk.callFunction(router, &DEXV2Router02::removeLiquidity,
        tokenA, tokenB, uint256_t("5000000000000000000"),
        uint256_t(0), uint256_t("500000000000000000000"), owner, deadline // insufficient amountB (500)
      ));*/
      // For coverage (sync and skim)
      sdk.callFunction(pair, &DEXV2Pair::sync);
      sdk.callFunction(pair, &DEXV2Pair::skim, owner);
    }

    SECTION("Deploy DEXV2 and add/remove liquidity to token/native pair") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testDEXV2RouterLiqTokenNativePair");
      Address tokenA = sdk.deployContract<ERC20>(std::string("TokenA"), std::string("TKNA"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;
      for (const auto& contract : sdk.getState().getCppContracts()) {
        if (contract.first == "NativeWrapper") REQUIRE(contract.second == wrapped);
        if (contract.first == "DEXV2Factory")  REQUIRE(contract.second == factory);
        if (contract.first == "DEXV2Router02") REQUIRE(contract.second == router);
      }

      // Approve "router" so it can spend up to 10000 TKNA on behalf of "owner"
      Hash approveATx = sdk.callFunction(tokenA, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenA, &ERC20::allowance, owner, router) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner) == uint256_t("10000000000000000000000"));

      uint256_t ownerNativeBeforeAddLiq = sdk.getNativeBalance(owner);
      // Add liquidity of 100 WSPARQ and 100 TKNA
      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;  // 60 seconds
      // token, amountTokenDesired, amountTokenMin, amountNativeMin, to, deadline
      Hash addLiquidityTx = sdk.callFunction(
        router, uint256_t("100000000000000000000"), &DEXV2Router02::addLiquidityNative,
        tokenA, uint256_t("100000000000000000000"), uint256_t("100000000000000000000"),
        uint256_t("100000000000000000000"), owner, deadline
      );

      // Check if operation worked successfully
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, wrapped);
      uint256_t ownerTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner);
      uint256_t ownerNative = sdk.getNativeBalance(owner);
      uint256_t pairTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, pair);
      uint256_t wrappedNative = sdk.getNativeBalance(wrapped);
      uint256_t pairNativeWrapped = sdk.callViewFunction(wrapped, &ERC20::balanceOf, pair);
      REQUIRE(ownerTknA == uint256_t("9900000000000000000000"));
      REQUIRE(ownerNative <= ownerNativeBeforeAddLiq - uint256_t("100000000000000000000") - (uint256_t(1000000000) * 21000));
      REQUIRE(pairTknA == uint256_t("100000000000000000000"));
      REQUIRE(wrappedNative == uint256_t("100000000000000000000"));
      REQUIRE(pairNativeWrapped == uint256_t("100000000000000000000"));

      // Approve "pair" so it can allow up to 10000 liquidity tokens to be
      // withdrawn by the "owner" (which has much less than that)
      Hash approvePairTx = sdk.callFunction(pair, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(pair, &ERC20::allowance, owner, router) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(pair, &ERC20::balanceOf, owner) == uint256_t("99999999999999999000"));

      uint256_t ownerNativeBeforeSubLiq = sdk.getNativeBalance(owner);
      // Remove 50 liquidity tokens
      deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;  // 60 seconds
      // token, liquidity, amountTokenMin, amountNativeMin, to, deadline
      Hash removeLiquidityTx = sdk.callFunction(
        router, uint256_t("100000000000000000000"), &DEXV2Router02::removeLiquidityNative,
        tokenA, uint256_t("50000000000000000000"), uint256_t("10000000000000000000"),
        uint256_t("10000000000000000000"), owner, deadline
      );

      // Check if operation worked successfully
      pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, wrapped);
      ownerTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner);
      ownerNative = sdk.getNativeBalance(owner);
      pairTknA = sdk.callViewFunction(tokenA, &ERC20::balanceOf, pair);
      wrappedNative = sdk.getNativeBalance(wrapped);
      pairNativeWrapped = sdk.callViewFunction(wrapped, &ERC20::balanceOf, pair);
      REQUIRE(ownerTknA == uint256_t("9950000000000000000000"));
      REQUIRE(ownerNative >= ownerNativeBeforeSubLiq - uint256_t("100000000000000000000") - (uint256_t(1000000000) * 21000));
      REQUIRE(pairTknA == uint256_t("50000000000000000000"));
      REQUIRE(wrappedNative == uint256_t("50000000000000000000"));
      REQUIRE(pairNativeWrapped == uint256_t("50000000000000000000"));
    }
    SECTION("swapExactTokensForTokens") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(std::string("testSwapExactTokensForTokens"));

      Address tokenA = sdk.deployContract<ERC20>(std::string("TokenA"), std::string("TKNA"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address tokenB = sdk.deployContract<ERC20>(std::string("TokenB"), std::string("TKNB"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;

      sdk.callFunction(factory, &DEXV2Factory::createPair, tokenA, tokenB);
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, tokenB);

      sdk.callFunction(tokenA, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      sdk.callFunction(tokenB, &ERC20::approve, router, uint256_t("10000000000000000000000"));

      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;

      sdk.callFunction(router, &DEXV2Router02::addLiquidity,
        tokenA, tokenB, uint256_t("500000000000000000000"), uint256_t("500000000000000000000"), uint256_t(0), uint256_t(0), owner, deadline);

      std::vector<Address> path = {tokenA, tokenB};
      sdk.callFunction(router, &DEXV2Router02::swapExactTokensForTokens,
        uint256_t("1000000000000000000"), uint256_t("0"), path, owner, deadline);
      uint256_t tokenABalance = sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner);
      uint256_t tokenBBalance = sdk.callViewFunction(tokenB, &ERC20::balanceOf, owner);

      REQUIRE(tokenABalance < uint256_t("9500000000000000000000"));
      REQUIRE(tokenBBalance > uint256_t("9500000000000000000000"));
    }

    SECTION("swapTokensForExactTokens") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(std::string("testSwapTokensForExactTokens"));

      Address tokenA = sdk.deployContract<ERC20>(std::string("TokenA"), std::string("TKNA"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address tokenB = sdk.deployContract<ERC20>(std::string("TokenB"), std::string("TKNB"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;

      sdk.callFunction(factory, &DEXV2Factory::createPair, tokenA, tokenB);
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, tokenA, tokenB);

      sdk.callFunction(tokenA, &ERC20::approve, router, uint256_t("10000000000000000000000"));
      sdk.callFunction(tokenB, &ERC20::approve, router, uint256_t("10000000000000000000000"));

      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;
      sdk.callFunction(router, &DEXV2Router02::addLiquidity,
        tokenA, tokenB, uint256_t("500000000000000000000"), uint256_t("500000000000000000000"), uint256_t(0), uint256_t(0), owner, deadline);
      std::vector<Address> path = {tokenA, tokenB};
      sdk.callFunction(router, &DEXV2Router02::swapTokensForExactTokens,
        uint256_t("1000000000000000000"), uint256_t("2000000000000000000"), path, owner, deadline);

      uint256_t tokenABalance = sdk.callViewFunction(tokenA, &ERC20::balanceOf, owner);
      uint256_t tokenBBalance = sdk.callViewFunction(tokenB, &ERC20::balanceOf, owner);

      REQUIRE(tokenABalance < uint256_t("9500000000000000000000"));
      REQUIRE(tokenBBalance > uint256_t("9500000000000000000000"));
    }
    SECTION("swapExactNativeForTokens") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(std::string("testSwapExactNativeForTokens"));

      Address token = sdk.deployContract<ERC20>(std::string("Token"), std::string("TKN"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;

      sdk.callFunction(factory, &DEXV2Factory::createPair, token, wrapped);
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, token, wrapped);

      // Approve token
      sdk.callFunction(token, &ERC20::approve, router, uint256_t("10000000000000000000000"));

      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;

      sdk.callFunction(router, uint256_t("100000000000000000000"), &DEXV2Router02::addLiquidityNative,
        token, uint256_t("500000000000000000000"), uint256_t(0), uint256_t(0), owner, deadline);

      std::vector<Address> path = {wrapped, token};

      sdk.callFunction(router, uint256_t("1000000000000000000"), &DEXV2Router02::swapExactNativeForTokens,
        uint256_t(0), path, owner, deadline);

      uint256_t tokenBalance = sdk.callViewFunction(token, &ERC20::balanceOf, owner);
      REQUIRE(tokenBalance > uint256_t("9500000000000000000000"));
    }

    SECTION("swapTokensForExactNative") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(std::string("testSwapTokensForExactNative"));

      Address token = sdk.deployContract<ERC20>(std::string("Token"), std::string("TKN"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;

      sdk.callFunction(factory, &DEXV2Factory::createPair, token, wrapped);
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, token, wrapped);

      sdk.callFunction(token, &ERC20::approve, router, uint256_t("10000000000000000000000"));

      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;

      sdk.callFunction(router, uint256_t("100000000000000000000"), &DEXV2Router02::addLiquidityNative,
        token, uint256_t("500000000000000000000"), uint256_t(0), uint256_t(0), owner, deadline);

      std::vector<Address> path = {token, wrapped};

      sdk.callFunction(router, &DEXV2Router02::swapTokensForExactNative,
        uint256_t("1000000000000000000"), uint256_t("6000000000000000000"), path, owner, deadline);

      uint256_t nativeBalance = sdk.getNativeBalance(owner);
      REQUIRE(nativeBalance > uint256_t("900000000000000000000"));
    }

    SECTION("swapExactTokensForNative") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(std::string("testSwapExactTokensForNative"));

      Address token = sdk.deployContract<ERC20>(std::string("Token"), std::string("TKN"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployContract<NativeWrapper>(std::string("WSPARQ"), std::string("WSPARQ"), uint8_t(18));
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;

      sdk.callFunction(factory, &DEXV2Factory::createPair, token, wrapped);
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, token, wrapped);

      sdk.callFunction(token, &ERC20::approve, router, uint256_t("10000000000000000000000"));

      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;
      auto balBefore = sdk.getNativeBalance(owner);
      auto addLiqTx = sdk.callFunction(router, uint256_t("100000000000000000000"), &DEXV2Router02::addLiquidityNative,
        token, uint256_t("500000000000000000000"), uint256_t(0), uint256_t(0), owner, deadline);

      auto txInfo = sdk.getTx(addLiqTx);
      auto tx = *txInfo.txBlockPtr;
      auto txExtraData = sdk.getStorage().getTxAdditionalData(addLiqTx);
      uint256_t gasUsed = txExtraData->gasUsed * tx.getMaxFeePerGas();

      // Check user balance
      REQUIRE(sdk.callViewFunction(token, &ERC20::balanceOf, owner) == uint256_t("9500000000000000000000"));
      REQUIRE(sdk.getNativeBalance(owner) == balBefore - gasUsed - uint256_t("100000000000000000000"));

      std::vector<Address> path = {token, wrapped};

      sdk.callFunction(router, &DEXV2Router02::swapExactTokensForNative,
        uint256_t("1000000000000000000"), uint256_t(0), path, owner, deadline);

      uint256_t nativeBalance = sdk.getNativeBalance(owner);
      REQUIRE(nativeBalance > uint256_t("900000000000000000000"));
    }

    SECTION("swapExactTokensForNative with EVM Native Wrapper") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(std::string("testSwapExactTokensForNative"));

      Address token = sdk.deployContract<ERC20>(std::string("Token"), std::string("TKN"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployBytecode(nativeWrapperBytecode);
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;

      sdk.callFunction(factory, &DEXV2Factory::createPair, token, wrapped);
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, token, wrapped);

      sdk.callFunction(token, &ERC20::approve, router, uint256_t("10000000000000000000000"));

      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;
      auto balBefore = sdk.getNativeBalance(owner);
      auto addLiqTx = sdk.callFunction(router, uint256_t("100000000000000000000"), &DEXV2Router02::addLiquidityNative,
        token, uint256_t("500000000000000000000"), uint256_t(0), uint256_t(0), owner, deadline);

      auto txInfo = sdk.getTx(addLiqTx);
      auto tx = *txInfo.txBlockPtr;
      auto txExtraData = sdk.getStorage().getTxAdditionalData(addLiqTx);
      uint256_t gasUsed = txExtraData->gasUsed * tx.getMaxFeePerGas();

      // Check user balance
      REQUIRE(sdk.callViewFunction(token, &ERC20::balanceOf, owner) == uint256_t("9500000000000000000000"));
      REQUIRE(sdk.getNativeBalance(owner) == balBefore - gasUsed - uint256_t("100000000000000000000"));

      std::vector<Address> path = {token, wrapped};

      sdk.callFunction(router, &DEXV2Router02::swapExactTokensForNative,
        uint256_t("1000000000000000000"), uint256_t(0), path, owner, deadline);

      uint256_t nativeBalance = sdk.getNativeBalance(owner);
      REQUIRE(nativeBalance > uint256_t("900000000000000000000"));
    }

    SECTION("swapTokensForExactNative with EVM Native Wrapper") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment(std::string("testSwapExactTokensForNative"));

      Address token = sdk.deployContract<ERC20>(std::string("Token"), std::string("TKN"), uint8_t(18), uint256_t("10000000000000000000000"));
      Address wrapped = sdk.deployBytecode(nativeWrapperBytecode);
      Address factory = sdk.deployContract<DEXV2Factory>(Address());
      Address router = sdk.deployContract<DEXV2Router02>(factory, wrapped);
      Address owner = sdk.getChainOwnerAccount().address;

      sdk.callFunction(factory, &DEXV2Factory::createPair, token, wrapped);
      Address pair = sdk.callViewFunction(factory, &DEXV2Factory::getPair, token, wrapped);

      sdk.callFunction(token, &ERC20::approve, router, uint256_t("10000000000000000000000"));

      uint256_t deadline = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count() + 60000000;
      auto balBefore = sdk.getNativeBalance(owner);
      auto addLiqTx = sdk.callFunction(router, uint256_t("100000000000000000000"), &DEXV2Router02::addLiquidityNative,
        token, uint256_t("500000000000000000000"), uint256_t(0), uint256_t(0), owner, deadline);

      auto txInfo = sdk.getTx(addLiqTx);
      auto tx = *txInfo.txBlockPtr;
      auto txExtraData = sdk.getStorage().getTxAdditionalData(addLiqTx);
      uint256_t gasUsed = txExtraData->gasUsed * tx.getMaxFeePerGas();

      // Check user balance
      REQUIRE(sdk.callViewFunction(token, &ERC20::balanceOf, owner) == uint256_t("9500000000000000000000"));
      REQUIRE(sdk.getNativeBalance(owner) == balBefore - gasUsed - uint256_t("100000000000000000000"));

      std::vector<Address> path = {token, wrapped};

      sdk.callFunction(router, &DEXV2Router02::swapTokensForExactNative,
        uint256_t("1000000000000000000"), uint256_t("6000000000000000000"), path, owner, deadline);

      uint256_t nativeBalance = sdk.getNativeBalance(owner);
      REQUIRE(nativeBalance > uint256_t("900000000000000000000"));
    }
  }
}

