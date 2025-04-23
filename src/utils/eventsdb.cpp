#include "eventsdb.h"
#include <sstream>

namespace {

template<typename T>
T blobTo(const void *data) {
  T target;
  std::memcpy(target.data(), data, target.size());
  return target;
}

Bytes blobToBytes(const void *data, const size_t size) {
  return Bytes(reinterpret_cast<const Byte*>(data), reinterpret_cast<const Byte*>(data) + size);
}

std::vector<Hash> blobToTopics(const void *data, const size_t size) {
  std::vector<Hash> topics;

  const Byte *start = reinterpret_cast<const Byte*>(data);
  const Byte *end = start + size;

  while (start < end) {
    topics.emplace_back(blobTo<Hash>(start));
    start += 32;
  }

  return topics;
}

int64_t getLogIndex(const SQLite::Database& db, int64_t blockNumber) {
  SQLite::Statement statement(db, "SELECT MAX(event_index), COUNT() FROM events WHERE block_number = ?");
  statement.bind(1, blockNumber);

  int64_t logIndex = 0;
  
  if (statement.executeStep()) {
    logIndex = (statement.getColumn(1).getUInt() > 0) ? (statement.getColumn(0).getUInt() + 1) : 0;
  }

  return logIndex;
}

SQLite::Database makeDatabase(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    std::filesystem::create_directories(path);
  }

  return SQLite::Database(std::string(path) + "events.db3", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
}

} // namespace

EventsDB::EventsDB(const std::filesystem::path& path) : db_(makeDatabase(path)) {
  std::string_view createEventsStatement =
    "CREATE TABLE IF NOT EXISTS events ("
    " address BLOB,"
    " event_index INTEGER,"
    " block_number INTEGER,"
    " block_hash BLOB,"
    " tx_index INTEGER,"
    " tx_hash BLOB,"
    " data BLOB,"
    " topic_0 BLOB,"
    " topic_1 BLOB,"
    " topic_2 BLOB,"
    " topic_3 BLOB)";

  db_.exec(createEventsStatement.data());
  db_.exec("CREATE INDEX IF NOT EXISTS block_number_index ON events (block_number)");
  db_.exec("CREATE INDEX IF NOT EXISTS block_hash_index ON events (block_hash)");
  db_.exec("CREATE INDEX IF NOT EXISTS address_index ON events (address)");
  db_.exec("CREATE INDEX IF NOT EXISTS tx_index_index ON events (tx_index)");
  db_.exec("CREATE INDEX IF NOT EXISTS topic_0_index ON events (topic_0)");
  db_.exec("CREATE INDEX IF NOT EXISTS topic_1_index ON events (topic_1)");
  db_.exec("CREATE INDEX IF NOT EXISTS topic_2_index ON events (topic_2)");
  db_.exec("CREATE INDEX IF NOT EXISTS topic_3_index ON events (topic_3)");
}

void EventsDB::putEvent(const Event& event) {
  SQLite::Statement query(db_, "INSERT INTO events VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

  query.bind(1, event.getAddress().data(), event.getAddress().size());
  query.bind(2, getLogIndex(db_, event.getBlockIndex()));
  query.bind(3, static_cast<int64_t>(event.getBlockIndex()));
  query.bind(4, event.getBlockHash().data(), event.getBlockHash().size());
  query.bind(5, static_cast<int64_t>(event.getTxIndex()));
  query.bind(6, event.getTxHash().data(), event.getTxHash().size());
  query.bind(7, event.getData().data(), event.getData().size());

  const auto& topics = event.getTopics();

  for (int i = 0; i < 4; i++) {
    if (i >= topics.size()) {
      query.bind(8 + i); // binds null
    } else {
      query.bind(8 + i, topics[i].data(), topics[i].size());
    }
  }

  query.exec();
}

std::vector<Event> EventsDB::getEvents(const EventsDB::Filters& filters) const {
  std::stringstream query;

  query << "SELECT address, event_index, block_number, block_hash, tx_index,"
           "       tx_hash, data, topic_0, topic_1, topic_2, topic_3"
           "  FROM events";

  auto whereOrAnd = [first = true] () mutable -> std::string_view {
    if (first) {
      first = false;
      return " WHERE";
    }

    return " AND";
  };

  if (filters.address.has_value()) {
    query << whereOrAnd() << " address = ?";
  }

  if (filters.fromBlock.has_value()) {
    query << whereOrAnd() << " block_number >= ?";
  }

  if (filters.toBlock.has_value()) {
    query << whereOrAnd() << " block_number <= ?";
  }

  if (filters.blockHash.has_value()) {
    query << whereOrAnd() << " block_hash = ?";
  }

  if (filters.txIndex.has_value()) {
    query << whereOrAnd() << " tx_index = ?";
  }

  for (int i = 0; i < filters.topics.size(); i++) {
    const size_t count = filters.topics[i].size();

    if (count == 0) {
      continue;
    }

    query << whereOrAnd() << " topic_" << i << " IN (?";

    for (int j = 0; j < count - 1; j++) {
      query << ", ?";
    }

    query << ")";
  }

  query << " ORDER BY block_number, event_index";

  SQLite::Statement statement(db_, query.str());
  unsigned count = 1;

  if (filters.address.has_value()) {
    statement.bind(count++, filters.address.value().data(), filters.address.value().size());
  }

  if (filters.fromBlock.has_value()) {
    statement.bind(count++, filters.fromBlock.value());
  }

  if (filters.toBlock.has_value()) {
    statement.bind(count++, filters.toBlock.value());
  }

  if (filters.blockHash.has_value()) {
    statement.bind(count++, filters.blockHash.value().data(), filters.blockHash.value().size());
  }

  if (filters.txIndex.has_value()) {
    statement.bind(count++, filters.txIndex.value());
  }

  for (int i = 0; i < filters.topics.size(); i++) {
    for (int j = 0; j < filters.topics[i].size(); j++) {
      statement.bind(count++, filters.topics[i][j].data(), filters.topics[i][j].size());
    }
  }

  std::vector<Event> events;

  while (statement.executeStep()) {
    auto address = blobTo<Address>(statement.getColumn(0).getBlob());
    auto eventIndex = statement.getColumn(1).getUInt();
    auto blockNumber = statement.getColumn(2).getUInt();
    auto blockHash = blobTo<Hash>(statement.getColumn(3).getBlob());
    auto txIndex = statement.getColumn(4).getUInt();
    auto txHash = blobTo<Hash>(statement.getColumn(5).getBlob());
    auto data = blobToBytes(statement.getColumn(6).getBlob(), statement.getColumn(6).getBytes());

    std::vector<Hash> topics;

    for (int i = 7; i < 11; i++) {
      auto column = statement.getColumn(i);

      if (column.isNull()) {
        break;
      }

      Hash& topic = topics.emplace_back();
      std::memcpy(topic.data(), column.getBlob(), column.getBytes());
    }

    events.emplace_back(Event(eventIndex, txHash, txIndex, blockHash, blockNumber, address, std::move(data), std::move(topics), false));
  }

  return events;
}

std::unique_ptr<SQLite::Transaction> EventsDB::transaction() { return std::make_unique<SQLite::Transaction>(db_); }
