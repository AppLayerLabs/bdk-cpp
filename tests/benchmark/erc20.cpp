/*
  Copyright (c) [2023-2024] [AppLayer Developers]
  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#include "../src/libs/catch2/catch_amalgamated.hpp"

#include "../src/contract/templates/erc20.h"

#include "../src/utils/uintconv.h"

#include "../sdktestsuite.hpp"

#include "../src/bytes/random.h"

namespace TERC20BENCHMARK {
  /*
   *
   * ERC20:
   * Constructor is called with argument "10000000000000000000000"
   *     // SPDX-  License-Identifier: MIT
   *     pragma solidity ^0.8.0;
   *
   *     import "@openzeppelin/contracts/token/ERC20/ERC20.sol";
   *
   *     contract ERC20Test is ERC20 {
   *         constructor(uint256 initialSupply) ERC20("TestToken", "TST") {
   *             _mint(msg.sender, initialSupply);
   *         }
   *     }
   */
  Bytes erc20bytecode = Hex::toBytes("0x608060405234801561000f575f80fd5b506040516115f23803806115f2833981810160405281019061003191906103aa565b6040518060400160405280600981526020017f54657374546f6b656e00000000000000000000000000000000000000000000008152506040518060400160405280600381526020017f545354000000000000000000000000000000000000000000000000000000000081525081600390816100ac9190610606565b5080600490816100bc9190610606565b5050506100cf33826100d560201b60201c565b506107ea565b5f73ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff1603610145575f6040517fec442f0500000000000000000000000000000000000000000000000000000000815260040161013c9190610714565b60405180910390fd5b6101565f838361015a60201b60201c565b5050565b5f73ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff16036101aa578060025f82825461019e919061075a565b92505081905550610278565b5f805f8573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f2054905081811015610233578381836040517fe450d38c00000000000000000000000000000000000000000000000000000000815260040161022a9392919061079c565b60405180910390fd5b8181035f808673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f2081905550505b5f73ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff16036102bf578060025f8282540392505081905550610309565b805f808473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f205f82825401925050819055505b8173ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef8360405161036691906107d1565b60405180910390a3505050565b5f80fd5b5f819050919050565b61038981610377565b8114610393575f80fd5b50565b5f815190506103a481610380565b92915050565b5f602082840312156103bf576103be610373565b5b5f6103cc84828501610396565b91505092915050565b5f81519050919050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52604160045260245ffd5b7f4e487b71000000000000000000000000000000000000000000000000000000005f52602260045260245ffd5b5f600282049050600182168061045057607f821691505b6020821081036104635761046261040c565b5b50919050565b5f819050815f5260205f209050919050565b5f6020601f8301049050919050565b5f82821b905092915050565b5f600883026104c57fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff8261048a565b6104cf868361048a565b95508019841693508086168417925050509392505050565b5f819050919050565b5f61050a61050561050084610377565b6104e7565b610377565b9050919050565b5f819050919050565b610523836104f0565b61053761052f82610511565b848454610496565b825550505050565b5f90565b61054b61053f565b61055681848461051a565b505050565b5b818110156105795761056e5f82610543565b60018101905061055c565b5050565b601f8211156105be5761058f81610469565b6105988461047b565b810160208510156105a7578190505b6105bb6105b38561047b565b83018261055b565b50505b505050565b5f82821c905092915050565b5f6105de5f19846008026105c3565b1980831691505092915050565b5f6105f683836105cf565b9150826002028217905092915050565b61060f826103d5565b67ffffffffffffffff811115610628576106276103df565b5b6106328254610439565b61063d82828561057d565b5f60209050601f83116001811461066e575f841561065c578287015190505b61066685826105eb565b8655506106cd565b601f19841661067c86610469565b5f5b828110156106a35784890151825560018201915060208501945060208101905061067e565b868310156106c057848901516106bc601f8916826105cf565b8355505b6001600288020188555050505b505050505050565b5f73ffffffffffffffffffffffffffffffffffffffff82169050919050565b5f6106fe826106d5565b9050919050565b61070e816106f4565b82525050565b5f6020820190506107275f830184610705565b92915050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52601160045260245ffd5b5f61076482610377565b915061076f83610377565b92508282019050808211156107875761078661072d565b5b92915050565b61079681610377565b82525050565b5f6060820190506107af5f830186610705565b6107bc602083018561078d565b6107c9604083018461078d565b949350505050565b5f6020820190506107e45f83018461078d565b92915050565b610dfb806107f75f395ff3fe608060405234801561000f575f80fd5b5060043610610091575f3560e01c8063313ce56711610064578063313ce5671461013157806370a082311461014f57806395d89b411461017f578063a9059cbb1461019d578063dd62ed3e146101cd57610091565b806306fdde0314610095578063095ea7b3146100b357806318160ddd146100e357806323b872dd14610101575b5f80fd5b61009d6101fd565b6040516100aa9190610a74565b60405180910390f35b6100cd60048036038101906100c89190610b25565b61028d565b6040516100da9190610b7d565b60405180910390f35b6100eb6102af565b6040516100f89190610ba5565b60405180910390f35b61011b60048036038101906101169190610bbe565b6102b8565b6040516101289190610b7d565b60405180910390f35b6101396102e6565b6040516101469190610c29565b60405180910390f35b61016960048036038101906101649190610c42565b6102ee565b6040516101769190610ba5565b60405180910390f35b610187610333565b6040516101949190610a74565b60405180910390f35b6101b760048036038101906101b29190610b25565b6103c3565b6040516101c49190610b7d565b60405180910390f35b6101e760048036038101906101e29190610c6d565b6103e5565b6040516101f49190610ba5565b60405180910390f35b60606003805461020c90610cd8565b80601f016020809104026020016040519081016040528092919081815260200182805461023890610cd8565b80156102835780601f1061025a57610100808354040283529160200191610283565b820191905f5260205f20905b81548152906001019060200180831161026657829003601f168201915b5050505050905090565b5f80610297610467565b90506102a481858561046e565b600191505092915050565b5f600254905090565b5f806102c2610467565b90506102cf858285610480565b6102da858585610512565b60019150509392505050565b5f6012905090565b5f805f8373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f20549050919050565b60606004805461034290610cd8565b80601f016020809104026020016040519081016040528092919081815260200182805461036e90610cd8565b80156103b95780601f10610390576101008083540402835291602001916103b9565b820191905f5260205f20905b81548152906001019060200180831161039c57829003601f168201915b5050505050905090565b5f806103cd610467565b90506103da818585610512565b600191505092915050565b5f60015f8473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f205f8373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f2054905092915050565b5f33905090565b61047b8383836001610602565b505050565b5f61048b84846103e5565b90507fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff811461050c57818110156104fd578281836040517ffb8f41b20000000000000000000000000000000000000000000000000000000081526004016104f493929190610d17565b60405180910390fd5b61050b84848484035f610602565b5b50505050565b5f73ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff1603610582575f6040517f96c6fd1e0000000000000000000000000000000000000000000000000000000081526004016105799190610d4c565b60405180910390fd5b5f73ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff16036105f2575f6040517fec442f050000000000000000000000000000000000000000000000000000000081526004016105e99190610d4c565b60405180910390fd5b6105fd8383836107d1565b505050565b5f73ffffffffffffffffffffffffffffffffffffffff168473ffffffffffffffffffffffffffffffffffffffff1603610672575f6040517fe602df050000000000000000000000000000000000000000000000000000000081526004016106699190610d4c565b60405180910390fd5b5f73ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff16036106e2575f6040517f94280d620000000000000000000000000000000000000000000000000000000081526004016106d99190610d4c565b60405180910390fd5b8160015f8673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f205f8573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f208190555080156107cb578273ffffffffffffffffffffffffffffffffffffffff168473ffffffffffffffffffffffffffffffffffffffff167f8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925846040516107c29190610ba5565b60405180910390a35b50505050565b5f73ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff1603610821578060025f8282546108159190610d92565b925050819055506108ef565b5f805f8573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f20549050818110156108aa578381836040517fe450d38c0000000000000000000000000000000000000000000000000000000081526004016108a193929190610d17565b60405180910390fd5b8181035f808673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f2081905550505b5f73ffffffffffffffffffffffffffffffffffffffff168273ffffffffffffffffffffffffffffffffffffffff1603610936578060025f8282540392505081905550610980565b805f808473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f205f82825401925050819055505b8173ffffffffffffffffffffffffffffffffffffffff168373ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef836040516109dd9190610ba5565b60405180910390a3505050565b5f81519050919050565b5f82825260208201905092915050565b5f5b83811015610a21578082015181840152602081019050610a06565b5f8484015250505050565b5f601f19601f8301169050919050565b5f610a46826109ea565b610a5081856109f4565b9350610a60818560208601610a04565b610a6981610a2c565b840191505092915050565b5f6020820190508181035f830152610a8c8184610a3c565b905092915050565b5f80fd5b5f73ffffffffffffffffffffffffffffffffffffffff82169050919050565b5f610ac182610a98565b9050919050565b610ad181610ab7565b8114610adb575f80fd5b50565b5f81359050610aec81610ac8565b92915050565b5f819050919050565b610b0481610af2565b8114610b0e575f80fd5b50565b5f81359050610b1f81610afb565b92915050565b5f8060408385031215610b3b57610b3a610a94565b5b5f610b4885828601610ade565b9250506020610b5985828601610b11565b9150509250929050565b5f8115159050919050565b610b7781610b63565b82525050565b5f602082019050610b905f830184610b6e565b92915050565b610b9f81610af2565b82525050565b5f602082019050610bb85f830184610b96565b92915050565b5f805f60608486031215610bd557610bd4610a94565b5b5f610be286828701610ade565b9350506020610bf386828701610ade565b9250506040610c0486828701610b11565b9150509250925092565b5f60ff82169050919050565b610c2381610c0e565b82525050565b5f602082019050610c3c5f830184610c1a565b92915050565b5f60208284031215610c5757610c56610a94565b5b5f610c6484828501610ade565b91505092915050565b5f8060408385031215610c8357610c82610a94565b5b5f610c9085828601610ade565b9250506020610ca185828601610ade565b9150509250929050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52602260045260245ffd5b5f6002820490506001821680610cef57607f821691505b602082108103610d0257610d01610cab565b5b50919050565b610d1181610ab7565b82525050565b5f606082019050610d2a5f830186610d08565b610d376020830185610b96565b610d446040830184610b96565b949350505050565b5f602082019050610d5f5f830184610d08565b92915050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52601160045260245ffd5b5f610d9c82610af2565b9150610da783610af2565b9250828201905080821115610dbf57610dbe610d65565b5b9291505056fea264697066735822122043402e069181d2f0057dcffd90d442615a3da729849963e98eada0e05475373164736f6c6343000819003300000000000000000000000000000000000000000000021e19e0c9bab2400000");

  /*

  // SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract MappingManager {
    // Outer mapping index is a uint64 and inner key is an address.
    mapping(uint64 => mapping(address => uint256)) public myMapping;

    // State variable to store the list of addresses.
    address[] public storedAddresses;

    // Stores the number of items (saved during generate) so we can loop in addall.
    uint64 public size;
  function generate(address[] calldata _addresses) external {
    // If we already have a list, check that the new list has the same length.
    if (storedAddresses.length > 0) {
      require(
          _addresses.length == storedAddresses.length,
          "New array must have the same length as the existing one."
      );
    }

    // Clear the stored addresses before (re)populating.
    delete storedAddresses;

    // Loop over the provided addresses.
    for (uint64 i = 0; i < _addresses.length; i++) {
      storedAddresses.push(_addresses[i]);
      // Initialize mapping at index i for address _addresses[i] with 0.
      myMapping[i][_addresses[i]] = 0;
    }

    // Save the size for later use in addall.
    size = uint64(_addresses.length);
  }

  function addall() external {
    // Loop through each stored address and increment its value.
    for (uint64 i = 0; i < size; i++) {
      myMapping[i][storedAddresses[i]] += 1;
    }
  }
}
  */

  Bytes mappingContractBytescode = Hex::toBytes("0x6080604052348015600e575f80fd5b506108a68061001c5f395ff3fe608060405234801561000f575f80fd5b5060043610610055575f3560e01c806351d5cda5146100595780637045ef03146100895780637c0dc417146100935780637d86195c146100af578063949d225d146100df575b5f80fd5b610073600480360381019061006e919061052b565b6100fd565b6040516100809190610581565b60405180910390f35b61009161011c565b005b6100ad60048036038101906100a891906105fb565b61021a565b005b6100c960048036038101906100c49190610670565b6103ff565b6040516100d691906106aa565b60405180910390f35b6100e761043a565b6040516100f491906106d2565b60405180910390f35b5f602052815f5260405f20602052805f5260405f205f91509150505481565b5f5b60025f9054906101000a900467ffffffffffffffff1667ffffffffffffffff168167ffffffffffffffff1610156102175760015f808367ffffffffffffffff1667ffffffffffffffff1681526020019081526020015f205f60018467ffffffffffffffff1681548110610194576101936106eb565b5b905f5260205f20015f9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f205f8282546102039190610745565b92505081905550808060010191505061011e565b50565b5f6001805490501115610272576001805490508282905014610271576040517f08c379a0000000000000000000000000000000000000000000000000000000008152600401610268906107f8565b60405180910390fd5b5b60015f61027f9190610453565b5f5b828290508167ffffffffffffffff1610156103cf57600183838367ffffffffffffffff168181106102b5576102b46106eb565b5b90506020020160208101906102ca9190610816565b908060018154018082558091505060019003905f5260205f20015f9091909190916101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff1602179055505f805f8367ffffffffffffffff1667ffffffffffffffff1681526020019081526020015f205f85858567ffffffffffffffff1681811061036a576103696106eb565b5b905060200201602081019061037f9190610816565b73ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020015f208190555080806103c790610841565b915050610281565b508181905060025f6101000a81548167ffffffffffffffff021916908367ffffffffffffffff1602179055505050565b6001818154811061040e575f80fd5b905f5260205f20015f915054906101000a900473ffffffffffffffffffffffffffffffffffffffff1681565b60025f9054906101000a900467ffffffffffffffff1681565b5080545f8255905f5260205f209081019061046e9190610471565b50565b5b80821115610488575f815f905550600101610472565b5090565b5f80fd5b5f80fd5b5f67ffffffffffffffff82169050919050565b6104b081610494565b81146104ba575f80fd5b50565b5f813590506104cb816104a7565b92915050565b5f73ffffffffffffffffffffffffffffffffffffffff82169050919050565b5f6104fa826104d1565b9050919050565b61050a816104f0565b8114610514575f80fd5b50565b5f8135905061052581610501565b92915050565b5f80604083850312156105415761054061048c565b5b5f61054e858286016104bd565b925050602061055f85828601610517565b9150509250929050565b5f819050919050565b61057b81610569565b82525050565b5f6020820190506105945f830184610572565b92915050565b5f80fd5b5f80fd5b5f80fd5b5f8083601f8401126105bb576105ba61059a565b5b8235905067ffffffffffffffff8111156105d8576105d761059e565b5b6020830191508360208202830111156105f4576105f36105a2565b5b9250929050565b5f80602083850312156106115761061061048c565b5b5f83013567ffffffffffffffff81111561062e5761062d610490565b5b61063a858286016105a6565b92509250509250929050565b61064f81610569565b8114610659575f80fd5b50565b5f8135905061066a81610646565b92915050565b5f602082840312156106855761068461048c565b5b5f6106928482850161065c565b91505092915050565b6106a4816104f0565b82525050565b5f6020820190506106bd5f83018461069b565b92915050565b6106cc81610494565b82525050565b5f6020820190506106e55f8301846106c3565b92915050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52603260045260245ffd5b7f4e487b71000000000000000000000000000000000000000000000000000000005f52601160045260245ffd5b5f61074f82610569565b915061075a83610569565b925082820190508082111561077257610771610718565b5b92915050565b5f82825260208201905092915050565b7f4e6577206172726179206d7573742068617665207468652073616d65206c656e5f8201527f67746820617320746865206578697374696e67206f6e652e0000000000000000602082015250565b5f6107e2603883610778565b91506107ed82610788565b604082019050919050565b5f6020820190508181035f83015261080f816107d6565b9050919050565b5f6020828403121561082b5761082a61048c565b5b5f61083884828501610517565b91505092915050565b5f61084b82610494565b915067ffffffffffffffff820361086557610864610718565b5b60018201905091905056fea2646970667358221220172f0755d4b2ebde17ed924baa885bedd928f43702aff0a70d2eee8a7790cbe864736f6c634300081a0033");

  TEST_CASE("ERC20 Benchmark", "[benchmark][erc20]") {
    SECTION("CPP ERC20 Benchmark") {
      std::unique_ptr<Options> options = nullptr;
      Address to(Utils::randBytes(20));

      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20CppBenchmark", {}, nullptr, IndexingMode::DISABLED);
      // const TestAccount& from, const Address& to, const uint256_t& value, Bytes data = Bytes()
      auto erc20Address = sdk.deployContract<ERC20>(std::string("TestToken"), std::string("TST"), uint8_t(18), uint256_t("10000000000000000000000"));
      // Now for the funny part, we are NOT a C++ contract, but we can
      // definitely take advantage of the templated ABI to interact with it
      // as the encoding is the same

      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::name) == "TestToken");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::symbol) == "TST");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::decimals) == 18);
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::totalSupply) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("10000000000000000000000"));


      // Create the transaction for transfer
      auto functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<Address, uint256_t>("transfer").value);
      Bytes transferEncoded(functor.cbegin(), functor.cend());
      Utils::appendBytes(transferEncoded, ABI::Encoder::encodeData<Address, uint256_t>(to, uint256_t("100")));
      TxBlock transferTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, transferEncoded);
      auto& state = sdk.getState();
      uint64_t iterations = 2500000;

      auto start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        state.call(transferTx);
      }
      auto end = std::chrono::high_resolution_clock::now();

      long double durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      long double microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "CPP ERC20 transfer took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "CPP Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;

      // Check if we actually transferred the tokens.
      uint256_t expectedToBalance = uint256_t(100) * iterations;
      uint256_t transferredToBalance = sdk.callViewFunction(erc20Address, &ERC20::balanceOf, to);
      uint256_t expectedFromBalance = uint256_t("10000000000000000000000") - expectedToBalance;
      uint256_t transferredFromBalance = sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address);
      REQUIRE (expectedToBalance == transferredToBalance);
      REQUIRE (expectedFromBalance == transferredFromBalance);
      // Dump the state
      sdk.getState().saveToDB();
    }
    #ifndef BUILD_TESTNET
    SECTION("CPP ERC20 generate Benchmark") {
      std::unique_ptr<Options> options = nullptr;
      Address to(Utils::randBytes(20));

      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20CppBenchmark", {}, nullptr, IndexingMode::DISABLED);
      // const TestAccount& from, const Address& to, const uint256_t& value, Bytes data = Bytes()
      auto erc20Address = sdk.deployContract<ERC20>(std::string("TestToken"), std::string("TST"), uint8_t(18), uint256_t("10000000000000000000000"));
      // Now for the funny part, we are NOT a C++ contract, but we can
      // definitely take advantage of the templated ABI to interact with it
      // as the encoding is the same

      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::name) == "TestToken");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::symbol) == "TST");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::decimals) == 18);
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::totalSupply) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("10000000000000000000000"));


      std::vector<Address> addresses;
      for (uint64_t i = 0; i < 1000; i++) {
        addresses.push_back(Address(Utils::randBytes(20)));
      }

      // Create the transaction for generate
      auto functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<std::vector<Address>>("generate").value);
      Bytes generateEncoded(functor.cbegin(), functor.cend());
      Utils::appendBytes(generateEncoded, ABI::Encoder::encodeData<std::vector<Address>>(addresses));
      TxBlock transferTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, generateEncoded);
      auto& state = sdk.getState();
      evmc_tx_context txContext;

      txContext.tx_origin = sdk.getChainOwnerAccount().address.toEvmcAddress();
      txContext.tx_gas_price = {};
      txContext.block_coinbase = to.toEvmcAddress();
      txContext.block_number = 1;
      txContext.block_timestamp = 1;
      txContext.block_gas_limit = std::numeric_limits<int64_t>::max();
      txContext.block_prev_randao = {};
      txContext.chain_id = {};
      txContext.block_base_fee = {};
      txContext.blob_base_fee = {};
      txContext.blob_hashes = nullptr;
      txContext.blob_hashes_count = 0;

      auto callInfo = transferTx.txToMessage();
      Hash randomnessHash = bytes::random();
      int64_t leftOverGas = std::numeric_limits<int64_t>::max();
      uint64_t iterations = 2500;

      auto start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        state.call(callInfo, txContext, ContractType::CPP, randomnessHash, randomnessHash, randomnessHash, leftOverGas);
      }
      auto end = std::chrono::high_resolution_clock::now();

      long double durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      long double microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "CPP ERC20 Generate took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "CPP Generate Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;


      // Create the transaction for addall
      functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<>("addall").value);
      Bytes addallEncoded(functor.cbegin(), functor.cend());
      TxBlock addAllTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, addallEncoded);
      evmc_tx_context addAllTxContract;

      addAllTxContract.tx_origin = sdk.getChainOwnerAccount().address.toEvmcAddress();
      addAllTxContract.tx_gas_price = {};
      addAllTxContract.block_coinbase = to.toEvmcAddress();
      addAllTxContract.block_number = 1;
      addAllTxContract.block_timestamp = 1;
      addAllTxContract.block_gas_limit = std::numeric_limits<int64_t>::max();
      addAllTxContract.block_prev_randao = {};
      addAllTxContract.chain_id = {};
      addAllTxContract.block_base_fee = {};
      addAllTxContract.blob_base_fee = {};
      addAllTxContract.blob_hashes = nullptr;
      addAllTxContract.blob_hashes_count = 0;

      callInfo = addAllTx.txToMessage();
      randomnessHash = bytes::random();
      leftOverGas = std::numeric_limits<int64_t>::max();
      iterations = 2500;

      start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        state.call(callInfo, txContext, ContractType::CPP, randomnessHash, randomnessHash, randomnessHash, leftOverGas);
      }
      end = std::chrono::high_resolution_clock::now();

      durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "CPP ERC20 Addall took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "CPP Addall Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;
    }

    SECTION("EVM generate Benchmark") {
      std::unique_ptr<Options> options = nullptr;
      Address to(Utils::randBytes(20));

      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20CppBenchmark", {}, nullptr, IndexingMode::DISABLED);
      // const TestAccount& from, const Address& to, const uint256_t& value, Bytes data = Bytes()
      std::cout << "Deploying MappingManager contract" << std::endl;
      auto erc20Address = sdk.deployBytecode(mappingContractBytescode);
      std::cout << "Deployed MappingManager contract" << std::endl;
      // Now for the funny part, we are NOT a C++ contract, but we can
      // definitely take advantage of the templated ABI to interact with it
      // as the encoding is the same

      std::vector<Address> addresses;
      for (uint64_t i = 0; i < 1000; i++) {
        addresses.push_back(Address(Utils::randBytes(20)));
      }

      // Create the transaction for generate
      auto functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<std::vector<Address>>("generate").value);
      Bytes generateEncoded(functor.cbegin(), functor.cend());
      Utils::appendBytes(generateEncoded, ABI::Encoder::encodeData<std::vector<Address>>(addresses));
      TxBlock transferTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, generateEncoded);
      auto& state = sdk.getState();
      evmc_tx_context txContext;

      txContext.tx_origin = sdk.getChainOwnerAccount().address.toEvmcAddress();
      txContext.tx_gas_price = {};
      txContext.block_coinbase = to.toEvmcAddress();
      txContext.block_number = 1;
      txContext.block_timestamp = 1;
      txContext.block_gas_limit = std::numeric_limits<int64_t>::max();
      txContext.block_prev_randao = {};
      txContext.chain_id = {};
      txContext.block_base_fee = {};
      txContext.blob_base_fee = {};
      txContext.blob_hashes = nullptr;
      txContext.blob_hashes_count = 0;

      auto callInfo = transferTx.txToMessage();
      Hash randomnessHash = bytes::random();
      int64_t leftOverGas = std::numeric_limits<int64_t>::max();
      uint64_t iterations = 2500000;

      auto start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        state.call(callInfo, txContext, ContractType::CPP, randomnessHash, randomnessHash, randomnessHash, leftOverGas);
      }
      auto end = std::chrono::high_resolution_clock::now();

      long double durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      long double microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "CPP ERC20 transferFrom took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "CPP Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;

      // Check if we actually transferred the tokens.
      uint256_t expectedToBalance = uint256_t(100) * iterations;
      uint256_t transferredToBalance = sdk.callViewFunction(erc20Address, &ERC20::balanceOf, to);
      uint256_t expectedFromBalance = uint256_t("10000000000000000000000") - expectedToBalance;
      uint256_t transferredFromBalance = sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address);
      REQUIRE (expectedToBalance == transferredToBalance);
      REQUIRE (expectedFromBalance == transferredFromBalance);
      // Dump the state
      sdk.getState().saveToDB();
    }
    #ifndef BUILD_TESTNET
    SECTION("CPP ERC20 generate Benchmark") {
      std::unique_ptr<Options> options = nullptr;
      Address to(Utils::randBytes(20));

      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20CppBenchmark", {}, nullptr, IndexingMode::DISABLED);
      // const TestAccount& from, const Address& to, const uint256_t& value, Bytes data = Bytes()
      auto erc20Address = sdk.deployContract<ERC20>(std::string("TestToken"), std::string("TST"), uint8_t(18), uint256_t("10000000000000000000000"));
      // Now for the funny part, we are NOT a C++ contract, but we can
      // definitely take advantage of the templated ABI to interact with it
      // as the encoding is the same

      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::name) == "TestToken");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::symbol) == "TST");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::decimals) == 18);
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::totalSupply) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("10000000000000000000000"));


      std::vector<Address> addresses;
      for (uint64_t i = 0; i < 1000; i++) {
        addresses.push_back(Address(Utils::randBytes(20)));
      }

      // Create the transaction for generate
      auto functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<std::vector<Address>>("generate").value);
      Bytes generateEncoded(functor.cbegin(), functor.cend());
      Utils::appendBytes(generateEncoded, ABI::Encoder::encodeData<std::vector<Address>>(addresses));
      TxBlock transferTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, generateEncoded);
      auto& state = sdk.getState();
      evmc_tx_context txContext;

      txContext.tx_origin = sdk.getChainOwnerAccount().address.toEvmcAddress();
      txContext.tx_gas_price = {};
      txContext.block_coinbase = to.toEvmcAddress();
      txContext.block_number = 1;
      txContext.block_timestamp = 1;
      txContext.block_gas_limit = std::numeric_limits<int64_t>::max();
      txContext.block_prev_randao = {};
      txContext.chain_id = {};
      txContext.block_base_fee = {};
      txContext.blob_base_fee = {};
      txContext.blob_hashes = nullptr;
      txContext.blob_hashes_count = 0;

      auto callInfo = transferTx.txToMessage();
      Hash randomnessHash = bytes::random();
      int64_t leftOverGas = std::numeric_limits<int64_t>::max();
      uint64_t iterations = 2500;

      auto start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        state.call(callInfo, txContext, ContractType::CPP, randomnessHash, randomnessHash, randomnessHash, leftOverGas);
      }
      auto end = std::chrono::high_resolution_clock::now();

      long double durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      long double microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "CPP ERC20 Generate took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "CPP Generate Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;


      // Create the transaction for addall
      functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<>("addall").value);
      Bytes addallEncoded(functor.cbegin(), functor.cend());
      TxBlock addAllTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, addallEncoded);
      evmc_tx_context addAllTxContract;

      addAllTxContract.tx_origin = sdk.getChainOwnerAccount().address.toEvmcAddress();
      addAllTxContract.tx_gas_price = {};
      addAllTxContract.block_coinbase = to.toEvmcAddress();
      addAllTxContract.block_number = 1;
      addAllTxContract.block_timestamp = 1;
      addAllTxContract.block_gas_limit = std::numeric_limits<int64_t>::max();
      addAllTxContract.block_prev_randao = {};
      addAllTxContract.chain_id = {};
      addAllTxContract.block_base_fee = {};
      addAllTxContract.blob_base_fee = {};
      addAllTxContract.blob_hashes = nullptr;
      addAllTxContract.blob_hashes_count = 0;

      callInfo = addAllTx.txToMessage();
      randomnessHash = bytes::random();
      leftOverGas = std::numeric_limits<int64_t>::max();
      iterations = 2500;

      start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        state.call(callInfo, txContext, ContractType::CPP, randomnessHash, randomnessHash, randomnessHash, leftOverGas);
      }
      end = std::chrono::high_resolution_clock::now();

      durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "CPP ERC20 Addall took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "CPP Addall Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;
    }

    SECTION("EVM generate Benchmark") {
      std::unique_ptr<Options> options = nullptr;
      Address to(Utils::randBytes(20));

      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20CppBenchmark", {}, nullptr, IndexingMode::DISABLED);
      // const TestAccount& from, const Address& to, const uint256_t& value, Bytes data = Bytes()
      std::cout << "Deploying MappingManager contract" << std::endl;
      auto erc20Address = sdk.deployBytecode(mappingContractBytescode);
      std::cout << "Deployed MappingManager contract" << std::endl;
      // Now for the funny part, we are NOT a C++ contract, but we can
      // definitely take advantage of the templated ABI to interact with it
      // as the encoding is the same

      std::vector<Address> addresses;
      for (uint64_t i = 0; i < 1000; i++) {
        addresses.push_back(Address(Utils::randBytes(20)));
      }

      // Create the transaction for generate
      auto functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<std::vector<Address>>("generate").value);
      Bytes generateEncoded(functor.cbegin(), functor.cend());
      Utils::appendBytes(generateEncoded, ABI::Encoder::encodeData<std::vector<Address>>(addresses));
      TxBlock transferTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, generateEncoded);
      auto& state = sdk.getState();
      evmc_tx_context txContext;

      txContext.tx_origin = sdk.getChainOwnerAccount().address.toEvmcAddress();
      txContext.tx_gas_price = {};
      txContext.block_coinbase = to.toEvmcAddress();
      txContext.block_number = 1;
      txContext.block_timestamp = 1;
      txContext.block_gas_limit = std::numeric_limits<int64_t>::max();
      txContext.block_prev_randao = {};
      txContext.chain_id = {};
      txContext.block_base_fee = {};
      txContext.blob_base_fee = {};
      txContext.blob_hashes = nullptr;
      txContext.blob_hashes_count = 0;

      auto callInfo = transferTx.txToMessage();
      Hash randomnessHash = bytes::random();
      int64_t leftOverGas = std::numeric_limits<int64_t>::max();
      uint64_t iterations = 1000;

      auto start = std::chrono::high_resolution_clock::now();
      std::cout << "Starting EVM Generate Benchmark" << std::endl;
      for (uint64_t i = 0; i < iterations; i++) {
        leftOverGas = std::numeric_limits<int64_t>::max();
        callInfo.gas = std::numeric_limits<int64_t>::max();
        state.call(callInfo, txContext, ContractType::EVM, randomnessHash, randomnessHash, randomnessHash, leftOverGas);
      }
      std::cout << "Finished EVM Generate Benchmark" << std::endl;
      auto end = std::chrono::high_resolution_clock::now();

      long double durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      long double microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "EVM ERC20 Generate took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "EVM Generate Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;


      // Create the transaction for addall
      functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<>("addall").value);
      Bytes addallEncoded(functor.cbegin(), functor.cend());
      TxBlock addAllTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, addallEncoded);
      evmc_tx_context addAllTxContract;

      addAllTxContract.tx_origin = sdk.getChainOwnerAccount().address.toEvmcAddress();
      addAllTxContract.tx_gas_price = {};
      addAllTxContract.block_coinbase = to.toEvmcAddress();
      addAllTxContract.block_number = 1;
      addAllTxContract.block_timestamp = 1;
      addAllTxContract.block_gas_limit = std::numeric_limits<int64_t>::max();
      addAllTxContract.block_prev_randao = {};
      addAllTxContract.chain_id = {};
      addAllTxContract.block_base_fee = {};
      addAllTxContract.blob_base_fee = {};
      addAllTxContract.blob_hashes = nullptr;
      addAllTxContract.blob_hashes_count = 0;

      callInfo = addAllTx.txToMessage();
      randomnessHash = bytes::random();
      leftOverGas = std::numeric_limits<int64_t>::max();
      iterations = 1000;

      start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        leftOverGas = std::numeric_limits<int64_t>::max();
        callInfo.gas = std::numeric_limits<int64_t>::max();
        state.call(callInfo, txContext, ContractType::EVM, randomnessHash, randomnessHash, randomnessHash, leftOverGas);
      }
      end = std::chrono::high_resolution_clock::now();

      durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "EVM ERC20 Addall took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "EVM Addall Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;
    }
    #endif // BUILD_TESTNET
    SECTION("CPP ERC20 transferFrom Benchmark") {
      std::unique_ptr<Options> options = nullptr;
      TestAccount sender = TestAccount::newRandomAccount();
      Address to(Utils::randBytes(20));

      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testERC20CppBenchmark", { sender }, nullptr, IndexingMode::DISABLED);
      // const TestAccount& from, const Address& to, const uint256_t& value, Bytes data = Bytes()
      auto erc20Address = sdk.deployContract<ERC20>(std::string("TestToken"), std::string("TST"), uint8_t(18), uint256_t("10000000000000000000000"));
      // Now for the funny part, we are NOT a C++ contract, but we can
      // definitely take advantage of the templated ABI to interact with it
      // as the encoding is the same

      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::name) == "TestToken");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::symbol) == "TST");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::decimals) == 18);
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::totalSupply) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("10000000000000000000000"));

      sdk.callFunction(erc20Address, &ERC20::approve, sender.address, std::numeric_limits<uint256_t>::max());

      // Create the transaction for transfer
      auto functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<Address, Address, uint256_t>("transferFrom").value);
      Bytes transferEncoded(functor.cbegin(), functor.cend());
      Utils::appendBytes(transferEncoded, ABI::Encoder::encodeData<Address, Address, uint256_t>(sdk.getChainOwnerAccount().address, to, uint256_t("100")));
      TxBlock transferTx = sdk.createNewTx(sender, erc20Address, 0, transferEncoded);
      auto& state = sdk.getState();
      evmc_tx_context txContext;

      txContext.tx_origin = sender.address.toEvmcAddress();
      txContext.tx_gas_price = {};
      txContext.block_coinbase = to.toEvmcAddress();
      txContext.block_number = 1;
      txContext.block_timestamp = 1;
      txContext.block_gas_limit = std::numeric_limits<int64_t>::max();
      txContext.block_prev_randao = {};
      txContext.chain_id = {};
      txContext.block_base_fee = {};
      txContext.blob_base_fee = {};
      txContext.blob_hashes = nullptr;
      txContext.blob_hashes_count = 0;

      auto callInfo = transferTx.txToMessage();
      Hash randomnessHash = bytes::random();
      int64_t leftOverGas = std::numeric_limits<int64_t>::max();
      uint64_t iterations = 2500000;

      auto start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        state.call(callInfo, txContext, ContractType::CPP, randomnessHash, randomnessHash, randomnessHash, leftOverGas);
      }
      auto end = std::chrono::high_resolution_clock::now();

      long double durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      long double microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "CPP ERC20 transferFrom took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "CPP Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;

      // Check if we actually transferred the tokens.
      uint256_t expectedToBalance = uint256_t(100) * iterations;
      uint256_t transferredToBalance = sdk.callViewFunction(erc20Address, &ERC20::balanceOf, to);
      uint256_t expectedFromBalance = uint256_t("10000000000000000000000") - expectedToBalance;
      uint256_t transferredFromBalance = sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address);
      REQUIRE (expectedToBalance == transferredToBalance);
      REQUIRE (expectedFromBalance == transferredFromBalance);
      // Dump the state
      sdk.getState().saveToDB();
    }

    SECTION("EVM ERC20 Benchmark") {
      std::unique_ptr<Options> options = nullptr;
      Address to(Utils::randBytes(20));

      auto sdk = SDKTestSuite::createNewEnvironment("testERC20EvmBenchmark", {}, nullptr, IndexingMode::DISABLED);

      auto erc20Address = sdk.deployBytecode(erc20bytecode);

      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::name) == "TestToken");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::symbol) == "TST");
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::decimals) == 18);
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::totalSupply) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("10000000000000000000000"));

      // Create the transaction for transfer
      auto functor = UintConv::uint32ToBytes(ABI::FunctorEncoder::encode<Address, uint256_t>("transfer").value);
      Bytes transferEncoded(functor.cbegin(), functor.cend());
      Utils::appendBytes(transferEncoded, ABI::Encoder::encodeData<Address, uint256_t>(to, uint256_t("100")));
      TxBlock transferTx = sdk.createNewTx(sdk.getChainOwnerAccount(), erc20Address, 0, transferEncoded);
      auto& state = sdk.getState();
      uint64_t iterations = 250000;

      auto start = std::chrono::high_resolution_clock::now();
      for (uint64_t i = 0; i < iterations; i++) {
        state.call(transferTx);
      }
      auto end = std::chrono::high_resolution_clock::now();

      long double durationInMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
      long double microSecsPerCall = durationInMicroseconds / iterations;
      std::cout << "EVM ERC20 transfer took " << microSecsPerCall << " microseconds per call" << std::endl;
      std::cout << "EVM Total Time: " << durationInMicroseconds / 1000000 << " seconds" << std::endl;

      // Check if we actually transferred the tokens.
      uint256_t expectedToBalance = uint256_t(100) * iterations;
      uint256_t transferredToBalance = sdk.callViewFunction(erc20Address, &ERC20::balanceOf, to);
      uint256_t expectedFromBalance = uint256_t("10000000000000000000000") - expectedToBalance;
      uint256_t transferredFromBalance = sdk.callViewFunction(erc20Address, &ERC20::balanceOf, sdk.getChainOwnerAccount().address);
      REQUIRE (expectedToBalance == transferredToBalance);
      REQUIRE (expectedFromBalance == transferredFromBalance);
      // Dump the state
      sdk.getState().saveToDB();
    }
  }
}
