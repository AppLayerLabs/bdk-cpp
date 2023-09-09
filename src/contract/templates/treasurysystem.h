#ifndef TREASURYSYSTEM_H
#define TREASURYSYSTEM_H

#include "../../utils/utils.h";
#include "../variables/safestring.h";
#include "../variables/accesscontrol.h"
#include "../variables/pausable.h"
#include "../abi.h";
#include "../dynamiccontract.h";

#include "enumerabletokens.h";
#include "enumerableaccounts.h";
#include "metacoin.h"
#include "pulsar_nft.h"

#include <utility>

class TreasurySystem : virtual public DynamicContract, virtual public AccessControl {
  protected:
    SafeString name;
    SafeUint256 creationBlock;
    SafeAddress nftContractAddress;
    TokensMap tokens;     // All tokens available in the game
    AccountsMap accounts; // For people that have no wallet but in general for every player
    Pausable::PausableActor pausableActor_;
    const Hash OPERATOR_ = Hash(Hex::toBytes("523a704056dcd17bcf83bed8b68c59416dac1119be77755efe3bde0a64e46e0c"));  // keccak("OPERATOR")

    void registerContractFunctions() override;

  public:
    const std::string E_UNKNOWN = "E_U";
    const std::string E_RECEIVE = "E_R";
    const std::string E_FALLBACK = "E_F";
    const std::string E_FEES_COLLECTOR = "E_FC";
    const std::string E_TOKEN_ADDRESS = "E_TA";
    const std::string E_TO_ACCOUNT_ID = "E_TA";
    const std::string E_FROM_ACCOUNT_ID = "E_FA";
    const std::string E_ACTION = "E_ACT";
    const std::string E_NICKNAME = "E_NIK";
    const std::string E_INFO = "E_INF";
    const std::string E_WALLET_ADDRESS = "E_WA";
    const std::string E_TOKEN_NAME = "E_TN";
    const std::string E_TOKEN_SYMBOL = "E_TS";
    const std::string E_WRONG_SYMBOL = "E_WS";
    const std::string E_AMOUNT = "E_A";
    const std::string E_NOT_ENOUGH_AMOUNT = "E_NEA";
    const std::string E_ACCOUNT_EXISTS = "E_AE";
    const std::string E_ACCOUNT_DOESNT_EXISTS = "E_ADE";

    struct TokenAndAmount {
      std::string symbol;
      uint256_t amount;
    };

    TreasurySystem(
      const std::string& name_, Address _nftContractAddress,
      ContractManagerInterface &interface,
      const Address& address, const Address& creator,
      const uint64_t& chainId, const std::unique_ptr<DB> &db
    );

    TreasurySystem(
      ContractManagerInterface &interface,
      const Address& address, const std::unique_ptr<DB> &db
    );

    ~TreasurySystem() override;

    const Hash OPERATOR() const { return this->OPERATOR_; }

    void initialize(const std::string& _name, Address _nftContractAddress) {
      this->name = _name;
      this->creationBlock = this->getBlockHeight();
      this->nftContractAddress = _nftContractAddress;
      this->grantRole(AccessControl::DEFAULT_ADMIN_ROLE(), this->getCaller());
      this->grantRole(this->OPERATOR(), this->getCaller());
    }

    void pause() {
      this->onlyRole(this->OPERATOR());
      Pausable::pause(this->pausableActor_);
    }

    void unpause() {
      this->onlyRole(this->OPERATOR());
      Pausable::unpause(this->pausableActor_);
    }

    void addOperator(Address opAdd) {
      this->onlyRole(AccessControl::DEFAULT_ADMIN_ROLE());
      this->grantRole(OPERATOR(), opAdd);
    }

    void revokeOperator(Address opAdd) {
      this->onlyRole(AccessControl::DEFAULT_ADMIN_ROLE());
      this->revokeRole(OPERATOR(), opAdd);
    }

    void setToken(Token _token) {
      this->onlyRole(this->OPERATOR());
      this->tokens.set(_token.symbol, _token);
    }

    uint256_t getTokensLength() { return this->tokens.length(); }

    std::vector<Token> getTokens() { return this->tokens.getTokens(); }

