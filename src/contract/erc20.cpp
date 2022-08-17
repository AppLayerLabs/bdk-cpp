#include "erc20.h"

void ERC20Contract::loadAllERC20(
  DBService &token_db, std::map<std::string, std::shared_ptr<ERC20Contract>> &tokens
) {
  std::vector<DBEntry> pairs = token_db.readBatch(DBPrefix::erc20Tokens);
  for (DBEntry info : pairs) {
    json infoJson = json::parse(info.value);
    tokens[info.key] = std::make_shared<ERC20Contract>(infoJson);
  }
}

bool ERC20Contract::saveAllERC20(
  std::map<std::string, std::shared_ptr<ERC20Contract>> &tokens, DBService &token_db
) {
  for (std::pair<std::string, std::shared_ptr<ERC20Contract>> token : tokens) {
    json jsonData;
    jsonData["name"] = token.second->name();
    jsonData["symbol"] = token.second->symbol();
    jsonData["decimals"] = token.second->decimals();
    jsonData["totalSupply"] = boost::lexical_cast<std::string>(token.second->totalSupply());
    jsonData["address"] = token.first;
    jsonData["balances"] = json::array();
    for (auto balance : token.second->balances()) {
      json tmp;
      tmp["address"] = balance.first;
      tmp["value"] = boost::lexical_cast<std::string>(balance.second);
      jsonData["balances"].push_back(tmp);
    }
    token_db.put(token.first, jsonData.dump(), DBPrefix::erc20Tokens);
  }
  return true;
}

bool ERC20Contract::mint(std::string to, dev::u256 value, bool commit) {
  if ((this->_totalSupply + value) < this->_totalSupply) return false;
  if ((_balances[to] + value) < _balances[to]) return false;
  if (commit) {
    this->_totalSupply += value;
    _balances[to] += value;
  }
  return true;
}

bool ERC20Contract::burn(std::string from, dev::u256 value, bool commit) {
  if ((this->_totalSupply - value) > this->_totalSupply) return false;
  if ((_balances[from] - value) > _balances[from]) return false;
  if (commit) {
    this->_totalSupply -= value;
    _balances[from] -= value;
  }
  return true;
}

ERC20Contract::ERC20Contract(json &data) {
  Utils::logToFile("ERC20: Constructing new contract");
  Utils::logToFile(data.dump(2));
  this->_name = data["name"].get<std::string>();
  this->_symbol = data["symbol"].get<std::string>();
  this->_decimals = data["decimals"].get<uint64_t>();
  Utils::logToFile("ERC20: Basic information added");
  this->_totalSupply = boost::lexical_cast<dev::u256>(data["totalSupply"].get<std::string>());
  this->_address = data["address"].get<std::string>();
  for (auto balance : data["balances"]) {
    std::string add = balance["address"].get<std::string>();
    dev::u256 bal = boost::lexical_cast<dev::u256>(balance["value"].get<std::string>());
    this->_balances[add] = bal;
  }
  Utils::logToFile("ERC20: Constructor finished");
}

dev::u256 ERC20Contract::balanceOf(std::string address) {
  return ((this->_balances.count(address)) ? this->_balances[address] : 0);
}

bool ERC20Contract::transfer(std::string from, std::string to, dev::u256 value, bool commit) {
  if (value > this->_balances[from]) return false;
  if (commit) { this->_balances[from] -= value; this->_balances[to] += value; }
  return true;
}

