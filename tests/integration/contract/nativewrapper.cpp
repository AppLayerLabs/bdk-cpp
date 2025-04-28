/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "libs/catch2/catch_amalgamated.hpp"

#include "contract/templates/standards/erc20.h"
#include "contract/templates/nativewrapper.h"
#include "contract/abi.h"
#include "contract/contractmanager.h"

#include "utils/db.h"
#include "utils/options.h"

#include "core/storage.h"
#include "core/state.h"

#include "../../sdktestsuite.hpp"

// TODO: test events if/when implemented

struct WETHWrapper {
  void setWrapper(const Address& address) {}
  void static registerContract() {
    ContractReflectionInterface::registerContractMethods<
      WETHWrapper
    >(
      std::vector<std::string>{},
      std::make_tuple("setWrapper", &WETHWrapper::setWrapper, FunctionTypes::NonPayable, std::vector<std::string>{"address"})
    );
  }
};

namespace TNativeWrapper {
  TEST_CASE("NativeWrapper tests", "[integration][contract][nativewrapper]") {
    Bytes nativeWrapperSolEncoded = Hex::toBytes("0x60806040526040518060400160405280600d81526020017f57726170706564204574686572000000000000000000000000000000000000008152506000908051906020019061004f9291906100ca565b506040518060400160405280600481526020017f57455448000000000000000000000000000000000000000000000000000000008152506001908051906020019061009b9291906100ca565b506012600260006101000a81548160ff021916908360ff1602179055503480156100c457600080fd5b5061016f565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f1061010b57805160ff1916838001178555610139565b82800160010185558215610139579182015b8281111561013857825182559160200191906001019061011d565b5b509050610146919061014a565b5090565b61016c91905b80821115610168576000816000905550600101610150565b5090565b90565b610cb18061017e6000396000f3fe60806040526004361061009c5760003560e01c8063313ce56711610064578063313ce567146102a257806370a08231146102d357806395d89b4114610338578063a9059cbb146103c8578063d0e30db01461043b578063dd62ed3e146104455761009c565b806306fdde03146100a6578063095ea7b31461013657806318160ddd146101a957806323b872dd146101d45780632e1a7d4d14610267575b6100a46104ca565b005b3480156100b257600080fd5b506100bb610567565b6040518080602001828103825283818151815260200191508051906020019080838360005b838110156100fb5780820151818401526020810190506100e0565b50505050905090810190601f1680156101285780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b34801561014257600080fd5b5061018f6004803603604081101561015957600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919080359060200190929190505050610605565b604051808215151515815260200191505060405180910390f35b3480156101b557600080fd5b506101be6106f7565b6040518082815260200191505060405180910390f35b3480156101e057600080fd5b5061024d600480360360608110156101f757600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803590602001909291905050506106ff565b604051808215151515815260200191505060405180910390f35b34801561027357600080fd5b506102a06004803603602081101561028a57600080fd5b8101908080359060200190929190505050610a48565b005b3480156102ae57600080fd5b506102b7610b79565b604051808260ff1660ff16815260200191505060405180910390f35b3480156102df57600080fd5b50610322600480360360208110156102f657600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610b8c565b6040518082815260200191505060405180910390f35b34801561034457600080fd5b5061034d610ba4565b6040518080602001828103825283818151815260200191508051906020019080838360005b8381101561038d578082015181840152602081019050610372565b50505050905090810190601f1680156103ba5780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b3480156103d457600080fd5b50610421600480360360408110156103eb57600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919080359060200190929190505050610c42565b604051808215151515815260200191505060405180910390f35b6104436104ca565b005b34801561045157600080fd5b506104b46004803603604081101561046857600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610c57565b6040518082815260200191505060405180910390f35b34600360003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020600082825401925050819055503373ffffffffffffffffffffffffffffffffffffffff167fe1fffcc4923d04b559f4d29a8bfc6cda04eb5b0d3c460751c2402c5c5cc9109c346040518082815260200191505060405180910390a2565b60008054600181600116156101000203166002900480601f0160208091040260200160405190810160405280929190818152602001828054600181600116156101000203166002900480156105fd5780601f106105d2576101008083540402835291602001916105fd565b820191906000526020600020905b8154815290600101906020018083116105e057829003601f168201915b505050505081565b600081600460003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055508273ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffffffffffffffff167f8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925846040518082815260200191505060405180910390a36001905092915050565b600047905090565b600081600360008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002054101561074d57600080fd5b3373ffffffffffffffffffffffffffffffffffffffff168473ffffffffffffffffffffffffffffffffffffffff161415801561082557507fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff600460008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000205414155b1561093e5781600460008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000205410156108b357600080fd5b81600460008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020600082825403925050819055505b81600360008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000206000828254039250508190555081600360008573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020600082825401925050819055508273ffffffffffffffffffffffffffffffffffffffff168473ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef846040518082815260200191505060405180910390a3600190509392505050565b80600360003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020541015610a9457600080fd5b80600360003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020600082825403925050819055503373ffffffffffffffffffffffffffffffffffffffff166108fc829081150290604051600060405180830381858888f19350505050158015610b27573d6000803e3d6000fd5b503373ffffffffffffffffffffffffffffffffffffffff167f7fcf532c15f0a6db0bd6d0e038bea71d30d808c7d98cb3bf7268a95bf5081b65826040518082815260200191505060405180910390a250565b600260009054906101000a900460ff1681565b60036020528060005260406000206000915090505481565b60018054600181600116156101000203166002900480601f016020809104026020016040519081016040528092919081815260200182805460018160011615610100020316600290048015610c3a5780601f10610c0f57610100808354040283529160200191610c3a565b820191906000526020600020905b815481529060010190602001808311610c1d57829003601f168201915b505050505081565b6000610c4f3384846106ff565b905092915050565b600460205281600052604060002060205280600052604060002060009150915050548156fea265627a7a7231582057681b07f208394e1de81e1a79eb9a703a3255f6a006dd3835a6b1ffad3f021864736f6c63430005110032");
    Bytes wrapperWrapperSolEncoded = Hex::toBytes("0x6080604052348015600e575f5ffd5b506104a18061001c5f395ff3fe608060405260043610610037575f3560e01c80632e1a7d4d14610042578063c2167d931461006a578063d0e30db0146100925761003e565b3661003e57005b5f5ffd5b34801561004d575f5ffd5b50610068600480360381019061006391906102c9565b61009c565b005b348015610075575f5ffd5b50610090600480360381019061008b919061035f565b6101d1565b005b61009a610213565b005b5f5f9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16632e1a7d4d826040518263ffffffff1660e01b81526004016100f59190610399565b5f604051808303815f87803b15801561010c575f5ffd5b505af115801561011e573d5f5f3e3d5ffd5b505050505f3373ffffffffffffffffffffffffffffffffffffffff16825a90604051610149906103df565b5f60405180830381858888f193505050503d805f8114610184576040519150601f19603f3d011682016040523d82523d5f602084013e610189565b606091505b50509050806101cd576040517f08c379a00000000000000000000000000000000000000000000000000000000081526004016101c49061044d565b60405180910390fd5b5050565b805f5f6101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050565b5f5f9054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1663d0e30db0346040518263ffffffff1660e01b81526004015f604051808303818588803b158015610279575f5ffd5b505af115801561028b573d5f5f3e3d5ffd5b5050505050565b5f5ffd5b5f819050919050565b6102a881610296565b81146102b2575f5ffd5b50565b5f813590506102c38161029f565b92915050565b5f602082840312156102de576102dd610292565b5b5f6102eb848285016102b5565b91505092915050565b5f73ffffffffffffffffffffffffffffffffffffffff82169050919050565b5f61031d826102f4565b9050919050565b5f61032e82610313565b9050919050565b61033e81610324565b8114610348575f5ffd5b50565b5f8135905061035981610335565b92915050565b5f6020828403121561037457610373610292565b5b5f6103818482850161034b565b91505092915050565b61039381610296565b82525050565b5f6020820190506103ac5f83018461038a565b92915050565b5f81905092915050565b50565b5f6103ca5f836103b2565b91506103d5826103bc565b5f82019050919050565b5f6103e9826103bf565b9150819050919050565b5f82825260208201905092915050565b7f455448207472616e73666572206661696c6564000000000000000000000000005f82015250565b5f6104376013836103f3565b915061044282610403565b602082019050919050565b5f6020820190508181035f8301526104648161042b565b905091905056fea2646970667358221220e30f06184ca8881cbaa664406295f0347c494b2b5e4ce280a6c9d3f31f6bec9c64736f6c634300081c0033");
    const uint256_t gasPrice = 1'000'000'000;
    SECTION("NativeWrapper creation") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testNativeWrapperCreation");
      Address nativeWrapper = sdk.deployContract<NativeWrapper>(
        std::string("WrappedToken"), std::string("WTKN"), uint8_t(18)
      );
      Address owner = sdk.getChainOwnerAccount().address;
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::name) == "WrappedToken");
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::symbol) == "WTKN");
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::decimals) == 18);
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::totalSupply) == 0);
    }

    SECTION("NativeWrapper deposit() and withdraw()") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testNativeWrapperDepositAndWithdraw");
      Address nativeWrapper = sdk.deployContract<NativeWrapper>(
        std::string("WrappedToken"), std::string("WTKN"), uint8_t(18)
      );
      Address owner = sdk.getChainOwnerAccount().address;
      uint256_t amountToTransfer = uint256_t("192838158112259");
      uint256_t amountToWithdraw = amountToTransfer / 3;

      Hash depositTx = sdk.callFunction(nativeWrapper, &NativeWrapper::deposit, amountToTransfer);

      REQUIRE(sdk.getNativeBalance(nativeWrapper) == amountToTransfer);

      uint256_t expectedGasUsed = gasPrice * (uint256_t(CONTRACT_EXECUTION_COST) * 2 + CPP_CONTRACT_CREATION_COST + CPP_CONTRACT_CALL_COST);

      REQUIRE(sdk.getNativeBalance(owner) == uint256_t("1000000000000000000000") - amountToTransfer - expectedGasUsed);
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::balanceOf, owner) == amountToTransfer);

      Hash withdrawTx = sdk.callFunction(nativeWrapper, &NativeWrapper::withdraw, amountToWithdraw);
      expectedGasUsed += gasPrice * (CONTRACT_EXECUTION_COST + CPP_CONTRACT_CALL_COST);

      REQUIRE(sdk.getNativeBalance(nativeWrapper) == amountToTransfer - amountToWithdraw);
      REQUIRE(sdk.getNativeBalance(owner) == uint256_t("1000000000000000000000") - amountToTransfer + amountToWithdraw - expectedGasUsed);
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::balanceOf, owner) == amountToTransfer - amountToWithdraw);
    }

    SECTION("Solidity NativeWrapper deposit() and withdraw()") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testSolidityNativeWrapperDepositAndWithdraw");
      auto nativeWrapper = sdk.deployBytecode(nativeWrapperSolEncoded);

      Address owner = sdk.getChainOwnerAccount().address;
      uint256_t amountToTransfer = uint256_t("192838158112259");
      uint256_t amountToWithdraw = amountToTransfer / 3;

      auto prevBalance = sdk.getNativeBalance(owner);
      Hash depositTx = sdk.callFunction(nativeWrapper, &NativeWrapper::deposit, amountToTransfer);

      REQUIRE(sdk.getNativeBalance(nativeWrapper) == amountToTransfer);
      auto txAdditionalInfo = sdk.getStorage().getTxAdditionalData(depositTx);

      uint256_t expectedGasUsed = gasPrice * txAdditionalInfo->gasUsed;

      REQUIRE(sdk.getNativeBalance(owner) == prevBalance - amountToTransfer - expectedGasUsed);
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::balanceOf, owner) == amountToTransfer);

      prevBalance = sdk.getNativeBalance(owner);
      Hash withdrawTx = sdk.callFunction(nativeWrapper, &NativeWrapper::withdraw, amountToWithdraw);
      txAdditionalInfo = sdk.getStorage().getTxAdditionalData(withdrawTx);
      expectedGasUsed = gasPrice * txAdditionalInfo->gasUsed;

      REQUIRE(sdk.getNativeBalance(nativeWrapper) == amountToTransfer - amountToWithdraw);
      REQUIRE(sdk.getNativeBalance(owner) == prevBalance + amountToWithdraw - expectedGasUsed);
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::balanceOf, owner) == amountToTransfer - amountToWithdraw);
    }

    SECTION("Solidity NativeWrapper deposit() and withdraw() WITH WETHWrapper") {
      SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testSolidityNativeWrapperDepositAndWithdraw");
      auto nativeWrapper = sdk.deployBytecode(nativeWrapperSolEncoded);
      auto wethWrapper = sdk.deployBytecode(wrapperWrapperSolEncoded);

      Hash setWrapperTx = sdk.callFunction(wethWrapper, &WETHWrapper::setWrapper, nativeWrapper);

      Address owner = sdk.getChainOwnerAccount().address;
      uint256_t amountToTransfer = uint256_t("192838158112259");
      uint256_t amountToWithdraw = amountToTransfer / 3;

      auto prevBalance = sdk.getNativeBalance(owner);
      Hash depositTx = sdk.callFunction(wethWrapper, &NativeWrapper::deposit, amountToTransfer);

      REQUIRE(sdk.getNativeBalance(nativeWrapper) == amountToTransfer);
      auto txAdditionalInfo = sdk.getStorage().getTxAdditionalData(depositTx);

      uint256_t expectedGasUsed = gasPrice * txAdditionalInfo->gasUsed;

      REQUIRE(sdk.getNativeBalance(owner) == prevBalance - amountToTransfer - expectedGasUsed);
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::balanceOf, wethWrapper) == amountToTransfer);

      prevBalance = sdk.getNativeBalance(owner);
      Hash withdrawTx;
      withdrawTx = sdk.callFunction(wethWrapper, &NativeWrapper::withdraw, amountToWithdraw);
      txAdditionalInfo = sdk.getStorage().getTxAdditionalData(withdrawTx);
      expectedGasUsed = gasPrice * txAdditionalInfo->gasUsed;

      REQUIRE(sdk.getNativeBalance(nativeWrapper) == amountToTransfer - amountToWithdraw);
      REQUIRE(sdk.getNativeBalance(owner) == prevBalance + amountToWithdraw - expectedGasUsed);
      REQUIRE(sdk.callViewFunction(nativeWrapper, &NativeWrapper::balanceOf, wethWrapper) == amountToTransfer - amountToWithdraw);
    }
  }
}
