/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <queue>
#include <condition_variable>
#include <future>

/// Enum for the log message types.
enum class LogType { TRACE, DEBUG, INFO, WARNING, ERROR, NONE };

/// Namespace with string prefixes for each blockchain module, for printing log/debug messages.
namespace Log {
  ///@{
  /** String for the given module. */
  const std::string blockchain = "Blockchain";
  const std::string storage = "Storage";
  const std::string snowmanVM = "SnowmanVM";
  const std::string mutableBlock = "MutableBlock";
  const std::string finalizedBlock = "FinalizedBlock";
  const std::string db = "DB";
  const std::string state = "State";
  const std::string grpcServer = "gRPCServer";
  const std::string grpcClient = "gRPCClient";
  const std::string utils = "Utils";
  const std::string httpServer = "HTTPServer";
  const std::string JsonRPCEncoding = "JsonRPC::Encoding";
  const std::string JsonRPCDecoding = "JsonRPC::Decoding";
  const std::string rdPoS = "rdPoS";
  const std::string ABI = "ABI";
  const std::string P2PSession = "P2P::Session";
  const std::string P2PClientFactory = "P2P::ClientFactory";
  const std::string P2PServer = "P2P::Server";
  const std::string P2PServerListener = "P2P::ServerListener";
  const std::string P2PManager = "P2P::Manager";
  const std::string P2PParser = "P2P::Parser";
  const std::string P2PRequestEncoder = "P2P::RequestEncoder";
  const std::string P2PRequestDecoder = "P2P::RequestDecoder";
  const std::string P2PResponseEncoder = "P2P::AnswerDecoder";
  const std::string P2PResponseDecoder = "P2P::AnswerEncoder";
  const std::string P2PBroadcastEncoder = "P2P::BroadcastEncoder";
  const std::string P2PDiscoveryWorker = "P2P::DiscoveryWorker";
  const std::string contractManager = "ContractManager";
  const std::string syncer = "Syncer";
  const std::string event = "Event";
  const std::string nodeConns = "P2P::NodeConns";
  const std::string consensus = "Consensus";
  const std::string contractHost = "ContractHost";
  const std::string dumpWorker = "DumpWorker";
  const std::string dumpManager = "DumpManager";
  const std::string logger = "Logger";
  const std::string sdkTestSuite = "SDKTestSuite";
  ///@}
}

/// Class for storing log information.
class LogInfo {
  private:
    LogType type_;        ///< Log type.
    std::string logSrc_;  ///< Log source.
    std::string func_;    ///< Function name.
    std::string message_; ///< Message to log.

  public:
    /// Empty constructor.
    LogInfo() : type_(LogType::DEBUG), func_(""), logSrc_(""), message_("") {};

    /**
     * Constructor.
     * @param type The log type.
     * @param logSrc The log source.
     * @param func The function name.
     * @param message The message to log.
     */
    LogInfo(LogType type, const std::string& logSrc, std::string&& func, std::string&& message) :
      type_(type), logSrc_(logSrc), func_(std::move(func)), message_(std::move(message)) {};

    ~LogInfo() = default; ///< Default destructor.

    /// Move constructor
    LogInfo(LogInfo&& other) noexcept :
      type_(other.type_), func_(std::move(other.func_)),
      logSrc_(std::move(other.logSrc_)), message_(std::move(other.message_))
    {};

    /// Move assign operator
    LogInfo& operator=(LogInfo&& other) noexcept {
      this->type_ = other.type_;
      this->func_ = std::move(other.func_);
      this->logSrc_ = std::move(other.logSrc_);
      this->message_ = std::move(other.message_);
      return *this;
    }

    ///@{
    /** Getter. */
    inline const LogType& getType() const noexcept { return this->type_; };
    inline const std::string& getLogSrc() const noexcept { return this->logSrc_; };
    inline const std::string& getFunc() const noexcept { return this->func_; };
    inline const std::string& getMessage() const noexcept { return this->message_; };
    ///@}
};

