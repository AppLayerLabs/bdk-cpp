/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef CLARGS_H
#define CLARGS_H

#include <boost/program_options.hpp> // includes string internally (and probably algorithm somewhere due to std::transform)

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
ProcessOptions parseCommandLineArgs(int argc, char* argv[], [[maybe_unused]] BDKTool tool) {
  ProcessOptions opt;
  try {
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
      ("help,h",
        "Print help message and exit")
      ("loglevel,l", boost::program_options::value<std::string>(),
        "Set the log level ([T]RACE, [D]EBUG, [I]NFO, [W]ARNING, [E]RROR, [N]ONE)")
      ("loglinelimit", boost::program_options::value<int>(),
        "Set the log line limit (# of lines per file); 0 = no limit")
      ("logfilelimit", boost::program_options::value<int>(),
        "Set the log file limit (# of files); 0 = no limit")
      ("netthreads", boost::program_options::value<int>(),
        "Set ManagerBase::netThreads_ (main IO thread count)")
      ;

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
      std::cout << desc << "\n";
      exit(0);
    }

    if (vm.count("loglevel")) {
      opt.logLevel = vm["loglevel"].as<std::string>();
    }

    if (vm.count("loglinelimit")) {
      opt.logLineLimit = vm["loglinelimit"].as<int>();
      if (opt.logLineLimit < 0) {
        std::cerr << "ERROR: --loglinelimit must be >= 0\n";
        return {};
      }
    }

    if (vm.count("logfilelimit")) {
      opt.logFileLimit = vm["logfilelimit"].as<int>();
      if (opt.logFileLimit < 0) {
        std::cerr << "ERROR: --logfilelimit must be >= 0\n";
        return {};
      }
    }

    if (vm.count("netthreads")) {
      opt.netThreads = vm["netthreads"].as<int>();
      if (opt.netThreads < 1) {
        std::cerr << "ERROR: --netthreads must be >= 1\n";
        return {};
      }
    }

  } catch (std::exception& e) {
    std::cout << "ERROR: parseCommandLineArgs(): " << e.what() << "\n";
    return {};
  } catch (...) {
    std::cout << "ERROR: parseCommandLineArgs(): Unknown exception\n";
    return {};
  }

  opt.valid = true;
  return opt;
}

/**
 * This function provides a default way to apply a ProcessOptions object.
 * @param opt Process Options object to apply.
 * @return `true` if no error, `false` otherwise.
 */
bool applyProcessOptions(ProcessOptions& opt) {
  if (!opt.valid) {
    std::cout << "ERROR: Invalid command-line arguments" << std::endl;
    return false;
  }

  std::transform(opt.logLevel.begin(), opt.logLevel.end(), opt.logLevel.begin(), ::toupper);

  if (opt.logLevel == "X") { opt.logLevel = "XTRACE"; }
  else if (opt.logLevel == "T") { opt.logLevel = "TRACE"; }
  else if (opt.logLevel == "D") { opt.logLevel = "DEBUG"; }
  else if (opt.logLevel == "I") { opt.logLevel = "INFO"; }
  else if (opt.logLevel == "W") { opt.logLevel = "WARNING"; }
  else if (opt.logLevel == "E") { opt.logLevel = "ERROR"; }
  else if (opt.logLevel == "F") { opt.logLevel = "FATAL"; }
  else if (opt.logLevel == "N") { opt.logLevel = "NONE"; }

  if (opt.logLevel == "") { /* Do nothing */ }
  else if (opt.logLevel == "XTRACE")  { Logger::setLogLevel(LogType::XTRACE); }
  else if (opt.logLevel == "TRACE")   { Logger::setLogLevel(LogType::TRACE); }
  else if (opt.logLevel == "DEBUG")   { Logger::setLogLevel(LogType::DEBUG); }
  else if (opt.logLevel == "INFO")    { Logger::setLogLevel(LogType::INFO); }
  else if (opt.logLevel == "WARNING") { Logger::setLogLevel(LogType::WARNING); }
  else if (opt.logLevel == "ERROR")   { Logger::setLogLevel(LogType::ERROR); }
  else if (opt.logLevel == "FATAL")   { Logger::setLogLevel(LogType::FATAL); }
  else if (opt.logLevel == "NONE")    { Logger::setLogLevel(LogType::NONE); }
  else {
    std::cout << "ERROR: Invalid log level requested: " << opt.logLevel << std::endl;
    return false;
  }

  if (opt.logLevel != "") {
    std::cout << "Log level set to " << opt.logLevel << std::endl;
  }

  if (opt.logLineLimit >= 0) {
    Logger::setLogLineLimit(opt.logLineLimit);
    std::cout << "Log line limit set to " << opt.logLineLimit << std::endl;
  }

  if (opt.logFileLimit >= 0) {
    Logger::setLogFileLimit(opt.logFileLimit);
    std::cout << "Log file limit set to " << opt.logFileLimit << std::endl;
  }

  if (opt.netThreads >= 0) { // negative number signals unset; 0 is invalid, but somehow it was set to that value
    P2P::ManagerBase::setNetThreads(opt.netThreads);
    std::cout << "ManagerBase::netThreads_ set to " << opt.netThreads << std::endl;
  }

  return true;
}

#endif // CLARGS_H
