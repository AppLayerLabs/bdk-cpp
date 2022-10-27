#include "blockmanager.h"

BlockManager::BlockManager(std::shared_ptr<DBService> &db) {
  loadFromDB(db);
  Utils::logToFile("BlockManager Loaded " + std::to_string(validatorsList.size()) + " validators");
  for (auto &validator : validatorsList) {
    Utils::logToFile("Validator: " + validator.hex());
  }
}

// Validators are stored in DB as list, 8 bytes (uint64_t) index -> 32 bytes (pubkey).
void BlockManager::loadFromDB(std::shared_ptr<DBService> &db) {
  auto validators = db->readBatch(DBPrefix::validators);
  validatorsList.reserve(validators.size());
  for (auto const &validator : validators) {
    auto it = validatorsList.begin();
    if (validator.key.size() != 8) {
      Utils::LogPrint(Log::blockManager,__func__,"Validator key size is not 8 bytes");
      throw std::runtime_error("Validator key size is not 8 bytes");
    }
    Utils::LogPrint(Log::blockManager,__func__,std::to_string(validator.value.size()));
    if (validator.value.size() != 20) {
      Utils::LogPrint(Log::blockManager,__func__,"Validator value is not 20 bytes (address)");
      throw std::runtime_error("Validator value is not 20 bytes (address)");
    }
    // Emplace_back, make sure that the list is *ordered* before saving to DB.
    validatorsList.emplace_back(Validator(validator.value, false));
    Utils::logToFile("Loop End");
  }
}

