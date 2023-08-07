#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <queue>
#include <condition_variable>
#include <future>

enum class LogType {
  DEBUG,
  INFO,
  WARNING,
  ERROR
};

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
}

class LogInfo {
  private:
    /// Log type.
    LogType type_;
    /// Log source.
    std::string logSrc_;
    /// Function name.
    std::string func_;
    /// Message to log.
    std::string message_;

public:
  /// Default constructor.
  LogInfo() : type_(LogType::DEBUG), func_(""), logSrc_(""), message_("") {};

  /// Constructor.
  LogInfo(LogType type, const std::string& logSrc, std::string&& func, std::string&& message) :
    type_(type), logSrc_(logSrc), func_(std::move(func)), message_(std::move(message)) {};

  /// Default destructor.
  ~LogInfo() {};

  /// Move constructor
  LogInfo(LogInfo&& other) noexcept :
    type_(other.type_), func_(std::move(other.func_)), logSrc_(std::move(other.logSrc_)), message_(std::move(other.message_)) {};

  /// Move assign operator
  LogInfo& operator=(LogInfo&& other) noexcept {
    this->type_ = other.type_;
    this->func_ = std::move(other.func_);
    this->logSrc_ = std::move(other.logSrc_);
    this->message_ = std::move(other.message_);
    return *this;
  }

  /// Get Type
  inline const LogType& getType() const noexcept { return type_; };

  /// Get Log Source
  inline const std::string& getLogSrc() const noexcept { return logSrc_; };

  /// Get Function
  inline const std::string& getFunc() const noexcept { return func_; };

  /// Get Message
  inline const std::string& getMessage() const noexcept { return message_; };
};


class Logger {
  private:
    /// Private constructor as it is a singleton.
    Logger() : logFile_("debug.log", std::ios::out | std::ios::app) {
      logThreadFuture_ = std::async(std::launch::async, &Logger::logger, this);
    }
    /// Make it non-copyable
    Logger(const Logger&) = delete;
    /// Make it non-assignable.
    Logger& operator=(const Logger&) = delete;
    /// Get the instance.
    static Logger& getInstance() {
      static Logger instance;
      return instance;
    };
    /// The file stream.
    std::ofstream logFile_;
    /// Mutex for protecting access to the log queue.
    std::mutex logQueueMutex_;
    /// Conditional variable to wait for new tasks.
    std::condition_variable cv_;
    /// Queue for the log tasks.
    std::queue<LogInfo> logQueue_;
    /// Current task being executed.
    LogInfo currentTask_;
    /// Flag for stopping the thread.
    std::atomic<bool> stopWorker_ = false;
    /// Future object that will be used to wait for the log thread to finish.
    std::future<void> logThreadFuture_;
    /// Function for the future object.
    void logger() {
      while (true) {
        std::unique_lock<std::mutex> lock(logQueueMutex_);
        // Wait until there's a task in the queue or stopWorker_ is true.
        cv_.wait(lock, [this] { return !logQueue_.empty() || stopWorker_; });
        // If stopWorker_ is true, return.
        if (stopWorker_) {
          return;
        }
        currentTask_ = std::move(logQueue_.front());
        logQueue_.pop();
        lock.unlock();
        logFileInternal();
      }
    };
    /**
     * Log something to the debug file
     * It doesnt consume a parameter as it will use the currentTask_ variable
     */
    void logFileInternal() {
      switch (currentTask_.getType()) {
        case LogType::DEBUG:
          logFile_ << "[" << getCurrentTimestamp() << " DEBUG] " << currentTask_.getLogSrc() << "::" << currentTask_.getFunc() << " - " << currentTask_.getMessage() << std::endl;
          break;
        case LogType::INFO:
          logFile_ << "[" << getCurrentTimestamp() << " INFO] " << currentTask_.getLogSrc() << "::" << currentTask_.getFunc() << " - " << currentTask_.getMessage() << std::endl;
          break;
        case LogType::WARNING:
          logFile_ << "[" << getCurrentTimestamp() << " WARNING] " << currentTask_.getLogSrc() << "::" << currentTask_.getFunc() << " - " << currentTask_.getMessage() << std::endl;
          break;
        case LogType::ERROR:
          logFile_ << "[" << getCurrentTimestamp() << " ERROR] " << currentTask_.getLogSrc() << "::" << currentTask_.getFunc() << " - " << currentTask_.getMessage() << std::endl;
          break;
      }
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
    /// Post the log task to the queue.
    static inline void logToDebug(LogInfo&& infoToLog) noexcept {
      getInstance().postLogTask(std::move(infoToLog));
    }

    /// Create and post the log task to the queue.
    static inline void logToDebug(LogType type, const std::string& logSrc, std::string&& func, std::string&& message) noexcept {
      LogInfo log = LogInfo(type, logSrc, std::move(func), std::move(message));
      getInstance().postLogTask(std::move(log));
    }

    ~Logger() {
      stopWorker_ = true;
      cv_.notify_one();
      logThreadFuture_.get();
      /// Flush the remaining queue
      while (!logQueue_.empty()) {
        currentTask_ = std::move(logQueue_.front());
        logQueue_.pop();
        logFileInternal();
      }
    }

    /**
     * Get the current timestamp as string in the following format:
     * "%Y-%m-%d %H:%M:%S.ms"
     * "%Y-%m-%d %H:%M:%S.ms"
     */
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
