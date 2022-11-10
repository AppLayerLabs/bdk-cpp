#include "blockmanager.h"
#include "subnet.h"
#include "state.h"

BlockManager::BlockManager(std::shared_ptr<DBService> &db, const Address &address, const Address &owner) : _isValidator(false), Contract(address, owner) {
  loadFromDB(db);
  Utils::logToFile("BlockManager Loaded " + std::to_string(validatorsList.size()) + " validators");
  for (auto &validator : validatorsList) {
    Utils::logToFile("Validator: " + validator.hex());
  }
}

BlockManager::BlockManager(std::shared_ptr<DBService> &db, const Hash& privKey, const Address &address, const Address &owner) : _validatorPrivKey(privKey), _isValidator(true), Contract(address, owner) {
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
  randomList = std::vector<std::reference_wrapper<Validator>>(validatorsList.begin(), validatorsList.end());
  State::gen.shuffleVector(randomList);
}

bool BlockManager::validateBlock(const std::shared_ptr<const Block> &block) const {
  // Check validator signature agaisn't block height  prevBlockHash. (not sure if enought for safety)
  managerLock.lock();
  std::string message;
  message += Utils::uint64ToBytes(block->nHeight());
  message += block->prevBlockHash().get();

  Hash hash = Utils::sha3(message);

  // TODO: do we need to include chainId In the block signature?
  auto pubkey = Secp256k1::recover(block->signature(), hash);
  if (Secp256k1::toAddress(pubkey) != this->validatorsList[0].get()) {
    Utils::LogPrint(Log::blockManager,__func__,"Block validator signature does not match validator[0]");
    managerLock.unlock();
    return false;
  }

  if (!Secp256k1::verify(pubkey, block->signature(), hash)) {
    Utils::LogPrint(Log::blockManager,__func__,"Block validator signature is invalid");
    managerLock.unlock();
    return false;
  }

  managerLock.unlock();
  return true;
}