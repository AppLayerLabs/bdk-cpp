/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "libs/catch2/catch_amalgamated.hpp"

#include "utils/clargs.h" // ProcessOptions
#include "utils/utils.h" // safePrintTest()

// Custom logging listener for Catch2.
class LoggingListener : public Catch::EventListenerBase {
  public:
    using EventListenerBase::EventListenerBase;

    std::string testCaseName = "NONE";

    // Called when a test run is starting
    void testRunStarting(Catch::TestRunInfo const& testRunInfo) override {
      GLOGINFO("Starting test run: " + testRunInfo.name);
    }

    // Called when a test case is starting
    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
      GLOGINFO("Starting TEST_CASE: " + testInfo.name);
      testCaseName = testInfo.name;
    }

    // Called when a section is starting
    void sectionStarting(Catch::SectionInfo const& sectionInfo) override {
      GLOGINFO("[" + testCaseName + "]: Starting SECTION: " + sectionInfo.name);
    }

    void sectionEnded(Catch::SectionStats const& sectionStats) override {
      GLOGINFO("[" + testCaseName + "]: Finished SECTION: " + sectionStats.sectionInfo.name);
    }

    // Called when a test case has ended
    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
      GLOGINFO("Finished TEST_CASE: " + testCaseStats.testInfo->name);
      testCaseName = "NONE";
    }

    // Called when a test run has ended
    void testRunEnded(Catch::TestRunStats const& testRunStats) override {
      GLOGINFO("Finished test run: " + std::to_string(testRunStats.totals.testCases.total()) + " test cases run.");
    }
};

CATCH_REGISTER_LISTENER(LoggingListener)

/**
 * Custom main function for Catch2.
 * CMakeLists.txt defines CATCH_AMALGAMATED_CUSTOM_MAIN so we can define our own main() here.
 */
int main(int argc, char* argv[]) {
  Utils::safePrintTest("bdkd-tests: Blockchain Development Kit unit test suite");
  Utils::safePrintTest("Any arguments before -- are sent to Catch2");
  Utils::safePrintTest("Any arguments after -- are sent to the BDK args parser");

  std::vector<char*> bdkArgs;
  std::vector<char*> catchArgs;
  bdkArgs.push_back(argv[0]);
  catchArgs.push_back(argv[0]);

  bool bdkArgsStarted = false;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--") == 0) {
      bdkArgsStarted = true;
      continue;
    }
    if (bdkArgsStarted) {
      bdkArgs.push_back(argv[i]);
    } else {
      catchArgs.push_back(argv[i]);
    }
  }

  // Even if there are no BDK args supplied, run this to apply the default debug level we want
  Utils::safePrintTest("Processing BDK args and defaults...");
  ProcessOptions opt = parseCommandLineArgs(bdkArgs.size(), bdkArgs.data(), BDKTool::UNIT_TEST_SUITE);
  if (opt.logLevel == "") opt.logLevel = "DEBUG";
  if (!applyProcessOptions(opt)) return 1;

  // Run Catch2
  Utils::safePrintTest("Running Catch2...");
  int result = Catch::Session().run(catchArgs.size(), catchArgs.data());
  return result;
}

