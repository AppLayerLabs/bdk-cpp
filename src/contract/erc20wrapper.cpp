#include "erc20wrapper.h"

ERC20Wrapper::ERC20Wrapper(ContractManager::ContractManagerInterface &interface, const Address& contractAddress, const std::unique_ptr<DB> &db) :
    DynamicContract(interface, contractAddress, db), _tokensAndBalances(this) {
  registerContractFunctions();
  auto tokensAndBalances = this->db->getBatch(this->getNewPrefix("_tokensAndBalances"));
  for (const auto& dbEntry : tokensAndBalances) {
    BytesArrView valueView(dbEntry.value);
    this->_tokensAndBalances[Address(dbEntry.key)][Address(valueView.subspan(0, 20))] = Utils::fromBigEndian<uint256_t>(valueView.subspan(20));
  }
  updateState(true);
}

ERC20Wrapper::ERC20Wrapper(ContractManager::ContractManagerInterface &interface,
                           const Address& address, const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db) :
    DynamicContract(interface, "ERC20Wrapper", address, creator, chainId, db), _tokensAndBalances(this) {
  registerContractFunctions();
  updateState(true);
}

ERC20Wrapper::~ERC20Wrapper() {
  DBBatch tokensAndBalancesBatch;

  for (auto it = _tokensAndBalances.cbegin(); it != _tokensAndBalances.cend(); ++it) {
    for (auto it2 = it->second.cbegin(); it2 != it->second.cend(); ++it2) {
      const auto& key = it->first.get();
      Bytes value = it2->first.asBytes();
      Utils::appendBytes(value, Utils::uintToBytes(it2->second));
      tokensAndBalancesBatch.push_back(key, value, this->getNewPrefix("_tokensAndBalances"));
    }
  }

  this->db->putBatch(tokensAndBalancesBatch);
}

void ERC20Wrapper::registerContractFunctions() {
  this->registerViewFunction(Hex::toBytes("0x43ab265f"), [this](const ethCallInfo &callInfo) {
    std::vector<ABI::Types> types = { ABI::Types::address };
    ABI::Decoder decoder(types, std::get<6>(callInfo));
    return this->getContractBalance(decoder.getData<Address>(0));
  });
  this->registerViewFunction(Hex::toBytes("0x6805d6ad"), [this](const ethCallInfo &callInfo) {
    std::vector<ABI::Types> types = { ABI::Types::address, ABI::Types::address };
    ABI::Decoder decoder(types, std::get<6>(callInfo));
    return this->getUserBalance(decoder.getData<Address>(0), decoder.getData<Address>(1));
  });
  this->registerFunction(Hex::toBytes("0xf3fef3a3"), [this](const ethCallInfo &callInfo) {
    std::vector<ABI::Types> types = { ABI::Types::address, ABI::Types::uint256 };
    ABI::Decoder decoder(types, std::get<6>(callInfo));
    this->withdraw(decoder.getData<Address>(0), decoder.getData<uint256_t>(1));
  });
  this->registerFunction(Hex::toBytes("0xa5f2a152"), [this](const ethCallInfo &callInfo) {
    std::vector<ABI::Types> types = { ABI::Types::address, ABI::Types::address, ABI::Types::uint256 };
    ABI::Decoder decoder(types, std::get<6>(callInfo));
    this->transferTo(decoder.getData<Address>(0), decoder.getData<Address>(1), decoder.getData<uint256_t>(2));
  });
  this->registerFunction(Hex::toBytes("0x47e7ef24"), [this](const ethCallInfo &callInfo) {
    std::vector<ABI::Types> types = { ABI::Types::address, ABI::Types::uint256 };
    ABI::Decoder decoder(types, std::get<6>(callInfo));
    this->deposit(decoder.getData<Address>(0), decoder.getData<uint256_t>(1));
  });
  return;
}

Bytes ERC20Wrapper::getContractBalance(const Address& token) const {
  auto* ERC20Token = this->getContract<ERC20>(token);
  return ERC20Token->balanceOf(this->getContractAddress());
}

Bytes ERC20Wrapper::getUserBalance(const Address& token, const Address& user) const {
  auto it = this->_tokensAndBalances.find(token);
  if (it == this->_tokensAndBalances.end()) {
    return ABI::Encoder({0}).getData();
  }
  auto itUser = it->second.find(user);
  if (itUser == it->second.end()) {
    return ABI::Encoder({0}).getData();
  }
  return ABI::Encoder({itUser->second}).getData();
}

void ERC20Wrapper::withdraw(const Address& token, const uint256_t& value) {
  auto it = this->_tokensAndBalances.find(token);
  if (it == this->_tokensAndBalances.end()) {
    throw std::runtime_error("Token not found");
  }
  auto itUser = it->second.find(this->getCaller());
  if (itUser == it->second.end()) {
    throw std::runtime_error("User not found");
  }
  if (itUser->second <= value) {
    throw std::runtime_error("Not enough balance");
  }

  itUser->second -= value;
  ABI::Encoder encoder({this->getCaller(), value}, "transfer(address,uint256)");
  this->callContract(token, encoder);
}

void ERC20Wrapper::transferTo(const Address& token, const Address& to, const uint256_t& value) {
  auto it = this->_tokensAndBalances.find(token);
  if (it == this->_tokensAndBalances.end()) {
    throw std::runtime_error("Token not found");
  }
  auto itUser = it->second.find(this->getCaller());
  if (itUser == it->second.end()) {
    throw std::runtime_error("User not found");
  }
  if (itUser->second <= value) {
    throw std::runtime_error("Not enough balance");
  }

  itUser->second -= value;
  ABI::Encoder encoder({to, value}, "transfer(address,uint256)");
  this->callContract(token, encoder);
}

void ERC20Wrapper::deposit(const Address& token, const uint256_t& value) {
  ABI::Encoder encoder({this->getCaller(), this->getContractAddress(), value}, "transferFrom(address,address,uint256)");
  this->callContract(token, encoder);
  this->_tokensAndBalances[token][this->getCaller()] += value;
}