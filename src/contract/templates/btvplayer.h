/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BTVPLAYER_H
#define BTVPLAYER_H

#include "standards/erc721uristorage.h"
#include "ownable.h"
#include "../variables/safeunorderedmap.h"
#include "../variables/safeuint.h"

class BTVPlayer : public virtual ERC721, public virtual Ownable {
  private:
    SafeAddress proposalContract_;
    SafeAddress energyContract_;
    SafeAddress worldContract_;
    SafeUnorderedMap<std::string, uint64_t> playerNames_;
    SafeUnorderedMap<uint64_t, std::string> playerToTokens_;
    SafeUnorderedMap<uint64_t, uint256_t> energyBalance_;
    SafeUnorderedMap<Address, std::unordered_set<uint64_t>> addressToPlayers_;
    SafeUint256_t tokenCounter_;

    void registerContractFunctions() override;

  protected:

    // Override ERC721::update_ to add player to addressToPlayers_ map
    Address update_(const Address& to, const uint256_t& tokenId, const Address& auth) override;

  public:
    /**
     * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
     */
    using ConstructorArguments = std::tuple<const std::string&, const std::string&>;

    BTVPlayer(const Address& address, const DB& db);

    BTVPlayer(
      const std::string &erc721_name, const std::string &erc721_symbol,
      const Address &address, const Address &creator, const uint64_t &chainId
    );


    std::string getPlayerName(const uint64_t& tokenId) const;
    bool playerExists(const std::string& playerName) const;
    void mintPlayer(const std::string &name, const Address& to);
    void setProposalContract(const Address& proposalContract);
    void setEnergyContract(const Address& energyContract);
    void setWorldContract(const Address& worldContract);
    Address getProposalContract() const;
    Address getEnergyContract() const;
    Address getWorldContract() const;
    uint256_t totalSupply() const;
    uint256_t getPlayerEnergy(const uint64_t& tokenId) const;
    void addPlayerEnergy(const uint64_t& tokenId, const uint256_t& energy);
    void takePlayerEnergy(const uint64_t& tokenId, const uint256_t& energy);
    std::vector<uint64_t> getPlayerTokens(const Address& player) const;

    void createProposal(const uint64_t& tokenId, const std::string& title, const std::string& description);
    void voteOnProposal(const uint64_t& tokenId, const uint64_t& proposalId, const uint256_t& energy);
    void removeVote(const uint64_t& tokenId, const uint64_t& proposalId, const uint256_t& energy);


    void approveProposalSpend();


    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        BTVPlayer, const std::string &, const std::string &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc721_name", "erc721_symbol"},
        std::make_tuple("getPlayerName", &BTVPlayer::getPlayerName, FunctionTypes::View, std::vector<std::string>{"tokenId"}),
        std::make_tuple("playerExists", &BTVPlayer::playerExists, FunctionTypes::View, std::vector<std::string>{"playerName"}),
        std::make_tuple("mintPlayer", &BTVPlayer::mintPlayer, FunctionTypes::NonPayable, std::vector<std::string>{"name"}),
        std::make_tuple("setProposalContract", &BTVPlayer::setProposalContract, FunctionTypes::NonPayable, std::vector<std::string>{"proposalContract"}),
        std::make_tuple("getProposalContract", &BTVPlayer::getProposalContract, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("setEnergyContract", &BTVPlayer::setEnergyContract, FunctionTypes::NonPayable, std::vector<std::string>{"energyContract"}),
        std::make_tuple("getEnergyContract", &BTVPlayer::getEnergyContract, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("setWorldContract", &BTVPlayer::setWorldContract, FunctionTypes::NonPayable, std::vector<std::string>{"worldContract"}),
        std::make_tuple("getWorldContract", &BTVPlayer::getWorldContract, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("totalSupply", &BTVPlayer::totalSupply, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getPlayerEnergy", &BTVPlayer::getPlayerEnergy, FunctionTypes::View, std::vector<std::string>{"tokenId"}),
        std::make_tuple("addPlayerEnergy", &BTVPlayer::addPlayerEnergy, FunctionTypes::NonPayable, std::vector<std::string>{"tokenId", "energy"}),
        std::make_tuple("takePlayerEnergy", &BTVPlayer::takePlayerEnergy, FunctionTypes::NonPayable, std::vector<std::string>{"tokenId", "energy"}),
        std::make_tuple("getPlayerTokens", &BTVPlayer::getPlayerTokens, FunctionTypes::View, std::vector<std::string>{"player"}),
        std::make_tuple("createProposal", &BTVPlayer::createProposal, FunctionTypes::NonPayable, std::vector<std::string>{"tokenId", "title", "description"}),
        std::make_tuple("voteOnProposal", &BTVPlayer::voteOnProposal, FunctionTypes::NonPayable, std::vector<std::string>{"tokenId", "proposalId", "energy"}),
        std::make_tuple("removeVote", &BTVPlayer::removeVote, FunctionTypes::NonPayable, std::vector<std::string>{"tokenId", "proposalId", "energy"}),
        std::make_tuple("approveProposalSpend", &BTVPlayer::approveProposalSpend, FunctionTypes::NonPayable, std::vector<std::string>{})
      );
    }

    /// Dump method
    DBBatch dump() const override;
};


#endif // BTVPLAYER_H