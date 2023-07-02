#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/jsonabi.h"
#include <filesystem>

using ContractToTest = std::tuple<ERC20, ERC20Wrapper, NativeWrapper>;

void testABIFiles(std::ifstream &generatedFile, std::ifstream &testFile) {
  if (!generatedFile.is_open() || !testFile.is_open()) {
      if (!generatedFile.is_open()) {
          throw std::runtime_error("Error: Could not open generatedFile.");
      }
      if (!testFile.is_open()) {
          throw std::runtime_error("Error: Could not open testFile.");
      }
  }

  std::string generatedFileContent((std::istreambuf_iterator<char>(generatedFile)),
                                    std::istreambuf_iterator<char>());
  std::string testFileContent((std::istreambuf_iterator<char>(testFile)),
                              std::istreambuf_iterator<char>());

  std::cout << "Generated file content: " << generatedFileContent << std::endl;
  std::cout << "Test file content: " << testFileContent << std::endl;

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
    std::ifstream testFile("tests/ERC20.json");

    testABIFiles(generatedFile, testFile);
  }

  SECTION("ContractABIGenerator check file content ERC20Wrapper") {
    std::ifstream generatedFile("ABI/ERC20Wrapper.json");
    std::ifstream testFile("tests/ERC20Wrapper.json");

    testABIFiles(generatedFile, testFile);
  }

  SECTION("ContractABIGenerator check file content NativeWrapper") {
    std::ifstream generatedFile("ABI/NativeWrapper.json");
    std::ifstream testFile("tests/NativeWrapper.json");

    testABIFiles(generatedFile, testFile);
  }

  SECTION("ContractABIGenerator check file content ContractManager") {

    std::ifstream generatedFile("ABI/ContractManager.json");
    std::ifstream testFile("tests/ContractManager.json");

    testABIFiles(generatedFile, testFile);
  }
}