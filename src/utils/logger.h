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
#include <mutex>
#include <typeinfo>
#include <type_traits>
#include <sstream>
#include <source_location>
#include <boost/core/demangle.hpp>

/// Enum for the log message types.
enum class LogType { TRACE, DEBUG, INFO, WARNING, ERROR, NONE };

///@{
/// Internal helper macros for logging
#define GET_LOGICAL_LOCATION Log::getLogicalLocationIfAvailable(this, \
          std::is_base_of<Log::LogicalLocationProvider, \
          std::decay<decltype(*this)>::type>{})
#define INSTANCE_LOG_BASE(type, message) Logger::logToDebug(type, GET_LOGICAL_LOCATION, \
          Log::getMethodName<std::remove_pointer_t<decltype(this)>>(__func__), message);
#define GET_FILE_NAME_FROM_PATH (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define STATIC_LOG_BASE(type, message) Logger::logToDebug(type, GET_FILE_NAME_FROM_PATH, __func__, message);
#define GEN_LOG_BASE(type, message) Logger::logToDebug(type, GET_FILE_NAME_FROM_PATH, "L" + std::to_string(__LINE__), message);
///@}

///@{
/// Logging macros to be used with a `this` (non-static context)
#define LOGTRACE(message)   INSTANCE_LOG_BASE(LogType::TRACE, message);
#define LOGDEBUG(message)   INSTANCE_LOG_BASE(LogType::DEBUG, message);
#define LOGINFO(message)    INSTANCE_LOG_BASE(LogType::INFO, message);
#define LOGWARNING(message) INSTANCE_LOG_BASE(LogType::WARNING, message);
#define LOGERROR(message)   INSTANCE_LOG_BASE(LogType::ERROR, message);
///@}

///@{
/// Logging macros to be used with a `this` (non-static context) and that also Log::safePrint() the message.
#define LOGTRACEP(message)   { INSTANCE_LOG_BASE(LogType::TRACE, message); Log::safePrint(message); }
#define LOGDEBUGP(message)   { INSTANCE_LOG_BASE(LogType::DEBUG, message); Log::safePrint(message); }
#define LOGINFOP(message)    { INSTANCE_LOG_BASE(LogType::INFO, message); Log::safePrint(message); }
#define LOGWARNINGP(message) { INSTANCE_LOG_BASE(LogType::WARNING, message); Log::safePrint(message); }
#define LOGERRORP(message)   { INSTANCE_LOG_BASE(LogType::ERROR, message); Log::safePrint(message); }
///@}

///@{
/// Logging macros to be used in a static context (does not log class name, even if available)
#define SLOGTRACE(message)   STATIC_LOG_BASE(LogType::TRACE, message);
#define SLOGDEBUG(message)   STATIC_LOG_BASE(LogType::DEBUG, message);
#define SLOGINFO(message)    STATIC_LOG_BASE(LogType::INFO, message);
#define SLOGWARNING(message) STATIC_LOG_BASE(LogType::WARNING, message);
#define SLOGERROR(message)   STATIC_LOG_BASE(LogType::ERROR, message);
///@}

///@{
/// Logging macros to be used in a static context (does not log class name, even if available) and that also Log::safePrint() the message.
#define SLOGTRACEP(message)   { STATIC_LOG_BASE(LogType::TRACE, message); Log::safePrint(message); }
#define SLOGDEBUGP(message)   { STATIC_LOG_BASE(LogType::DEBUG, message); Log::safePrint(message); }
#define SLOGINFOP(message)    { STATIC_LOG_BASE(LogType::INFO, message); Log::safePrint(message); }
#define SLOGWARNINGP(message) { STATIC_LOG_BASE(LogType::WARNING, message); Log::safePrint(message); }
#define SLOGERRORP(message)   { STATIC_LOG_BASE(LogType::ERROR, message); Log::safePrint(message); }
///@}

///@{
/// Logging macros that omit the function name (to be used in generated functions)
#define GLOGTRACE(message)   GEN_LOG_BASE(LogType::TRACE, message);
#define GLOGDEBUG(message)   GEN_LOG_BASE(LogType::DEBUG, message);
#define GLOGINFO(message)    GEN_LOG_BASE(LogType::INFO, message);
#define GLOGWARNING(message) GEN_LOG_BASE(LogType::WARNING, message);
#define GLOGERROR(message)   GEN_LOG_BASE(LogType::ERROR, message);
///@}

///@{
/// Logging macros that omit the function name (to be used in generated functions) and that also Log::safePrint() the message.
#define GLOGTRACEP(message)   { GEN_LOG_BASE(LogType::TRACE, message); Log::safePrint(message); }
#define GLOGDEBUGP(message)   { GEN_LOG_BASE(LogType::DEBUG, message); Log::safePrint(message); }
#define GLOGINFOP(message)    { GEN_LOG_BASE(LogType::INFO, message); Log::safePrint(message); }
#define GLOGWARNINGP(message) { GEN_LOG_BASE(LogType::WARNING, message); Log::safePrint(message); }
#define GLOGERRORP(message)   { GEN_LOG_BASE(LogType::ERROR, message); Log::safePrint(message); }
///@}

/// Namespace with logging utilities
namespace Log {
  ///@{
  /** String for the given module. */
  const std::string snowmanVM = "SnowmanVM";
  const std::string P2PManager = "P2P::Manager";
  const std::string logger = "Logger";
  ///@}

  inline std::mutex __safePrintMutex; ///< Mutex for Log::safePrint

  inline std::atomic<bool> logToCout = false; ///< Indicates whether logging to stdout is allowed (for safePrint()).