    void removeToken(const std::string& _symbol) {
      this->onlyRole(this->OPERATOR());
      if (_symbol.length() == 0) throw std::runtime_error(E_TOKEN_SYMBOL);
      Token token = this->tokens.get(_symbol);
      this->tokens.remove(token.symbol);
    }

    void setTokensStatus(Address _addr, uint256_t _status) {
      this->onlyRole(this->OPERATOR());
      if (_addr == Address()) throw std::runtime_error(E_TOKEN_ADDRESS);
      for (uint256_t i = 0; i < this->tokens.length(); i++) {
        std::pair<std::string, Token> t = this->tokens.at(i);
        this->callContractFunction(t.second.erc20, &MetaCoin::setStatus, _addr, _status);
      }
    }

    std::vector<uint256_t> getTokensStatus(Address _addr) {
      this->onlyRole(this->OPERATOR());
      if (_addr == Address()) throw std::runtime_error(E_TOKEN_ADDRESS);
      uint256_t length = this->tokens.length();
      std::vector<uint256_t> _tokensStatus;
      for (uint256_t i = 0; i < length; i++) {
        std::pair<std::string, Token> t = this->tokens.at(i);
        _tokensStatus.push_back(this->callContractViewFunction(
          t.second.erc20, &MetaCoin::getStatus()
        ));
      }
    }

    void createAccount(
      const std::string& _accountId, const std::string& _nickname, const std::string& _info
    ) {
      this->onlyRole(this->OPERATOR());
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      if (_nickname.length() == 0) throw std::runtime_error(E_NICKNAME);
      if (_info.length() == 0) throw std::runtime_error(E_INFO);
      if (this->accounts.createAccount(_accountId, _nickname, _info)) {
        for (Token t : this->tokens.getTokens()) {
          this->accounts.onMint(_accountId, t.symbol, 0);
        }
      } else {
        throw std::runtime_error(E_ACCOUNT_EXISTS);
      }
    }

    void removeAccount(const std::string& _accountId) {
      this->onlyRole(this->OPERATOR());
      if (!this->accounts.removeAccount(_accountId)) {
        throw std::runtime_error(E_ACCOUNT_DOESNT_EXISTS);
      }
    }

    void setAccountInfo(const std::string& _accountId, const std::string& _info) {
      this->onlyRole(this->OPERATOR());
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      if (_info.length() == 0) throw std::runtime_error(E_INFO);
      this->accounts.setAccountInfo(_accountId, _info);
    }

    bool existAccount(const std::string& _accountId) {
      this->onlyRole(this->OPERATOR());
      return this->accounts.contains(_accountId);
    }

    AccountStruct getAccount(const std::string& _accountId) {
      this->onlyRole(this->OPERATOR());
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      AccountStruct account = this->accounts.get(_accountId);
      if (account.linkedWalletAddress != Address()) {
        std::pair<std::vector<uint256_t>, std::vector<std::string>> p;
        p = this->accounts.getBalanceOfAllTokensInAccount(_accountId);
        account.tokensBalances.balances_ = p.first;
        account.tokensBalances.symbols_ = p.second;
      }
      account.nfts = this->accounts.getNfts(_accountId);
      return account;
    }

    uint256_t getAccountsLength() {
      this->onlyRole(this->OPERATOR());
      return this->accounts.length();
    }

    std::pair<std::string, AccountStruct> getAccountByIndex(uint256_t index) {
      this->onlyRole(this->OPERATOR());
      std::pair<std::string, AccountStruct> p = this->accounts.at(index);
      p.second.nfts = this->accounts.getNfts(p.first);
      return p;
    }

