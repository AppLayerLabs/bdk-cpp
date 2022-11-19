#include "p2p.h"

P2PMsg::P2PMsg(
  std::string id, std::vector<std::variant<uint64_t, uint256_t, std::string>> args
) {
  // Append ID - check if it exists, throw if it doesn't
  auto it = p2pcmds.find(id);
  if (it == p2pcmds.end()) {
    throw std::runtime_error(std::string(__func__) + ": Command does not exist: " + id);
  }
  this->msg = it->second.first;
  if (!it->second.second) return; // No need to continue if ID has no args

  // Append args for IDs that have them
  std::string err = std::string(__func__) + ": " + it->first + ": ";
  if (it->second.first == "0001") {  // sendTransaction
    if (args.size() != 1) {
      err += "Invalid arg size - expected 1, got " + args.size();
      throw std::runtime_error(err);
    } else if (!std::holds_alternative<std::string>(args[0])) {
      err += "Invalid arg - expected string";
      throw std::runtime_error(err);
    }
    this->msg += std::get<std::string>(args[0]);
  } else if (it->second.first == "0002") {  // sendBulkTransaction
    if (args.size() < 3) {
      err += "Invalid arg size - expected at least 3, got " + args.size();
      throw std::runtime_error(err);
    } else if (args.size() % 2 == 0) {
      err += "Missing arg - expected uint64_t + string";
      throw std::runtime_error(err);
    }
    for (int i = 0; i < args.size(); i++) {
      if ((i == 0 || i % 2 != 0) && !std::holds_alternative<uint64_t>(args[i])) {
        err += "Invalid arg[" + std::to_string(i) + "] - expected uint64_t";
        throw std::runtime_error(err);
      } else if (!std::holds_alternative<std::string>(args[i])) {
        err += "Invalid arg[" + std::to_string(i) + "] - expected string";
        throw std::runtime_error(err);
      }
      this->msg += (i == 0 || i % 2 != 0)
        ? Utils::uint64ToBytes(std::get<uint64_t>(args[i]))
        : std::get<std::string>(args[i]);
    }
  } else if (it->second.first == "0003") {  // requestBlockByNumber
    if (args.size() != 1) {
      err += "Invalid arg size - expected 1, got " + args.size();
      throw std::runtime_error(err);
    } else if (!std::holds_alternative<uint64_t>(args[0])) {
      err += "Invalid arg - expected uint64_t";
      throw std::runtime_error(err);
    }
    this->msg += Utils::uint64ToBytes(std::get<uint64_t>(args[0]));
  } else if (it->second.first == "0004") {  // requestBlockByHash
    if (args.size() != 1) {
      err += "Invalid arg size - expected 1, got " + args.size();
      throw std::runtime_error(err);
    } else if (!std::holds_alternative<uint256_t>(args[0])) {
      err += "Invalid arg - expected uint256_t";
      throw std::runtime_error(err);
    }
    this->msg += Utils::uint256ToBytes(std::get<uint256_t>(args[0]));
  } else if (it->second.first == "0005") {  // requestBlockRange
    if (args.size() != 2) {
      err += "Invalid arg size - expected 2, got " + args.size();
      throw std::runtime_error(err);
    } else if (
      !std::holds_alternative<uint64_t>(args[0]) ||
      !std::holds_alternative<uint64_t>(args[1])
    ) {
      err += "One or more invalid args - expected uint64_t";
      throw std::runtime_error(err);
    }
    this->msg += Utils::uint64ToBytes(std::get<uint64_t>(args[0]))
      + Utils::uint64ToBytes(std::get<uint64_t>(args[1]));
  } else if (it->second.first == "0006") {  // newBestBlock
    if (args.size() != 1) {
      err += "Invalid arg size - expected 1, got " + args.size();
      throw std::runtime_error(err);
    } else if (!std::holds_alternative<std::string>(args[0])) {
      err += "Invalid arg - expected string";
      throw std::runtime_error(err);
    }
    this->msg += std::get<std::string>(args[0]);
  } else if (it->second.first == "0007") {  // sendValidatorTransaction
    if (args.size() != 1) {
      err += "Invalid arg size - expected 1, got " + args.size();
      throw std::runtime_error(err);
    } else if (!std::holds_alternative<std::string>(args[0])) {
      err += "Invalid arg - expected string";
      throw std::runtime_error(err);
    }
    this->msg += std::get<std::string>(args[0]);
  } else if (it->second.first == "0008") {  // sendBulkValidatorTransaction
    if (args.size() < 3) {
      err += "Invalid arg size - expected at least 3, got " + args.size();
      throw std::runtime_error(err);
    } else if (args.size() % 2 == 0) {
      err += "Missing arg - expected uint64_t + string";
      throw std::runtime_error(err);
    }
    for (int i = 0; i < args.size(); i++) {
      if ((i == 0 || i % 2 != 0) && !std::holds_alternative<uint64_t>(args[i])) {
        err += "Invalid arg[" + std::to_string(i) + "] - expected uint64_t";
        throw std::runtime_error(err);
      } else if (!std::holds_alternative<std::string>(args[i])) {
        err += "Invalid arg[" + std::to_string(i) + "] - expected string";
        throw std::runtime_error(err);
      }
      this->msg += (i == 0 || i % 2 != 0)
        ? Utils::uint64ToBytes(std::get<uint64_t>(args[i]))
        : std::get<std::string>(args[i]);
    }
  }
}

P2PRes::P2PRes(std::string data, std::shared_ptr<ChainHead> ch) {
  // Check if ID is valid, throw if it's not
  std::string id = data.substr(0, 4);  // First 2 hex bytes
  bool found = false;
  for (std::pair<std::string, std::pair<std::string, bool>> cmd : p2pcmds) {
    if (cmd.second.first == id) { found = true; break; }
  }
  if (!found) throw std::runtime_error(std::string(__func__) + ": Command does not exist: " + id);

  // Parse the message data
  // TODO: the rest of the commands
  if (id == "0000") { // info
    this->res += "";  // TODO: version
    this->res += Utils::uint64ToBytes(  // Epoch
      std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()
      ).count()
    );
    this->res += Utils::uint64ToBytes(ch->latest()->nHeight()); // nHeight
    this->res += ch->latest()->getBlockHash().get();  // nBestHash
    this->res += "";  // TODO: connectedNodes
  }
}

