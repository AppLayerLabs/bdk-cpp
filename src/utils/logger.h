/*
Copyright (c) [2023-2024] [Sparq Network]

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
enum class LogType { DEBUG, INFO, WARNING, ERROR };

/// Namespace with string prefixes for each blockchain module, for printing log/debug messages.
namespace Log {
  const std::string blockchain = "Blockchain";                     ///< String for `Blockchain`.
  const std::string storage = "Storage";                           ///< String for `Storage`.
  const std::string snowmanVM = "SnowmanVM";                       ///< String for `SnowmanVM`.
  const std::string block = "Block";                               ///< String for `Block`.
  const std::string db = "DB";                                     ///< String for `DB`.
  const std::string state = "State";                               ///< String for `State`.
  const std::string grpcServer = "gRPCServer";                     ///< String for `gRPCServer`.
  const std::string grpcClient = "gRPCClient";                     ///< String for `gRPCClient`.
  const std::string utils = "Utils";                               ///< String for `Utils`.
  const std::string httpServer = "HTTPServer";                     ///< String for `HTTPServer`.
  const std::string JsonRPCEncoding = "JsonRPC::Encoding";         ///< String for `JsonRPC::Encoding`.
  const std::string JsonRPCDecoding = "JsonRPC::Decoding";         ///< String for `JsonRPC::Decoding`.
  const std::string rdPoS = "rdPoS";                               ///< String for `rdPoS`.
  const std::string ABI = "ABI";                                   ///< String for `ABI`.
  const std::string P2PSession = "P2P::Session";                   ///< String for `P2P::Session`.
  const std::string P2PClientFactory = "P2P::ClientFactory";       ///< String for `P2P::ClientFactory`.
  const std::string P2PServer = "P2P::Server";                     ///< String for `P2P::Server`.
  const std::string P2PServerListener = "P2P::ServerListener";     ///< String for `P2P::ServerListener`.
  const std::string P2PManager = "P2P::Manager";                   ///< String for `P2P::Manager`.
  const std::string P2PParser = "P2P::Parser";                     ///< String for `P2P::Parser`.
  const std::string P2PRequestEncoder = "P2P::RequestEncoder";     ///< String for `P2P::RequestEncoder`.
  const std::string P2PRequestDecoder = "P2P::RequestDecoder";     ///< String for `P2P::RequestDecoder`.
  const std::string P2PResponseEncoder = "P2P::AnswerDecoder";     ///< String for `P2P::ResponseEncoder`.
  const std::string P2PResponseDecoder = "P2P::AnswerEncoder";     ///< String for `P2P::ResponseDecoder`.
  const std::string P2PBroadcastEncoder = "P2P::BroadcastEncoder"; ///< String for `P2P::BroadcastEncoder`.
  const std::string P2PDiscoveryWorker = "P2P::DiscoveryWorker";   ///< String for `P2P::DiscoveryWorker`.
  const std::string contractManager = "ContractManager";           ///< String for `ContractManager`.
  const std::string syncer = "Syncer";                             ///< String for `Syncer`.
  const std::string event = "Event";                               ///< String for `Event`.
  const std::string contractHost = "ContractHost";                 ///< String for `ContractHost`.
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
    Logger() : logFile_("debug.log", std::ios::out | std::ios::app) {
      logThreadFuture_ = std::async(std::launch::async, &Logger::logger, this);
    }
    Logger(const Logger&) = delete;             ///< Make it non-copyable
    Logger& operator=(const Logger&) = delete;  ///< Make it non-assignable.

    /// Get the instance.
    static Logger& getInstance() { static Logger instance; return instance; }

    std::ofstream logFile_;                 ///< The file stream.
    std::mutex logQueueMutex_;              ///< Mutex for protecting access to the log queue.
    std::condition_variable cv_;            ///< Conditional variable to wait for new tasks.
    std::queue<LogInfo> logQueue_;          ///< Queue for the log tasks.
    LogInfo curTask_;                       ///< Current task being executed.
    std::atomic<bool> stopWorker_ = false;  ///< Flag for stopping the thread.
    std::future<void> logThreadFuture_;     ///< Future object used to wait for the log thread to finish.

    /// Function for the future object.
    void logger() {
      while (true) {
        std::unique_lock<std::mutex> lock(logQueueMutex_);
        // Wait until there's a task in the queue or stopWorker_ is true.
        cv_.wait(lock, [this] { return !logQueue_.empty() || stopWorker_; });
        if (stopWorker_) return;  // If stopWorker_ is true, return.
        curTask_ = std::move(logQueue_.front());
        logQueue_.pop();
        lock.unlock();
        logFileInternal();
      }
    };

    /**
     * Log something to the debug file.
     * Does not consume a parameter as it will use the `curTask_` variable.
     */
    void logFileInternal() {
      std::string logType = "";
      switch (curTask_.getType()) {
        case LogType::DEBUG: logType = "DEBUG"; break;
        case LogType::INFO: logType = "INFO"; break;
        case LogType::WARNING: logType = "WARNING"; break;
        case LogType::ERROR: logType = "ERROR"; break;
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
     * Log debug data to the debug file.
     * @param infoToLog The data to log.
     */
    static inline void logToDebug(LogInfo&& infoToLog) noexcept {
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
