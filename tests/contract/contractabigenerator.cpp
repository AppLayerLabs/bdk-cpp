#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/jsonabi.h"
#include <filesystem>
#include <string>

using ContractToTest = std::tuple<ERC20, ERC20Wrapper, NativeWrapper>;

void testABIFiles(const std::string &generatedFilePath, const std::string &testFilePath) {
    std::ifstream generatedFile(generatedFilePath);
    std::ifstream testFile(testFilePath);

    if (!generatedFile.is_open() || !testFile.is_open()) {
        if (!generatedFile.is_open()) {
            std::cout << "Error: Could not open generatedFile at path " << generatedFilePath << std::endl;
        }
        if (!testFile.is_open()) {
            std::cout << "Error: Could not open testFile at path " << testFilePath << std::endl;
        }
        return;
    }

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
    std::string workspace = std::getenv("GITHUB_WORKSPACE") ? std::getenv("GITHUB_WORKSPACE") : ".";
    std::string generatedFilePath = workspace + "/ABI/ERC20.json";
    std::string testFilePath = workspace + "/tests/ERC20.json";

    testABIFiles(generatedFilePath, testFilePath);
  }

  SECTION("ContractABIGenerator check file content ERC20Wrapper") {
    std::string workspace = std::getenv("GITHUB_WORKSPACE") ? std::getenv("GITHUB_WORKSPACE") : ".";
    std::string generatedFilePath = workspace + "/ABI/ERC20Wrapper.json";
    std::string testFilePath = workspace + "/tests/ERC20Wrapper.json";

    testABIFiles(generatedFilePath, testFilePath);
  }

  SECTION("ContractABIGenerator check file content NativeWrapper") {
    std::string workspace = std::getenv("GITHUB_WORKSPACE") ? std::getenv("GITHUB_WORKSPACE") : ".";
    std::string generatedFilePath = workspace + "/ABI/NativeWrapper.json";
    std::string testFilePath = workspace + "/tests/NativeWrapper.json";

    testABIFiles(generatedFilePath, testFilePath);
  }

  SECTION("ContractABIGenerator check file content ContractManager") {
    std::string workspace = std::getenv("GITHUB_WORKSPACE") ? std::getenv("GITHUB_WORKSPACE") : ".";
    std::string generatedFilePath = workspace + "/ABI/ContractManager.json";
    std::string testFilePath = workspace + "/tests/ContractManager.json";

    testABIFiles(generatedFilePath, testFilePath);
  }
}