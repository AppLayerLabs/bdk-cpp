#ifndef METACOIN_H
#define METACOIN_H

#include "erc20snapshot.h"
#include "accesscontrol.h"
#include "../variables/pausable.h"
#include "../variables/safeenumerablemap.h"

/// Template for a MetaCoin contract.
class MetaCoin : virtual public ERC20, virtual public ERC20Snapshot, virtual public AccessControl {
  protected:
    SafeBool _isAbleToTransfer;
    const Hash OPERATOR_ = Hash(Hex::toBytes("523a704056dcd17bcf83bed8b68c59416dac1119be77755efe3bde0a64e46e0c"));
    SafeEnumerableMap<Address,uint256_t> accountsStatus;
    Pausable::PausableActor pausableActor_;

    virtual void _update(const Address& from, const Address& to, const uint256_t& value);
    void registerContractFunctions() override;
  public:
    using ConstructorArguments = std::tuple<>;

    MetaCoin(
      ContractManagerInterface& interface,
      const Address& contractAddress, const std::unique_ptr<DB>& db
    );

    MetaCoin(
      ContractManagerInterface &interface,
      const Address &address, const Address &creator,
      const uint64_t &chainId, const std::unique_ptr<DB> &db
    );

    ~MetaCoin() override;

    const Hash OPERATOR() const { return OPERATOR_; };
    const uint256_t NORMAL() const { return 0; };
    const uint256_t FREEZE() const { return 1; };
    const uint256_t BAN() const { return 2; };
    /// Error codes
    const std::string E_UNKOWN() const { return "E_U"; }
    const std::string E_ACCOUNT_FREEZED_OR_BANNED() const { return "E_AFB"; }
    const std::string E_NOT_ABLE_TO_TRANSFER() const { return "E_NATT"; }

    void initialize(const std::string& name, const std::string& symbol, const std::vector<Address>& operators, bool isAbleToTransfer);
    uint256_t getAccountsStatusLength() const;
    /// std::tuple<Address,uint256_t>
    BytesEncoded getAccountStatusByIndex(const uint256_t& index);
    /// std::tuple<bool, uint256_t>
    BytesEncoded getAccountStatus(const Address& account);
    void setStatus(const Address& account, const uint256_t& status);
    uint256_t getStatus(const Address& account);
    void snapshot();
    void pause();
    void unpause();
    void mint(const Address& to, const uint256_t& amount);
    void burn(const Address& from, const uint256_t& amount);
    void setIsAbleToTransfer(bool value);

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        MetaCoin, ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{},
        std::make_tuple("OPERATOR", &MetaCoin::OPERATOR, "view", std::vector<std::string>{}),
        std::make_tuple("NORMAL", &MetaCoin::NORMAL, "view", std::vector<std::string>{}),
        std::make_tuple("FREEZE", &MetaCoin::FREEZE, "view", std::vector<std::string>{}),
        std::make_tuple("BAN", &MetaCoin::BAN, "view", std::vector<std::string>{}),
        std::make_tuple("E_UNKOWN", &MetaCoin::E_UNKOWN, "view", std::vector<std::string>{}),
        std::make_tuple("E_ACCOUNT_FREEZED_OR_BANNED", &MetaCoin::E_ACCOUNT_FREEZED_OR_BANNED, "view", std::vector<std::string>{}),
        std::make_tuple("E_NOT_ABLE_TO_TRANSFER", &MetaCoin::E_NOT_ABLE_TO_TRANSFER, "view", std::vector<std::string>{}),
        std::make_tuple("initialize", &MetaCoin::initialize, "nonpayable", std::vector<std::string>{"name", "symbol", "operators", "isAbleToTransfer"}),
        std::make_tuple("getAccountsStatusLength", &MetaCoin::getAccountsStatusLength, "view", std::vector<std::string>{}),
        std::make_tuple("getAccountStatusByIndex", &MetaCoin::getAccountStatusByIndex, "view", std::vector<std::string>{"index"}),
        std::make_tuple("getAccountStatus", &MetaCoin::getAccountStatus, "view", std::vector<std::string>{"account"}),
        std::make_tuple("setStatus", &MetaCoin::setStatus, "nonpayable", std::vector<std::string>{"account", "status"}),
        std::make_tuple("getStatus", &MetaCoin::getStatus, "view", std::vector<std::string>{"account"}),
        std::make_tuple("snapshot", &MetaCoin::snapshot, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("pause", &MetaCoin::pause, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("unpause", &MetaCoin::unpause, "nonpayable", std::vector<std::string>{}),
        std::make_tuple("mint", &MetaCoin::mint, "nonpayable", std::vector<std::string>{"to", "amount"}),
        std::make_tuple("burn", &MetaCoin::burn, "nonpayable", std::vector<std::string>{"from", "amount"}),
        std::make_tuple("setIsAbleToTransfer", &MetaCoin::setIsAbleToTransfer, "nonpayable", std::vector<std::string>{"value"})
      );
    };
};





#endif // METACOIN_H