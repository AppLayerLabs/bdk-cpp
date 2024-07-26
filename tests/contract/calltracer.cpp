/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/calltracer.h"
#include "../sdktestsuite.hpp"
#include "../blockchainwrapper.hpp"
#include "../src/contract/templates/simplecontract.h"

static const Bytes testBytecode = Hex::toBytes("0x608060405234801561001057600080fd5b50610219806100206000396000f3fe608060405234801561001057600080fd5b50600436106100415760003560e01c80631003e2d2146100465780634fa522db14610062578063853255cc14610092575b600080fd5b610060600480360381019061005b9190610129565b6100b0565b005b61007c60048036038101906100779190610129565b6100cb565b6040516100899190610165565b60405180910390f35b61009a6100e5565b6040516100a79190610165565b60405180910390f35b806000808282546100c191906101af565b9250508190555050565b60006100d6826100b0565b6100de6100e5565b9050919050565b60008054905090565b600080fd5b6000819050919050565b610106816100f3565b811461011157600080fd5b50565b600081359050610123816100fd565b92915050565b60006020828403121561013f5761013e6100ee565b5b600061014d84828501610114565b91505092915050565b61015f816100f3565b82525050565b600060208201905061017a6000830184610156565b92915050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052601160045260246000fd5b60006101ba826100f3565b91506101c5836100f3565b92508282019050808211156101dd576101dc610180565b5b9291505056fea264697066735822122010806f8bd0eb78dd8bf1e05d0621ad54dfe78cd22c6a67e02decd89cd4a2208064736f6c63430008130033");
static const Bytes testProxyBytecode = Hex::toBytes("0x608060405234801561001057600080fd5b50610341806100206000396000f3fe608060405234801561001057600080fd5b50600436106100365760003560e01c80637714eaca1461003b5780638e6113211461006b575b600080fd5b61005560048036038101906100509190610232565b61009b565b6040516100629190610281565b60405180910390f35b6100856004803603810190610080919061029c565b610121565b6040516100929190610281565b60405180910390f35b60008273ffffffffffffffffffffffffffffffffffffffff16634fa522db836040518263ffffffff1660e01b81526004016100d69190610281565b6020604051808303816000875af11580156100f5573d6000803e3d6000fd5b505050506040513d601f19601f8201168201806040525081019061011991906102de565b905092915050565b60008173ffffffffffffffffffffffffffffffffffffffff1663853255cc6040518163ffffffff1660e01b8152600401602060405180830381865afa15801561016e573d6000803e3d6000fd5b505050506040513d601f19601f8201168201806040525081019061019291906102de565b9050919050565b600080fd5b600073ffffffffffffffffffffffffffffffffffffffff82169050919050565b60006101c98261019e565b9050919050565b6101d9816101be565b81146101e457600080fd5b50565b6000813590506101f6816101d0565b92915050565b6000819050919050565b61020f816101fc565b811461021a57600080fd5b50565b60008135905061022c81610206565b92915050565b6000806040838503121561024957610248610199565b5b6000610257858286016101e7565b92505060206102688582860161021d565b9150509250929050565b61027b816101fc565b82525050565b60006020820190506102966000830184610272565b92915050565b6000602082840312156102b2576102b1610199565b5b60006102c0848285016101e7565b91505092915050565b6000815190506102d881610206565b92915050565b6000602082840312156102f4576102f3610199565b5b6000610302848285016102c9565b9150509291505056fea26469706673582212201c63da23a5ecb525a33d51b61ad178576a9dbc8733cc98401d2be1db76021bbf64736f6c63430008130033");

inline static Hash getLatestTransactionHash(const Storage& storage) {
  return storage.latest()->getTxs().front().hash();
}

struct TestWrapper {
  uint256_t sum() const { return 0; }
  void add(const uint256_t& val) { }
  uint256_t addAndReturn(const uint256_t& val) { return 0; }

  static void registerContract() {
    ContractReflectionInterface::registerContractMethods<TestWrapper>(
      std::vector<std::string>{},
      std::make_tuple("sum", &TestWrapper::sum, FunctionTypes::View, std::vector<std::string>{}),
      std::make_tuple("add", &TestWrapper::add, FunctionTypes::View, std::vector<std::string>{"val"}),
      std::make_tuple("addAndReturn", &TestWrapper::addAndReturn, FunctionTypes::View, std::vector<std::string>{"val"})
    );
  }
};

struct TestProxyWrapper {
  uint256_t sumOf(const Address& addr) const { return 0; }
  uint256_t addToAndReturn(const Address& addr, const uint256_t& val) { return 0; }

  static void registerContract() {
    ContractReflectionInterface::registerContractMethods<TestProxyWrapper>(
      std::vector<std::string>{},

      std::make_tuple("sumOf", &TestProxyWrapper::sumOf, FunctionTypes::View, std::vector<std::string>{"addr"}),
      std::make_tuple("addToAndReturn", &TestProxyWrapper::addToAndReturn, FunctionTypes::View, std::vector<std::string>{"addr", "val"})
    );
  }
};

