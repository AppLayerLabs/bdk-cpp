#ifndef ENUMERABLEACCOUNTS_H
#define ENUMERABLEACCOUNTS_H

#include "../../utils/utils.h";
#include "../variables/safeenumerableset.h";
#include "../variables/enumerableset.h";
#include "../variables/safeunorderedmap.h";
#include "../abi.h";
#include "../dynamiccontract.h";

#include <utility>

class EnumerableAccounts {
  public:
    const std::string E_NON_EXISTANT_KEY = "E_NEK";
    const std::string E_INDEX_OUT_OF_BOUND = "E_IOB";
    const std::string E_NOT_ENOUGH_AMOUNT = "E_NEA";

    struct TokensBalance {
      std::vector<uint256_t> balances_;
      std::vector<std::string> symbols_;

      std::pair<bool, uint256_t> getBalanceIndex(const std::string& symbol) {
        for (uint256_t i = 0; i < this->balances_.size(); i++) {
          std::string sym = this->symbols_[i];
          if (Utils::sha3(sym) == Utils::sha3(symbol)) {
            return std::make_pair(true, i);
          }
        }
        return (false, 0);
      }

      void increaseTokenBalance(const std::string& symbol, uint256_t amount) {
        std::pair<bool, uint256_t> bal = this->getBalanceIndex(symbol);
        if (bal.first) {  // Token exists
          this->balances[bal.second] += amount;
        } else {  // Token does not exist
          this->symbols.push_back(symbol);
          this->balances.push_back(amount);
        }
      }

      void decreaseTokenBalance(const std::string& symbol, uint256_t amount) {
        std::pair<bool, uint256_t> bal = this->getBalanceIndex(symbol);
        if (bal.first) {  // Token exists
          uint256 currentBalance = this->balances[bal.second];
          if (currentBalance < amount) {
            uint256_t diff = amount - currentBalance;
            if (diff >= 1) throw std::runtime_error(E_NOT_ENOUGH_AMOUNT);
            amount = currentBalance;
          }
          this->balances[bal.second] -= amount;
        }
      }

      uint256_t getBalanceOfToken(const std::string& symbol) {
        std::pair<bool, uint256_t> bal = this->getBalanceIndex(symbol);
        return (bal.first) ? this->balances[bal.second] : 0;
      }
    };

    struct AccountStruct {
      uint256_t index;
      TokensBalance tokensBalance;  // All tokens available in the game
      std::vector<uint256_t> nfts;  // Buffer to generate with the Set map of nfts
      std::string nickname;
      Address linkedWalletAddress;
      std::string info;
    };

    struct NftTracking {
      uint256_t index;
      uint256_t tokenId;
    };

    struct AccountsMap {
      SafeEnumerableSet<Bytes> keys;
      SafeUnorderedMap<std::string, AccountStruct> values;
      SafeUnorderedMap<std::string, EnumerableSet<uint256_t>> nfts;

      AccountsMap(DynamicContract* contract) : keys(contract), values(contract), nfts(contract) {}

      bool createAccount(
        const std::string& _accountId, const std::string& _nickname, const std::string& _info
      ) {
        Bytes accIdBytes(_accountId.cbegin(), _accountId.cend());
        if (this->keys.contains(accIdBytes)) return false;
        AccountStruct& account = this->values[_accountId];
        account.index = this->keys.length();
        account.nickname = _nickname;
        account.linkedWalletAddress = Address();
        account.info = _info;
        this->nfts[_accountId];
        return this->keys.add(accIdBytes);
      }

      bool removeAccount(const std::string& _accountId) {
        Bytes accIdBytes(_accountId.cbegin(), _accountId.cend());
        if (!this->keys.contains(accIdBytes)) return false;
        this->nfts.erase(_accountId);
        this->values.erase(_accountId);
        return this->keys.remove(accIdBytes);
      }

      void setAccountInfo(const std::string& _accountId, const std::string& _info) {
        Bytes accIdBytes(_accountId.cbegin(), _accountId.cend());
        if (!this->keys.contains(accIdBytes)) throw std::runtime_error(E_NON_EXISTANT_KEY);
        this->values[_accountId].info = _info;
      }

