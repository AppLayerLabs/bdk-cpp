/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "db.h"

#include "dynamicexception.h"

DB::DB(const std::filesystem::path& path, bool compress) {
  this->opts_.create_if_missing = true; // Create database folder in disk if it doesn't exist
  if (compress) this->opts_.compression = rocksdb::kLZ4Compression; // Activate LZ4 compression for database entries if requested
  if (!std::filesystem::exists(path)) { // Ensure the database path can actually be found
    std::filesystem::create_directories(path);
  }
  auto status = rocksdb::DB::Open(this->opts_, path, &this->db_);
  if (!status.ok()) {
    LOGERROR("Failed to open DB: " + status.ToString());
    throw DynamicException("Failed to open DB: " + status.ToString());
  }
}

bool DB::putBatch(const DBBatch& batch) {
  std::lock_guard lock(this->batchLock_);
  rocksdb::WriteBatch wb;
  for (const auto& dels : batch.getDels())
    wb.Delete(rocksdb::Slice(reinterpret_cast<const char*>(dels.data()), dels.size()));
  for (const auto& puts : batch.getPuts())
    wb.Put(rocksdb::Slice(reinterpret_cast<const char*>(puts.key.data()), puts.key.size()),
           rocksdb::Slice(reinterpret_cast<const char*>(puts.value.data()), puts.value.size()));
  rocksdb::Status s = this->db_->Write(rocksdb::WriteOptions(), &wb);
  return s.ok();
}

std::vector<DBEntry> DB::getBatch(
  const Bytes& bytesPfx, const std::vector<Bytes>& keys
) const {
  std::lock_guard lock(this->batchLock_);
  std::vector<DBEntry> ret;
  std::unique_ptr<rocksdb::Iterator> it(this->db_->NewIterator(rocksdb::ReadOptions()));
  rocksdb::Slice pfx(reinterpret_cast<const char*>(bytesPfx.data()), bytesPfx.size());

  // Search for all entries
  if (keys.empty()) {
    for (it->Seek(pfx); it->Valid() && it->key().starts_with(pfx); it->Next()) {
      auto keySlice = it->key();
      keySlice.remove_prefix(pfx.size());
      ret.emplace_back(Bytes(keySlice.data(), keySlice.data() + keySlice.size()), Bytes(it->value().data(), it->value().data() + it->value().size()));
    }
    it.reset();
    return ret;
  }
  // Search for specific entries from keys
  for (const auto& key : keys) {
    Bytes newPfx = bytesPfx;
    Utils::appendBytes(newPfx, key);
    rocksdb::Slice newPfxSlice(reinterpret_cast<const char*>(newPfx.data()), newPfx.size());
    for (it->Seek(newPfxSlice); it->Valid() && it->key().starts_with(newPfxSlice); it->Next()) {
      auto keySlice = it->key();
      keySlice.remove_prefix(newPfx.size());
      ret.emplace_back(Bytes(keySlice.data(), keySlice.data() + keySlice.size()), Bytes(it->value().data(), it->value().data() + it->value().size()));
    }
  }
  it.reset();
  return ret;
}

std::vector<Bytes> DB::getKeys(const Bytes& pfx, const Bytes& start, const Bytes& end) const {
  std::vector<Bytes> ret;
  std::unique_ptr<rocksdb::Iterator> it(this->db_->NewIterator(rocksdb::ReadOptions()));
  Bytes startBytes = pfx;
  Bytes endBytes = pfx;
  if (!start.empty()) Utils::appendBytes(startBytes, start);
  if (!end.empty()) Utils::appendBytes(endBytes, end);
  rocksdb::Slice startSlice(reinterpret_cast<const char*>(startBytes.data()), startBytes.size());
  rocksdb::Slice endSlice(reinterpret_cast<const char*>(endBytes.data()), endBytes.size());
  for (it->Seek(startSlice); it->Valid(); it->Next()) {
    if (this->opts_.comparator->Compare(it->key(), endSlice) > 0) {
      if (!it->key().starts_with(endSlice)) break;
    }
    rocksdb::Slice keySlice = it->key();
    keySlice.remove_prefix(pfx.size());
    ret.emplace_back(keySlice.data(), keySlice.data() + keySlice.size());
  }
  it.reset();
  return ret;
}

Bytes DB::getLastByPrefix(const Bytes& pfx) const {
  std::unique_ptr<rocksdb::Iterator> it(this->db_->NewIterator(rocksdb::ReadOptions()));
  Bytes nextPfx = pfx;

  bool overflow;
  int i = static_cast<int>(nextPfx.size()) - 1;
  // increment the given prefix by 1
  do { overflow = (++nextPfx[i--]) == 0; } while (overflow && i >= 0);

  if (overflow) [[unlikely]] {
    it->SeekToLast();
  } else [[likely]] {
    it->SeekForPrev(rocksdb::Slice(reinterpret_cast<const char*>(nextPfx.data()), nextPfx.size()));
  }

  if (!it->Valid()) return {};

  if (!it->key().starts_with(rocksdb::Slice(reinterpret_cast<const char*>(pfx.data()), pfx.size()))) return {};

  return Bytes(it->value().data(), it->value().data() + it->value().size());
}