namespace TCallTracer {

TEST_CASE("CallTracer Tests", "[trace]") {
  SECTION("EVM Single Call") {
    SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("TestTraceContracts");

    const Address contractAddress = sdk.deployBytecode(testBytecode);

    uint256_t res;

    sdk.callViewFunction(contractAddress, &TestWrapper::sum);
    REQUIRE(res == 0);

    sdk.callFunction(contractAddress, &TestWrapper::add, uint256_t(33));

    Hash txHash = getLatestTransactionHash(sdk.getStorage());
    std::optional<trace::Call> callTrace = sdk.getStorage().getCallTrace(txHash);
    REQUIRE(callTrace);

    REQUIRE(callTrace->type == trace::Call::Type::CALL);
    REQUIRE(callTrace->from == sdk.getOptions().getChainOwner());
    REQUIRE(callTrace->to == contractAddress);
    REQUIRE(callTrace->value == FixedBytes<32>());
    // TODO: gas and gasUsed?
    // TODO: what are these 4 bytes prefix?
    REQUIRE(FixedBytes<32>(callTrace->input | std::views::drop(4)) == FixedBytes<32>(Utils::uint256ToBytes(uint256_t(33))));

    REQUIRE(callTrace->output == Bytes());
    REQUIRE(callTrace->calls.empty());

    res = sdk.callViewFunction(contractAddress, &TestWrapper::sum);
    REQUIRE(res == 33);

    sdk.callFunction(contractAddress, &TestWrapper::addAndReturn, uint256_t(66));

    txHash = getLatestTransactionHash(sdk.getStorage());
    callTrace = sdk.getStorage().getCallTrace(txHash);
    REQUIRE(callTrace);

    REQUIRE(callTrace->type == trace::Call::Type::CALL);
    REQUIRE(callTrace->from == sdk.getOptions().getChainOwner());
    REQUIRE(callTrace->to == contractAddress);
    REQUIRE(callTrace->value == FixedBytes<32>());
    // TODO: gas and gasUsed?
    // TODO: what are these 4 bytes prefix?
    REQUIRE(FixedBytes<32>(callTrace->input | std::views::drop(4)) == FixedBytes<32>(Utils::uint256ToBytes(uint256_t(66))));

    REQUIRE(callTrace->output == Utils::makeBytes(Utils::uint256ToBytes(uint256_t(99))));
    REQUIRE(callTrace->calls.empty());

    res = sdk.callViewFunction(contractAddress, &TestWrapper::sum);
    REQUIRE(res == 99);
  }

  SECTION("EVM Nested Calls") {
    SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("TestTraceContracts");
    const Address testContractAddress = sdk.deployBytecode(testBytecode);
    const Address testProxyContractAddress = sdk.deployBytecode(testProxyBytecode);

    uint256_t res;

    res = sdk.callViewFunction(testContractAddress, &TestWrapper::sum);
    REQUIRE(res == 0);

    sdk.callFunction(testContractAddress, &TestWrapper::add, uint256_t(45));

    res = sdk.callViewFunction(testProxyContractAddress, &TestProxyWrapper::sumOf, testContractAddress);
    REQUIRE(res == 45);

    sdk.callFunction(testProxyContractAddress, &TestProxyWrapper::addToAndReturn, testContractAddress, uint256_t(55));

    const Hash txHash = getLatestTransactionHash(sdk.getStorage());
    std::optional<trace::Call> callTrace = sdk.getStorage().getCallTrace(txHash);
    REQUIRE(callTrace);

    REQUIRE(callTrace->type == trace::Call::Type::CALL);
    REQUIRE(callTrace->from == sdk.getOptions().getChainOwner());
    REQUIRE(callTrace->to == testProxyContractAddress);
    REQUIRE(callTrace->value == FixedBytes<32>());
    // TODO: gas and gasUsed
    REQUIRE(Address(callTrace->input | std::views::drop(16) | std::views::take(20)) == testContractAddress);
    REQUIRE(FixedBytes<32>(callTrace->input | std::views::drop(36)) == FixedBytes<32>(Utils::uint256ToBytes(uint256_t(55))));

    REQUIRE(callTrace->output == Utils::makeBytes(Utils::uint256ToBytes(uint256_t(100))));
    REQUIRE(callTrace->calls.size() == 1);

    const trace::Call& nestedCall = callTrace->calls.front();

    REQUIRE(nestedCall.type == trace::Call::Type::CALL);
    REQUIRE(nestedCall.from == testProxyContractAddress);
    REQUIRE(nestedCall.to == testContractAddress);
    REQUIRE(nestedCall.value == FixedBytes<32>());
    REQUIRE(FixedBytes<32>(nestedCall.input | std::views::drop(4)) == FixedBytes<32>(Utils::uint256ToBytes(uint256_t(55))));

    REQUIRE(nestedCall.output == Utils::makeBytes(Utils::uint256ToBytes(uint256_t(100))));
    REQUIRE(nestedCall.calls.empty());

    res = sdk.callViewFunction(testContractAddress, &TestWrapper::sum);
    REQUIRE(res == 100);
  }
}

} // namespace TCallTracer