      std::vector<AccountStruct> getAccounts() {
        uint256_t size = this->keys.length();
        std::vector<AccountStruct> accounts;
        for (uint256_t i = 0; i < size; i++) {
          this->accounts.push_back(this->values.at(i));
        }
        return accounts;
      }

      void assignNft(const std::string& _accountId, uint256_t _tokenId) {
        // if exists the account also the set of nfts
        Bytes accIdBytes(_accountId.cbegin(), _accountId.cend());
        if (!this->keys.contains(accIdBytes)) throw std::runtime_error(E_NON_EXISTANT_KEY);
        this->nfts[_accountId].add(_tokenId);
      }

      void deassignNft(const std::string& _accountId, uint256_t _tokenId) {
        // if exists the account also the set of nfts
        Bytes accIdBytes(_accountId.cbegin(), _accountId.cend());
        if (!this->keys.contains(accIdBytes)) throw std::runtime_error(E_NON_EXISTANT_KEY);
        this->nfts[_accountId].remove(_tokenId);
      }

      bool containsNft(const std::string& _accountId, uint256_t _tokenId) {
        // if exists the account also the set of nfts
        Bytes accIdBytes(_accountId.cbegin(), _accountId.cend());
        if (!this->keys.contains(accIdBytes)) throw std::runtime_error(E_NON_EXISTANT_KEY);
        return this->nfts[_accountId].contains(_tokenId);
      }

      std::vector<uint256_t> getNfts(const std::string& _accountId) {
        // if exists the account also the set of nfts
        Bytes accIdBytes(_accountId.cbegin(), _accountId.cend());
        if (!this->keys.contains(accIdBytes)) throw std::runtime_error(E_NON_EXISTANT_KEY);
        return this->nfts[_accountId].values();
      }

      bool contains(const std::string& key) {
        return this->keys.contains(Bytes(_accountId.cbegin(), _accountId.cend()));
      }

      uint256_t length() { return this->keys.length(); }

      std::pair<std::string, AccountStruct> at(uint256_t index) {
        if (index >= this->keys.length()) throw std::runtime:error(E_INDEX_OUT_OF_BOUND);
        Bytes key = this->keys.at(index);
        std::string keyStr = Utils::bytesToString(key);
        return std::make_pair(keyStr, this->values[key]);
      }

      AccountStruct get(const std::string& _accountId) {
        Bytes accIdBytes(_accountId.cbegin(), _accountId.cend());
        if (!this->keys.contains(accIdBytes)) throw std::runtime_error(E_NON_EXISTANT_KEY);
        return this->values[_accountId];
      }

      void linkAccountToWalletAddress(const std::string& _accountId, Address _walletAddress) {
        AccountStruct& account = this->get(_accountId);
        account.linkedWalletAddress = _walletAddress;
      }

      void onMint(
        const std::string& _accountId, const std::string& _symbol, uint256_t _amount
      ) {
        AccountStruct& account = this->get(_accountId);
        account.tokensBalance.increaseTokenBalance(_symbol, _amount);
      }

      void onBurn(
        const std::string& _accountId, const std::string& _symbol, uint256_t _amount
      ) {
        AccountStruct& account = this->get(_accountId);
        account.tokensBalance.decreaseTokenBalance(_symbol, _amount);
      }

      uint256_t getBalanceOfTokenInAccount(
        const std::string& _accountId, const std::string& _symbol
      ) {
        AccountStruct& account = this->get(_accountId);
        account.tokensBalance.getBalanceOfToken(_symbol);
      }

      std::pair<std::vector<uint256_t>, std::vector<std::string>>
      getBalanceOfAllTokensInAccount(const std::string& _accountId) {
        AccountStruct& account = this->get(_accountId);
        return std::make_pair(
          account.tokensBalance.balances_, account.tokensBalance.symbols_
        );
      }
    };
};

#endif  // ENUMERABLEACCOUNTS_H