    void linkAcountToWalletAddress(const std::string& _accountId, Address _walletAddress) {
      this->onlyRole(this->OPERATOR());
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      if (_walletAddress != Address()) throw std::runtime_error(E_WALLET_ADDRESS);
      AccountStruct account = this->accounts.get(_accountId);
      if (account.linkedWalletAddress == Address()) {
        this->accounts.linkAccountToWalletAddress(_accountId, _walletAddress);
        for (Token t : this->tokens.getTokens()) {
          uint256_t amount = this->accounts.getBalanceOfTokenInAccount(_accountId, t.symbol);
          this->accounts.onBurn(_accountId, t.symbol, amount);
          this->callContractFunction(t.erc20, &MetaCoin::mint, _walletAddress, amount);
        }
      } else {
        for (Token t : this->tokens.getTokens()) {
          uint256_t amount = this->callContractViewFunction(t.erc20, &MetaCoin::balanceOf, account.linkedWalletAddress);
          this->callContractFunction(t.erc20, &MetaCoin::burn, account.linkedWalletAddress, amount);
          this->callContractFunction(t.erc20, &MetaCoin::mint, _walletAddress, amount);
        }
        this->accounts.linkAccountToWalletAddress(_accountId, _walletAddress);
      }
    }

    void mint(
      const std::string& _action, Address _to, const std::string& _symbol, uint256_t _amount
    ) {
      this->onlyRole(this->OPERATOR());
      if (_to == Address()) throw std::runtime_error(E_TOKEN_ADDRESS);
      if (_amount == 0) throw std::runtime_error(E_AMOUNT);
      this->callContractFunction(this->tokens.get(_symbol).erc20, &MetaCoin::mint, _to, _amount);
    }

    void burn(
      const std::string& _action, Address _from, const std::string& _symbol, uint256_t _amount
    ) {
      this->onlyRole(this->OPERATOR());
      if (_from == Address()) throw std::runtime_error(E_TOKEN_ADDRESS);
      if (_amount == 0) throw std::runtime_error(E_AMOUNT);
      this->callContractFunction(this->tokens.get(_symbol).erc20, &MetaCoin::burn, _from, _amount);
    }

    void mintToAccount(
      const std::string& _action, const std::string& _accountId,
      std::vector<TokenAndAmount> _tokensAndAmount
    ) {
      this->onlyRole(this->OPERATOR());
      if (_action.length() == 0) throw std::runtime_error(E_ACTION);
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      if (_tokensAndAmount.length() == 0) throw std::runtime_error(E_UNKNOWN);
      AccountStruct account = this->accounts.get(_accountId);
      if (account.linkedWalletAddress != Address()) {
        for (TokensAndAmount t : _tokensAndAmount) {
          this->mint(_action, account.linkedWalletAddress, t.symbol, t.amount);
        }
      } else {
        for (TokensAndAmount t : _tokensAndAmount) {
          this->accounts.onMint(_accountId, t.symbol, t.amount);
        }
      }
    }

    void burnFromAccount(
      const std::string& _action, const std::string& _accountId,
      std::vector<TokenAndAmount> _tokensAndAmount
    ) {
      this->onlyRole(this->OPERATOR());
      if (_action.length() == 0) throw std::runtime_error(E_ACTION);
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      if (_tokensAndAmount.length() == 0) throw std::runtime_error(E_UNKNOWN);
      AccountStruct account = this->accounts.get(_accountId);
      if (account.linkedWalletAddress != Address()) {
        for (TokensAndAmount t : _tokensAndAmount) {
          this->burn(_action, account.linkedWalletAddress, t.symbol, t.amount);
        }
      } else {
        for (TokensAndAmount t : _tokensAndAmount) {
          this->accounts.onBurn(_accountId, t.symbol, t.amount);
        }
      }
    }

    void transfer(
      const std::string& _action, const std::string& _fromAccountId,
      const std::string& _toAccountId, const std::string& symbol_, uint256_t _amount
    ) {
      this->onlyRole(this->OPERATOR());
      if (_action.length() == 0) throw std::runtime_error(E_ACTION);
      if (_symbol.length() == 0) throw std::runtime_error(E_TOKEN_SYMBOL);
      if (_amount == 0) throw std::runtime_error(E_AMOUNT);
      AccountStruct fromAccount = this->accounts.get(_fromAccountId);
      AccountStruct toAccount = this->accounts.get(_toAccountId);
      // TODO: in the original those are literally not used anywhere inside the function's scope, only assigned at the commented lines
      //Address from = Address(this); Address to = Address(this);

      if (fromAccount.linkedWalletAddress != Address()) {
        this->burn(_action, fromAccount.linkedWalletAddress, _symbol, _amount);
        //from = fromAccount.linkedWalletAddress;
      } else {
        this->accounts.onBurn(_fromAccountId, _symbol, _amount);
      }
      if (toAccount.linkedWalletAddress != Address()) {
        this->mint(_action, toAccount.linkedWalletAddress, _symbol, _amount);
        //to = fromAccount.linkedWalletAddress;
      } else {
        this->accounts.onMint(_toAccountId, _symbol, _amount);
      }
    }