  /**
   * Print a string to stdout if logToCout is enabled (e.g. not in a test).
   * @param str The string to print.
   */
  inline void safePrint(std::string_view str) {
    if (!logToCout) return;
    std::lock_guard lock(__safePrintMutex);
    std::cout << str << std::endl;
  };

  /**
   * Print a string to stdout, even if logToCout is disabled (e.g. even in a test).
   * @param str The string to print.
   */
  inline void safePrintTest(std::string_view str) {
    std::lock_guard lock(__safePrintMutex);
    std::cout << str << std::endl;
  }

  /**
   * Interface implemented by any object that wishes to provide a custom
   * logical logging location (socket address, peer ID, etc.) to the
   * `logSrc_` argument of `LogInfo`, as generated by the LOGxxx macros.
   */
  class LogicalLocationProvider {
  public:
    /**
     * Method that should be overriden by subclasses in order to provide
     * a custom `logSrc_` for their LOGxxx log messages.
     * @return A custom logical location string.
     */
    virtual std::string getLogicalLocation() const = 0;
  };

  /**
   * Get the address of the given pointer as an hex string.
   * @param ptr Pointer to return the address of.
   * @return The address of ptr as an hex string.
   */
  inline std::string pointerToHexString(const void* ptr) {
    std::ostringstream oss;
    oss << std::hex << ptr;
    return oss.str();
  }

  /**
   * Get the ID of the current thread as a string.
   * @return The ID of the current thread as a string.
   */
  inline std::string getThreadIdAsString() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
  }

  /// LOG macro provider of logical location when the class of `this` extends `LogicalLocationProvider`.
  template <typename T>
  std::string getLogicalLocationIfAvailable(T* obj, std::true_type) {
    return obj->getLogicalLocation();
  }

  /// LOG macro provider of a default logical location (thread ID + value of `this` pointer).
  template <typename T>
  std::string getLogicalLocationIfAvailable(T* obj, std::false_type) {
    return "{" + getThreadIdAsString() + "," + pointerToHexString(obj) + "}";
  }

  /// Get a pretty "ClassName::MethodName" for the current `this` object.
  template<typename T>
  std::string getMethodName(const char* func) {
    return boost::core::demangle(typeid(T).name()) + "::" + std::string(func);
  }
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
      type_(type), logSrc_(logSrc.length() > 0 ? " " + logSrc : logSrc), func_(std::move(func)), message_(std::move(message))
    {}

    ~LogInfo() = default; ///< Default destructor.

    /// Move constructor
    LogInfo(LogInfo&& other) noexcept :
      type_(other.type_), func_(std::move(other.func_)),
      logSrc_(std::move(other.logSrc_)), message_(std::move(other.message_))
    {}

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
    std::atomic<bool> echoToCout_ = false;  ///< Flag for echoing all logging to stdout as well.

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
                Log::safePrint("ERROR: Failed to copy rotating log file " + activeLogFileName_
                               + " to " + archiveLogFileName + "(" + e.what() + ")");
              }
              // Handle log file limit (only guaranteed to work if you don't change the limit after boot)
              if (logFileLimit_ > 0) {
                int oldestLogFileNum = logFileNum - logFileLimit_ + 1;
                if (oldestLogFileNum >= 0) {
                  std::string oldLogFileName = activeLogFileName_ + "." + std::to_string(oldestLogFileNum);
                  try {
                    std::filesystem::remove(oldLogFileName);
                  } catch (const std::filesystem::filesystem_error& e) {
                    Log::safePrint("ERROR: Failed to remove old log file " + oldLogFileName
                                   + "(" + e.what() + ")");
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
    }

    /**
     * Log something to the debug file.
     * Does not consume a parameter as it will use the `curTask_` variable.
     */
    void logFileInternal() {
      std::string logType = "";
      switch (curTask_.getType()) {
        case LogType::TRACE:   logType = "TRA"; break;
        case LogType::DEBUG:   logType = "DBG"; break;
        case LogType::INFO:    logType = "INF"; break;
        case LogType::WARNING: logType = "WAR"; break;
        case LogType::ERROR:   logType = "ERR"; break;
        case LogType::NONE:    logType = "SYS"; break;
        default:               logType = "BAD"; break;
      }
      this->logFile_
        << "["
        << getCurrentTimestamp()
        << " "
        << logType
        //<< " " // logSrc has to include its own left padding because it can be ommitted ("")
        << curTask_.getLogSrc()
        << " "
        << curTask_.getFunc()
        << "] "
        << curTask_.getMessage()
        << std::endl;
      if (this->echoToCout_) {
        // This is only enabled during specific debugging scenarios, so don't
        //   affect the (faster) streaming directly to the logFile_ above.
        Log::safePrintTest(
            "["
          + getCurrentTimestamp()
          + " "
          + logType
          //+ " "  // logSrc has to include its own left padding because it can be ommitted ("")
          + curTask_.getLogSrc()
          + " "
          + curTask_.getFunc()
          + "] "
          + curTask_.getMessage()
        );
      }
    };

    /// Post a task to the queue.
    void postLogTask(LogInfo&& infoToLog) noexcept {
      {
        std::unique_lock<std::mutex> lock(logQueueMutex_);
        logQueue_.emplace(std::move(infoToLog));
      }
      cv_.notify_one();
    }

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
     * Toggle echoing to stdout.
     * @param echoToCout `true` to enable echoing to stdout, `false` to disable.
     */
    static inline void setEchoToCout(bool echoToCout) {
      getInstance().echoToCout_ = echoToCout;
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
