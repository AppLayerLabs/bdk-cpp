#include "eventsdb.h"
#include <sstream>

namespace {

template<typename T>
T blobTo(const void *data) {
  T target;
  std::memcpy(target.data(), data, target.size());
  return target;
}

inline Bytes blobToBytes(const void *data, const size_t size) {
  return Bytes(reinterpret_cast<const Byte*>(data), reinterpret_cast<const Byte*>(data) + size);
}

inline std::vector<Hash> blobToTopics(const void *data, const size_t size) {
  std::vector<Hash> topics;

  const Byte *start = reinterpret_cast<const Byte*>(data);
  const Byte *end = start + size;

  while (start < end) {
    topics.emplace_back(blobTo<Hash>(start));
    start += 32;
  }

  return topics;
}

} // namespace

// TODO: use path
EventsDB::EventsDB(const std::filesystem::path& path)
  : db_("example.db3", SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE) {

  std::string_view createEventsStatement =
    "CREATE TABLE IF NOT EXISTS events ("
    " id INTEGER PRIMARY KEY AUTOINCREMENT,"
    " address BLOB,"
    " event_index INTEGER,"
    " block_number INTEGER,"
    " block_hash BLOB,"
    " tx_index INTEGER,"
    " tx_hash BLOB,"
    " data BLOB)";

  std::string_view createTopicsStatement = 
    "CREATE TABLE IF NOT EXISTS topics ("
    " event_id INTEGER,"
    " topic_index INTEGER,"
    " topic_value BLOB,"
    " FOREIGN KEY(event_id) REFERENCES events(id))";

  std::string_view topicsIndexStatement = "CREATE INDEX IF NOT EXISTS topics_index ON topics (topic_index, topic_value)";
  std::string_view blockNumberIndexStatement = "CREATE INDEX IF NOT EXISTS block_number_index ON events (block_number)";

  db_.exec(createEventsStatement.data());
  db_.exec(createTopicsStatement.data());
  db_.exec(topicsIndexStatement.data());
  db_.exec(blockNumberIndexStatement.data());
}

void EventsDB::putEvent(const Event& event) {
  SQLite::Transaction transaction(db_);
  SQLite::Statement query(db_, "INSERT INTO events (address, event_index, block_number, block_hash, tx_index, tx_hash, data) VALUES (?, ?, ?, ?, ?, ?, ?) RETURNING id");

  query.bind(1, event.getAddress().data(), event.getAddress().size());
  query.bind(2, static_cast<int64_t>(event.getLogIndex()));
  query.bind(3, static_cast<int64_t>(event.getBlockIndex()));
  query.bind(4, event.getBlockHash().data(), event.getBlockHash().size());
  query.bind(5, static_cast<int64_t>(event.getTxIndex()));
  query.bind(6, event.getTxHash().data(), event.getTxHash().size());
  query.bind(7, event.getData().data(), event.getData().size());

  if (!query.executeStep()) {
    throw std::runtime_error("failed to execute insert statement on events database");
  }

  const unsigned int id = query.getColumn(0).getUInt();

  query = SQLite::Statement(db_, "INSERT INTO topics VALUES (?, ?, ?)");

  const auto& topics = event.getTopics();

  for (int i = 0; i < topics.size(); i++) {
    query.bind(1, id);
    query.bind(2, i);
    query.bind(3, topics[i].data(), topics[i].size());
    query.exec();
    query.reset();
  }

  transaction.commit();
}

std::vector<Event> EventsDB::getEvents(const EventsDB::Filters& filters) const {
  std::stringstream query;

  query << "SELECT address, event_index, block_number, block_hash, tx_index, tx_hash, data, GROUP_CONCAT(topic_value)"
           "  FROM events e"
           "  JOIN topics t"
           "    ON e.id = t.event_id";

  auto whereOrAnd = [first = bool(true)] () mutable -> std::string_view {
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

  for (int i = 0; i < filters.topics.size(); i++) {
    const std::vector<Hash>& topicFilter = filters.topics[i];

    if (topicFilter.empty()) {
      continue;
    }

    if (topicFilter.size() == 1) {
      query << whereOrAnd() << " topic_index = ? AND topic_value = ?";
      continue;
    }

    query << whereOrAnd() << "(";

    for (int j = 0; j < topicFilter.size(); j++) {
      if (j != 0) {
        query << " or";
      }

      query << "(topic_index = ? AND topic_value = ?)";
    }

    query << ")";
  }

  query << " GROUP BY address, event_index, block_number, tx_index";

  SQLite::Statement statement(db_, query.str());
  int count = 1;

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

  for (int i = 0; i < filters.topics.size(); i++) {
    const std::vector<Hash>& topicFilter = filters.topics[i];

    for (int j = 0; j < topicFilter.size(); j++) {
      statement.bind(count++, j);
      statement.bind(count++, topicFilter.data(), topicFilter.size());
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
    auto topics = blobToTopics(statement.getColumn(7).getBlob(), statement.getColumn(7).getBytes());

    events.emplace_back(Event(eventIndex, txHash, txIndex, blockHash, blockNumber, address, std::move(data), std::move(topics), false));
  }

  return events;
}
