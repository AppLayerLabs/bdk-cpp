#ifndef BDK_EVENTSDB_H
#define BDK_EVENTSDB_H

#include "utils.h"
#include "contract/event.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include <boost/algorithm/string/case_conv.hpp>

class EventsDB {
public:
  struct Filters {
    std::optional<int64_t> fromBlock;
    std::optional<int64_t> toBlock;
    std::optional<Hash> blockHash;
    std::optional<Address> address;
    std::optional<int64_t> txIndex;
    std::vector<std::vector<Hash>> topics;
  };

  explicit EventsDB(const std::filesystem::path& path);

  std::vector<Event> getEvents(const Filters& filters) const;

  void putEvent(const Event& event);

  SQLite::Transaction transaction();

private:
  SQLite::Database db_;
};

#endif // BDK_EVENTSDB_H
