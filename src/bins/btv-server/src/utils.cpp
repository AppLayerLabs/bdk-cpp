#include "utils.h"

namespace BTVServer {
  void fail(const std::string& class_, boost::system::error_code ec, const std::string& what) {
    Printer::safePrint(class_ + "::fail: " + what + ": " + ec.what());
  }
}