/// Singleton class for logging.
class Logger {
  private:
    /// Private constructor as it is a singleton.
    Logger() : activeLogFileName_("bdk.log") {
      logLevel_ = LogType::NONE; // The logger component does not log anything by default
      logLineLimit_ = 100000;
      logFileLimit_ = 0; // No limit to the number of log files by default
      logThreadFuture_ = std::async(std::launch::async, &Logger::logger, this);
    }
    Logger(const Logger&) = delete;             ///< Make it non-copyable
    Logger& operator=(const Logger&) = delete;  ///< Make it non-assignable.

    /// Get the instance.
    static Logger& getInstance() { static Logger instance; return instance; }

    const std::string activeLogFileName_;   ///< Base name for log files
    std::atomic<LogType> logLevel_;         ///< Current log level (doesn't log anything less than this).
    std::atomic<int> logLineLimit_;         ///< Number of log lines until the log rotates.
    std::atomic<int> logFileLimit_;         ///< Number of log files to keep before deleting older ones.
    std::ofstream logFile_;                 ///< The file stream.
    std::mutex logQueueMutex_;              ///< Mutex for protecting access to the log queue.
    std::condition_variable cv_;            ///< Conditional variable to wait for new tasks.
    std::queue<LogInfo> logQueue_;          ///< Queue for the log tasks.
    LogInfo curTask_;                       ///< Current task being executed.
    std::atomic<bool> stopWorker_ = false;  ///< Flag for stopping the thread.
    std::future<void> logThreadFuture_;     ///< Future object used to wait for the log thread to finish.

    /// Function for the future object.
    void logger() {
      int logFileNum = -1;
      int logLineCount = INT_MAX;
      while (true) {
        if (logLineCount > logLineLimit_) {
          if (logFileNum >= 0) {
            std::string archiveLogFileName = activeLogFileName_ + "." + std::to_string(logFileNum);
            curTask_ = LogInfo(LogType::NONE, Log::logger, __func__,
                              "Copying rotating log file " + activeLogFileName_ + " to " + archiveLogFileName);
            logFileInternal();
            logFile_.close();
            if (logFileLimit_ != 1) { // If the limit is 1, then we keep only the active one
              try {
                std::filesystem::copy(activeLogFileName_, archiveLogFileName,
                                      std::filesystem::copy_options::overwrite_existing);
              } catch (const std::filesystem::filesystem_error& e) {
                std::cout << "ERROR: Failed to copy rotating log file " << activeLogFileName_
                          << " to " << archiveLogFileName << "(" << e.what() << ")" << std::endl;
              }
              // Handle log file limit (only guaranteed to work if you don't change the limit after boot)
              if (logFileLimit_ > 0) {
                int oldestLogFileNum = logFileNum - logFileLimit_ + 1;
                if (oldestLogFileNum >= 0) {
                  std::string oldLogFileName = activeLogFileName_ + "." + std::to_string(oldestLogFileNum);
                  try {
                    std::filesystem::remove(oldLogFileName);
                  } catch (const std::filesystem::filesystem_error& e) {
                    std::cout << "ERROR: Failed to remove old log file " << oldLogFileName
                              << "(" << e.what() << ")" << std::endl;
                  }
                }
              }
            }
          }
          logLineCount = 0;
          ++logFileNum;
          logFile_.open(activeLogFileName_, std::ios::out | std::ios::trunc);
          std::string nextArchiveLogFileName = activeLogFileName_ + "." + std::to_string(logFileNum);
          curTask_ = LogInfo(LogType::NONE, Log::logger, __func__,
                    "Starting rotating log file #" + std::to_string(logFileNum)
                    + " (will later be archived to " + nextArchiveLogFileName + ")");
          logFileInternal();
        }
        // Wait until there's a task in the queue or stopWorker_ is true.
        {
          std::unique_lock<std::mutex> lock(logQueueMutex_);
          cv_.wait(lock, [this] { return !logQueue_.empty() || stopWorker_; });
          if (stopWorker_) return;  // If stopWorker_ is true, return.
          curTask_ = std::move(logQueue_.front());
          logQueue_.pop();
        }
        logFileInternal();
        ++logLineCount;
      }
    };

