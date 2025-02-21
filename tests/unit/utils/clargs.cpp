/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "libs/catch2/catch_amalgamated.hpp"

#include "utils/clargs.h"

namespace TClargs {
  TEST_CASE("Command-Line Arguments", "[unit][utils][clargs]") {
    SECTION("parseCommandLineArgs") {
      ProcessOptions opt;
      REQUIRE_FALSE(opt.valid);

      // argv[0] needs to exist so the other args can be parsed correctly.
      // "--help" exits prematurely with exit(0) so we can't test it unless that changes.
      // Casting is done to suppress `-Wwrite-strings`.
      //char* argsHelp[2] = {(char*)"bdkd-tests", (char*)"--help"};
      //opt = parseCommandLineArgs(2, argsHelp, BDKTool::FULL_NODE);
      char* argsFull[9] = {
        (char*)"bdkd-tests",
        (char*)"--loglevel", (char*)"X",
        (char*)"--loglinelimit", (char*)"1000",
        (char*)"--logfilelimit", (char*)"10",
        (char*)"--rootpath", (char*)"aaa",
      };
      opt = parseCommandLineArgs(9, argsFull, BDKTool::FULL_NODE);
      REQUIRE(opt.valid);
      REQUIRE(opt.logLevel == "X");
      REQUIRE(opt.logLineLimit == 1000);
      REQUIRE(opt.logFileLimit == 10);
      REQUIRE(opt.rootPath == "aaa");

      // For coverage
      ProcessOptions opt2;
      char* argsNoLogLineLimit[3] = {(char*)"bdkd-tests", (char*)"--loglinelimit", (char*)"-1",};
      opt2 = parseCommandLineArgs(3, argsNoLogLineLimit, BDKTool::FULL_NODE);
      REQUIRE(!opt2.valid);
      char* argsNoLogFileLimit[3] = {(char*)"bdkd-tests", (char*)"--logfilelimit", (char*)"-1",};
      opt2 = parseCommandLineArgs(3, argsNoLogFileLimit, BDKTool::FULL_NODE);
      REQUIRE(!opt2.valid);
    }

    SECTION("applyProcessOptions") {

      // Set all options except log level manually so we can focus on its coverage later
      ProcessOptions opt;
      REQUIRE_FALSE(opt.valid);
      REQUIRE_FALSE(applyProcessOptions(opt));

      /*
        Tests cannot set process options since process options are process-wide and
        the global test suite (which is a single process) sets its own process-wide options, which
        would then get overwritten by this, and thus break the unit test suite.
        For the process-wide options feature to be testable, some kind of trick is needed, like
        reversible process options (which is a bad idea; just an example).

      opt.valid = true;
      opt.logLevel = "";
      opt.logLineLimit = 1000;
      opt.logFileLimit = 10;
      opt.rootPath = "";
      REQUIRE(applyProcessOptions(opt));

      // Focus exclusively on coverage for all valid log levels
      ProcessOptions opt2;
      opt2.valid = true;

      opt2.logLevel = "X"; // XTRACE
      REQUIRE(applyProcessOptions(opt2));
      opt2.logLevel = "T"; // TRACE
      REQUIRE(applyProcessOptions(opt2));
      opt2.logLevel = "D"; // DEBUG
      REQUIRE(applyProcessOptions(opt2));
      opt2.logLevel = "I"; // INFO
      REQUIRE(applyProcessOptions(opt2));
      opt2.logLevel = "W"; // WARNING
      REQUIRE(applyProcessOptions(opt2));
      opt2.logLevel = "E"; // ERROR
      REQUIRE(applyProcessOptions(opt2));
      opt2.logLevel = "F"; // FATAL
      REQUIRE(applyProcessOptions(opt2));
      opt2.logLevel = "N"; // NONE
      REQUIRE(applyProcessOptions(opt2));
      opt2.logLevel = "?"; // Invalid log level
      REQUIRE_FALSE(applyProcessOptions(opt2));
      */
    }
  }
}

