#ifndef BTVSERVER_UTILS_H
#define BTVSERVER_UTILS_H

#include "../../../libs/json.hpp"
#include "../../../utils/strings.h"
#include <future>
#include <functional>
#include <boost/asio/io_context.hpp>
#include <boost/algorithm/string.hpp>
#include <memory>

#include "utils/utils.h"
#include "utils/tx.h"
#include <utils/hex.h>

namespace BTVServer {

  void fail(const std::string& class_, boost::system::error_code ec, const std::string& what);

  template <typename T>
  json makeRequestMethod(const std::string& method, const T& params, const uint64_t& id = 1) {
    return json({
      {"jsonrpc", "2.0"},
      {"id", id},
      {"method", method},
      {"params", params}
    });
  }

  class Printer {
  private:
    std::mutex printMutex;
    std::unique_ptr<std::deque<std::string>> printQueue;
    std::future<void> printerFuture;
    bool run = true;

    Printer() {
      printerFuture = std::async(std::launch::async, &Printer::print, this);
    }

    ~Printer() {
      run = false;
      printerFuture.get();
    }

    void print() {
      while(run) {
        std::unique_ptr<std::deque<std::string>> toPrint;
        {
          std::lock_guard<std::mutex> lock(this->printMutex);
          if (this->printQueue == nullptr) {
            // Do absolutely nothing
          } else {
            toPrint = std::move(this->printQueue);
            this->printQueue = nullptr;
          }
        }
        if (toPrint != nullptr) {
          for (const auto& str : *toPrint) {
            std::cout << str << std::endl;
          }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  public:
    static void safePrint (std::string&& str) {
      static Printer printer;
      std::lock_guard<std::mutex> lock(printer.printMutex);
      if (printer.printQueue == nullptr) {
        printer.printQueue = std::make_unique<std::deque<std::string>>();
      }
      printer.printQueue->emplace_back(std::move(str));
    }
  };
};

#endif // BTVSERVER_UTILS_H