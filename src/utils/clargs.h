/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CLARGS_H
#define CLARGS_H

#include <boost/program_options.hpp> // includes string internally (and probably algorithm somewhere due to std::transform)

#include "../net/p2p/managernormal.h"

/// List of BDK programs that the argument parser is aware of.
enum class BDKTool { FULL_NODE, DISCOVERY_NODE, UNIT_TEST_SUITE };

/**
 * Result of parsing command-line options for the node.
 * Default option values should signal that they weren't set, either by command-line
 * arguments or by the program itself.
 */
struct ProcessOptions {
  bool valid = false;      ///< Set to true only if parameter parsing did not fail
  std::string logLevel;    ///< Desired log level name
  int logLineLimit = -1;   ///< Desired log line count limit for the rotating logger log file
  int logFileLimit = -1;   ///< Desired log file hard limit (erases older log files past this count)
  int netThreads = -1;     ///< Desired IO thread count for P2P message processing
};

/**
 * This function can be called from the main() function of any BDK node program to
 * parse a set of command-line arguments. It does not check if the provided values
 * are valid, only the expected argument count/type format.
 * @param argc Forwarded argc from main().
 * @param argv Forwarded argv from main().
 * @param tool Which tool is taking args; can be used to determine which args are available.
 * @return A ProcessOptions struct with the result of argument parsing.
 */
ProcessOptions parseCommandLineArgs(int argc, char* argv[], [[maybe_unused]] BDKTool tool);

/**
 * This function provides a default way to apply a ProcessOptions object.
 * @param opt Process Options object to apply.
 * @return `true` if no error, `false` otherwise.
 */
bool applyProcessOptions(ProcessOptions& opt);

#endif // CLARGS_H