    uint256_t getBalanceOfTokenInAccount(
      const std::string& _accountId, const std::string& _symbol
    ) {
      if (_symbol.length() == 0) throw std::runtime_error(E_TOKEN_SYMBOL);
      Account account = this->accounts.get(_accountsId);
      return (account.linkedWalletAddress != Address())
        ? this->callContractViewFunction(this->tokens.get(_symbol).erc20,
          &MetaCoin::balanceOf, account.linkedWalletAddress
        )
        : this->accounts.getBalanceOfTokenInAccount(_accountId, _symbol);
    }

    std::pair<std::vector<uint256_t>, std::vector<std::string>>
    getBalanceOfAllTokensInAccount(const std::string& _accountId) {
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      AccountStruct account = this->accounts.get(_accountId);
      if (account.linkedWalletAddress != Address()) {
        std::pair<std::vector<uint256_t>, std::vector<std::string>> ret;
        for (Token t : this->tokens.getTokens()) {
          ret.first.push_back(this->callContractViewFunction(
            t.erc20, &MetaCoin::balanceOf, account.linkedWalletAddress
          ));
          ret.second.push_back(t.symbol);
        }
        return ret;
      } else {
        return this->accounts.getBalanceOfAllTokensInAccount(_accountId);
      }
    }

    void assignNftsToAccount(const std::string& _accountId, std::vector<uint256_t> _tokenIds) {
      this->onlyRole(this->OPERATOR());
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      if (!this->accounts.contains(_accountId)) throw std::runtime_error(E_ACCOUNT_DOESNT_EXISTS);
      for (uint256_t tokenId : _tokenIds) this->accounts.assignNft(_accountId, tokenId);
    }

    void deassignNftsToAccount(const std::string& _accountId, std::vector<uint256_t> _tokenIds) {
      this->onlyRole(this->OPERATOR());
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      if (!this->accounts.contains(_accountId)) throw std::runtime_error(E_ACCOUNT_DOESNT_EXISTS);
      for (uint256_t tokenId : _tokenIds) this->accounts.deassignNft(_accountId, tokenId);
    }

