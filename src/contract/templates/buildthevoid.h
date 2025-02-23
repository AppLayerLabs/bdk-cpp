/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BUILDTHEVOID_H
#define BUILDTHEVOID_H


#include "btvcommon.h"
#include "ownable.h"
#include "../../utils/db.h"
#include "../../utils/utils.h"
#include "../dynamiccontract.h"
#include "../variables/safeaddress.h"
#include "../variables/safeunorderedmap.h"
#include "contract/variables/safevector.h"

/// Template for an ERC20 contract.
class BuildTheVoid : virtual public DynamicContract, virtual public Ownable {
  private:
    SafeAddress playerContract_;
    SafeAddress energyContract_;
    SafeUnorderedMap<uint64_t, BTVUtils::PlayerInformation> activePlayers_;
    SafeUnorderedMap<uint64_t, BTVUtils::PlayerInformation> inactivePlayers_;
    SafeUnorderedMap<uint64_t, BTVUtils::PlayerInformation> deadPlayers_;
    SafeVector<BTVUtils::WorldBlockPos> surfaceBlocks_; // Used to generate energy blocks on top of.
    SafeUint64_t energyBlockCounter_;
    BTVUtils::World world_;

    void internalLogoutPlayer(); // Used by the self-calling methods
    void internalKillPlayer(); // Used by the self-calling methods
    void internalSpawnEnergyBlock();
    void selfcallUpdate(); // Used by the self-calling methods


    /// Function for calling the register functions for contracts.
    void registerContractFunctions() override;

  public:
    using ConstructorArguments = std::tuple<>;

    BuildTheVoid(const Address& address, const DB& db);

    BuildTheVoid(const Address &address, const Address &creator, const uint64_t &chainId);

    // Event
    void PlayerMoved(const EventParam<uint64_t, false>& playerId, const EventParam<int32_t, false>& x, const EventParam<int32_t, false>& y, const EventParam<int32_t, false>& z) {
      this->emitEvent("PlayerMoved", std::make_tuple(playerId, x, y, z));
    }
    void PlayerLogin(const EventParam<uint64_t, false>& playerId, const EventParam<int32_t, false>& x, const EventParam<int32_t, false>& y, const EventParam<int32_t, false>& z) {
      this->emitEvent("PlayerLogin", std::make_tuple(playerId, x, y, z));
    }
    void PlayerLogout(const EventParam<uint64_t, false>& playerId) {
      this->emitEvent("PlayerLogout", std::make_tuple(playerId));
    }
    void BlockChanged(const EventParam<uint64_t, false>& playerId, const EventParam<int32_t, false>& x, const EventParam<int32_t, false>& y, const EventParam<int32_t, false>& z, const EventParam<uint8_t, false>& blockType, const EventParam<uint64_t, false>& timestamp) {
      this->emitEvent("BlockChanged", std::make_tuple(playerId, x, y, z, blockType, timestamp));
    }
    void ClaimedEnergy(const EventParam<uint64_t, false>& playerId, const EventParam<uint256_t, false>& value) {
      this->emitEvent("ClaimedEnergy", std::make_tuple(playerId, value));
    }
    void PlayerDead(const EventParam<uint64_t, false>& playerId) {
      this->emitEvent("PlayerDead", std::make_tuple(playerId));
    }

    // Admin methods
    void setPlayerContract(const Address& playerContract);
    void setEnergyContract(const Address& energyContract);
    void forceUpdate();
    void approve();

    // Client methods
    void loginPlayer(const uint64_t& playerId, const uint256_t& energy);
    void logoutPlayer(const uint64_t& playerId);
    void changeBlock(const uint64_t& playerId, const int32_t& x, const int32_t& y, const int32_t& z, const BTVUtils::BlockType& type);
    void movePlayer(const uint64_t& playerId, const int32_t& x, const int32_t& y, const int32_t& z);
    void claimEnergy(const uint64_t& playerId, const int32_t& x, const int32_t& y, const int32_t& z);
    Bytes getChunk(const int32_t& cx, const int32_t& cy) const;

    Address getPlayerContract() const;
    Address getEnergyContract() const;
    std::vector<BTVUtils::PlayerInformationData> getActivePlayers() const;
    std::vector<BTVUtils::PlayerInformationData> getInnactivePlayers() const;
    std::vector<BTVUtils::PlayerInformationData> getDeadPlayers() const;

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        BuildTheVoid,
        const Address &, const Address &,
        const uint64_t &,
        DB&
      >(
        std::vector<std::string>{},
        std::make_tuple("setPlayerContract", &BuildTheVoid::setPlayerContract, FunctionTypes::NonPayable, std::vector<std::string>{"playerContract"}),
        std::make_tuple("setEnergyContract", &BuildTheVoid::setEnergyContract, FunctionTypes::NonPayable, std::vector<std::string>{"energyContract"}),
        std::make_tuple("forceUpdate", &BuildTheVoid::forceUpdate, FunctionTypes::NonPayable, std::vector<std::string>{}),
        std::make_tuple("approve", &BuildTheVoid::approve, FunctionTypes::NonPayable, std::vector<std::string>{}),
        std::make_tuple("loginPlayer", &BuildTheVoid::loginPlayer, FunctionTypes::NonPayable, std::vector<std::string>{"playerId"}),
        std::make_tuple("logoutPlayer", &BuildTheVoid::logoutPlayer, FunctionTypes::NonPayable, std::vector<std::string>{"playerId"}),
        std::make_tuple("changeBlock", &BuildTheVoid::changeBlock, FunctionTypes::NonPayable, std::vector<std::string>{"playerId", "x", "y", "z", "type"}),
        std::make_tuple("movePlayer", &BuildTheVoid::movePlayer, FunctionTypes::NonPayable, std::vector<std::string>{"playerId", "x", "y", "z"}),
        std::make_tuple("claimEnergy", &BuildTheVoid::claimEnergy, FunctionTypes::NonPayable, std::vector<std::string>{"playerId", "x", "y", "z"}),
        std::make_tuple("getChunk", &BuildTheVoid::getChunk, FunctionTypes::View, std::vector<std::string>{"cx", "cy"}),
        std::make_tuple("getPlayerContract", &BuildTheVoid::getPlayerContract, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getEnergyContract", &BuildTheVoid::getEnergyContract, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getActivePlayers", &BuildTheVoid::getActivePlayers, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getInnactivePlayers", &BuildTheVoid::getInnactivePlayers, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getDeadPlayers", &BuildTheVoid::getDeadPlayers, FunctionTypes::View, std::vector<std::string>{})
      );
      ContractReflectionInterface::registerContractEvents<BuildTheVoid>(
        std::make_tuple("PlayerMoved", false, &BuildTheVoid::PlayerMoved, std::vector<std::string>{"playerId", "x", "y", "z"}),
        std::make_tuple("PlayerLogin", false, &BuildTheVoid::PlayerLogin, std::vector<std::string>{"playerId", "x", "y", "z"}),
        std::make_tuple("PlayerLogout", false, &BuildTheVoid::PlayerLogout, std::vector<std::string>{"playerId"}),
        std::make_tuple("BlockChanged", false, &BuildTheVoid::BlockChanged, std::vector<std::string>{"playerId", "x", "y", "z", "blockType", "timestamp"}),
        std::make_tuple("ClaimedEnergy", false, &BuildTheVoid::ClaimedEnergy, std::vector<std::string>{"playerId", "value"}),
        std::make_tuple("PlayerDead", false, &BuildTheVoid::PlayerDead, std::vector<std::string>{"playerId"})
        );
    }

    DBBatch dump() const override;
};

#endif // BUILDTHEVOID_H