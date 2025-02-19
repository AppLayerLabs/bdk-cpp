/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef BTVPROPOSALS_H
#define BTVPROPOSALS_H

#include <memory>

#include "ownable.h"
#include "../../utils/db.h"
#include "../../utils/utils.h"
#include "../abi.h"
#include "../dynamiccontract.h"
#include "../variables/safestring.h"
#include "../variables/safeuint.h"
#include "../variables/safeunorderedmap.h"
#include "../variables/safeaddress.h"

using BTVProposal = std::tuple<
  uint256_t, // Energy Staked in Proposal
  std::string, // Proposal Title
  std::string // Proposal Description
>;


class BTVProposals : public virtual DynamicContract, public Ownable {
  private:
    SafeUint64_t proposalCount_;
    SafeUnorderedMap<uint64_t, BTVProposal> activeProposals_;
    SafeUnorderedMap<uint64_t, BTVProposal> completedProposals_;
    SafeAddress playerContract_;
    SafeAddress energyContract_;
    SafeUnorderedMap<uint64_t, std::unordered_map<uint64_t, uint256_t, SafeHash>> proposalVotes_;
    SafeUint256_t proposalPrice_;

    void registerContractFunctions() override;
    void onlyPlayer() const;
  public:
    using ConstructorArguments = std::tuple<>;

    BTVProposals(const Address& address, const DB& db);
    BTVProposals(
      const Address &address, const Address &creator, const uint64_t &chainId
    );

    void createProposal(const std::string& title, const std::string& description); // From
    void voteOnProposal(const uint64_t& tokenId, const uint64_t& proposalId, const uint256_t& energy);
    void removeVote(const uint64_t& tokenId, const uint64_t& proposalId, const uint256_t& energy);

    void completeProposal(const uint64_t& proposalId);

    void setProposalPrice(const uint256_t& price);
    void setPlayerContract(const Address& playerContract);
    void setEnergyContract(const Address& energyContract);

    std::vector<BTVProposal> getActiveProposals() const;
    std::vector<BTVProposal> getCompletedProposals() const;
    std::vector<std::tuple<uint64_t, uint256_t>> getProposalVotes(const uint64_t& proposalId) const;
    uint256_t getProposalPrice() const;
    uint256_t getProposalEnergy(const uint64_t& proposalId) const;
    uint64_t getProposalCount() const;

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        BTVProposals, const std::string &, const std::string &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{""},
        std::make_tuple("createProposal", &BTVProposals::createProposal, FunctionTypes::NonPayable, std::vector<std::string>{"title", "description"}),
        std::make_tuple("voteOnProposal", &BTVProposals::voteOnProposal, FunctionTypes::NonPayable, std::vector<std::string>{"tokenId", "proposalId", "energy"}),
        std::make_tuple("removeVote", &BTVProposals::removeVote, FunctionTypes::NonPayable, std::vector<std::string>{"tokenId", "proposalId", "energy"}),
        std::make_tuple("completeProposal", &BTVProposals::completeProposal, FunctionTypes::NonPayable, std::vector<std::string>{"proposalId"}),
        std::make_tuple("setProposalPrice", &BTVProposals::setProposalPrice, FunctionTypes::NonPayable, std::vector<std::string>{"price"}),
        std::make_tuple("setPlayerContract", &BTVProposals::setPlayerContract, FunctionTypes::NonPayable, std::vector<std::string>{"playerContract"}),
        std::make_tuple("setEnergyContract", &BTVProposals::setEnergyContract, FunctionTypes::NonPayable, std::vector<std::string>{"energyContract"}),
        std::make_tuple("getActiveProposals", &BTVProposals::getActiveProposals, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getCompletedProposals", &BTVProposals::getCompletedProposals, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getProposalVotes", &BTVProposals::getProposalVotes, FunctionTypes::View, std::vector<std::string>{"proposalId"}),
        std::make_tuple("getProposalPrice", &BTVProposals::getProposalPrice, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("getProposalEnergy", &BTVProposals::getProposalEnergy, FunctionTypes::View, std::vector<std::string>{"proposalId"}),
        std::make_tuple("getProposalCount", &BTVProposals::getProposalCount, FunctionTypes::View, std::vector<std::string>{})
      );
    }

    DBBatch dump() const override;
};


#endif // BTVPROPOSALS_H