    /**
     * Log something to the debug file.
     * Does not consume a parameter as it will use the `curTask_` variable.
     */
    void logFileInternal() {
      std::string logType = "";
      switch (curTask_.getType()) {
        case LogType::TRACE: logType = "TRACE"; break;
        case LogType::DEBUG: logType = "DEBUG"; break;
        case LogType::INFO: logType = "INFO"; break;
        case LogType::WARNING: logType = "WARNING"; break;
        case LogType::ERROR: logType = "ERROR"; break;
        case LogType::NONE: logType = "NONE"; break;
        default: logType = "INVALID_LOG_TYPE"; break;
      }
      this->logFile_ << "[" << getCurrentTimestamp() << " " << logType << "] "
        << curTask_.getLogSrc() << "::" << curTask_.getFunc()
        << " - " << curTask_.getMessage() << std::endl;
    };

    /// Post a task to the queue.
    void postLogTask(LogInfo&& infoToLog) noexcept {
      {
        std::unique_lock<std::mutex> lock(logQueueMutex_);
        logQueue_.emplace(std::move(infoToLog));
      }
      cv_.notify_one();
    };

  public:

    /**
     * Set the log level.
     * @param logLevel The log level.
     */
    static inline void setLogLevel(LogType logLevel) {
      getInstance().logLevel_ = logLevel;
    }

    /**
     * Get the log level.
     * @return The current log level.
     */
    static inline LogType getLogLevel() {
      return getInstance().logLevel_;
    }

    /**
     * Set the log line limit for each rotating log file.
     * @param logLineLimit Number of lines in bdk.log before it is archived and reset.
     */
    static inline void setLogLineLimit(int logLineLimit) {
      getInstance().logLineLimit_ = logLineLimit;
    }

    /**
     * Set the log file limit for each rotating log file.
     * @param logLineLimit Maximum number of log files to keep (0 = no limit).
     */
    static inline void setLogFileLimit(int logFileLimit) {
      getInstance().logFileLimit_ = logFileLimit;
    }

    /**
     * Log debug data to the debug file.
     * @param infoToLog The data to log.
     */
    static inline void logToDebug(LogInfo&& infoToLog) noexcept {
      if (infoToLog.getType() < getInstance().logLevel_) return;
      getInstance().postLogTask(std::move(infoToLog));
    }

    /**
     * Log debug data to the debug file.
     * @param type The type of the log.
     * @param logSrc The source of the log.
     * @param func The function name.
     * @param message The message to log.
     */
    static inline void logToDebug(
      LogType type, const std::string& logSrc, std::string&& func, std::string&& message
    ) noexcept {
      if (type < getInstance().logLevel_) return;
      auto log = LogInfo(type, logSrc, std::move(func), std::move(message));
      getInstance().postLogTask(std::move(log));
    }

    /// Destructor.
    ~Logger() {
      stopWorker_ = true;
      cv_.notify_one();
      logThreadFuture_.get();
      // Flush the remaining queue
      while (!logQueue_.empty()) {
        curTask_ = std::move(logQueue_.front());
        logQueue_.pop();
        logFileInternal();
      }
    }

    /// Get the current timestamp as a string in the "%Y-%m-%d %H:%M:%S.ms" format.
    static inline std::string getCurrentTimestamp() {
      auto now = std::chrono::system_clock::now();
      auto itt = std::chrono::system_clock::to_time_t(now);
      std::ostringstream ss;
      ss << std::put_time(gmtime(&itt), "%Y-%m-%d %H:%M:%S");
      auto millisec = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
      ss << '.' << std::setfill('0') << std::setw(3) << millisec.count();
      return ss.str();
    }
};

#endif // LOGGER_H
