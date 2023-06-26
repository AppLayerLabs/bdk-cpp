#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/jsonabi.h"
#include <filesystem>

using ContractToTest = std::tuple<ERC20, ERC20Wrapper, NativeWrapper>;

void testABIFiles(std::ifstream &generatedFile, std::ifstream &testFile) {
    std::string generatedFileContent((std::istreambuf_iterator<char>(generatedFile)),
                                     std::istreambuf_iterator<char>());
    std::string testFileContent((std::istreambuf_iterator<char>(testFile)),
                                std::istreambuf_iterator<char>());

    REQUIRE(generatedFileContent == testFileContent);

    generatedFile.close();
    testFile.close();
}

TEST_CASE("ContractABIGenerator helper", "[contract][contractabigenerator]") {
  SECTION("ContractABIGenerator writeContractsToJson") {
    REQUIRE(JsonAbi::writeContractsToJson<ContractToTest>() == 0);
    REQUIRE(std::filesystem::exists("ABI/ERC20.json"));
    REQUIRE(std::filesystem::exists("ABI/ERC20Wrapper.json"));
    REQUIRE(std::filesystem::exists("ABI/NativeWrapper.json"));
    REQUIRE(std::filesystem::exists("ABI/ContractManager.json"));
  }

  SECTION("ContractABIGenerator check file content ERC20") {
    std::ifstream generatedFile("ABI/ERC20.json");
    std::ifstream testFile("tests/ERC20ABI.json");

    testABIFiles(generatedFile, testFile);
  }

  SECTION("ContractABIGenerator check file content ERC20Wrapper") {
    std::ifstream generatedFile("ABI/ERC20Wrapper.json");
    std::ifstream testFile("tests/ERC20WrapperABI.json");

    testABIFiles(generatedFile, testFile);
  }

  SECTION("ContractABIGenerator check file content NativeWrapper") {
    std::ifstream generatedFile("ABI/NativeWrapper.json");
    std::ifstream testFile("tests/NativeWrapperABI.json");

    testABIFiles(generatedFile, testFile);
  }

  SECTION("ContractABIGenerator check file content ContractManager") {

    std::ifstream generatedFile("ABI/ContractManager.json");
    std::ifstream testFile("tests/ContractManagerABI.json");

    testABIFiles(generatedFile, testFile);
  }
}