    std::vector<uint256_t> getNftsFromAccount(const std::string& _accountId) {
      this->onlyRole(this->OPERATOR());
      if (_accountId.length() == 0) throw std::runtime_error(E_TO_ACCOUNT_ID);
      if (!this->accounts.contains(_accountId)) throw std::runtime_error(E_ACCOUNT_DOESNT_EXISTS);
      AccountStruct account = this->accounts.get(_accountId);
      std::vector<uint256_t> nftsInTreasurySystem = this->accounts.getNfts(_accountId);
      if (account.linkedWalletAddress != Address()) {
        uint256_t balance = this->callContractViewFunction(
          nftContractAddress, &PulsarNft::balanceOf, account.linkedWalletAddress
        );
        if (balance > 0) {
          std::vector<uint256_t> nftsWithWallet;
          for (uint256_t i = 0; i < balance; i++) nftsWithWallet.push_back(
            this->callContractViewFunction(nftContractAddress,
              &PulsarNft::tokenOfOwnerByIndex, account.linkedWalletAddress, i
            )
          );
          for (uint256_t i = 0; i < nftsInTreasurySystem.length(); i++) {
            nftsWithWallet[i + balance] = nftsInTreasurySystem[i];
          }
        }
      }
      return nftsInTreasurySystem;
    }

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        TreasurySystem, ContractManagerInterface&,
        const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{"name_", "_nftContractAddress"},
        std::make_tuple("OPERATOR", &TreasurySystem::OPERATOR, "view", std::vector<std::string>{}),
        std::make_tuple("initialize", &TreasurySystem::initialize, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("pause", &TreasurySystem::pause, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("unpause", &TreasurySystem::unpause, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("addOperator", &TreasurySystem::addOperator, "nonpayable", std::vector<std::string>{"opAdd"}),
        std::make_tuple("removeOperator", &TreasurySystem::removeOperator, "nonpayable", std::vector<std::string>{"opAdd"}),
        std::make_tuple("setToken", &TreasurySystem::setToken, "nonpayable", std::vector<std::string>{"_token"}),
        std::make_tuple("getTokensLength", &TreasurySystem::getTokensLength, "view", std::vector<std::string>{}),
        std::make_tuple("getTokens", &TreasurySystem::getTokens, "view", std::vector<std::string>{}),
        std::make_tuple("removeToken", &TreasurySystem::removeToken, "nonpayable", std::vector<std::string>{"_symbol"}),
        std::make_tuple("setTokensStatus", &TreasurySystem::setTokensStatus, "nonpayable", std::vector<std::string>{"_addr", "_status"}),
        std::make_tuple("getTokensStatus", &TreasurySystem::getTokensStatus, "view", std::vector<std::string>{"_addr"}),
        std::make_tuple("createAccount", &TreasurySystem::createAccount, "nonpayable", std::vector<std::string>{"_accountId", "_nickname", "_info"}),
        std::make_tuple("removeAccount", &TreasurySystem::removeAccount, "nonpayable", std::vector<std::string>{"_accountId"}),
        std::make_tuple("setAccountInfo", &TreasurySystem::setAccountInfo, "nonpayable", std::vector<std::string>{"_accountId", "_info"}),
        std::make_tuple("existAccount", &TreasurySystem::existAccount, "view", std::vector<std::string>{"_accountId"}),
        std::make_tuple("getAccount", &TreasurySystem::getAccount, "view", std::vector<std::string>{"_accountId"}),
        std::make_tuple("getAccountsLength", &TreasurySystem::getAccountsLength, "view", std::vector<std::string>{}),
        std::make_tuple("getAccountByIndex", &TreasurySystem::getAccountByIndex, "view", std::vector<std::string>{"index"}),
        std::make_tuple("linkAccountToWalletAddress", &TreasurySystem::linkAccountToWalletAddress, "nonpayable", std::vector<std::string>{"_accountId", "_walletAddress"}),
        std::make_tuple("mint", &TreasurySystem::mint, "nonpayable", std::vector<std::string>{"_action", "_to", "_symbol", "_amount"}),
        std::make_tuple("burn", &TreasurySystem::burn, "nonpayable", std::vector<std::string>{"_action", "_from", "_symbol", "_amount"}),
        std::make_tuple("mintToAccount", &TreasurySystem::mintToAccount, "nonpayable", std::vector<std::string>{"_action", "_accountId", "_tokensAndAmount"}),
        std::make_tuple("burnFromAccount", &TreasurySystem::burnToAccount, "nonpayable", std::vector<std::string>{"_action", "_accountId", "_tokensAndAmount"}),
        std::make_tuple("transfer", &TreasurySystem::transfer, "nonpayable", std::vector<std::string>{"_action", "_fromAccountId", "_toAccountId", "_symbol", "_amount"}),
        std::make_tuple("getBalanceOfTokenInAccount", &TreasurySystem::getBalanceOfTokenInAccount, "view", std::vector<std::string>{"_accountId", "_symbol"}),
        std::make_tuple("getBalanceOfAllTokensInAccount", &TreasurySystem::getBalanceOfAllTokensInAccount, "view", std::vector<std::string>{"_accountId"}),
        std::make_tuple("assignNftsToAccount", &TreasurySystem::assignNftsToAccount, "nonpayable", std::vector<std::string>{"_accountId", "_tokenIds"}),
        std::make_tuple("deassignNftsToAccount", &TreasurySystem::deassignNftsToAccount, "nonpayable", std::vector<std::string>{"_accountId", "_tokenIds"}),
        std::make_tuple("getNftsFromAccount", &TreasurySystem::getNftsFromAccount, "view", std::vector<std::string>{"_accountId"})
      );
    }
};

#endif  // TREASURYSTSTEM_H
