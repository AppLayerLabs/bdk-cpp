/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef PEBBLE_H
#define PEBBLE_H

#include "erc721uristorage.h"
#include "ownable.h"
#include "../variables/safeunorderedmap.h"
#include "../variables/safeuint.h"

class Pebble : public virtual ERC721URIStorage, public virtual Ownable {
  public:
    // enum declaration MUST be public
    enum class Rarity {
      Normal,
      Gold,
      Diamond
    };
  protected:
    SafeUint256_t maxSupply_;
    SafeUint256_t tokenIds_;
    SafeUnorderedMap<uint64_t, Rarity> tokenRarity_;
    SafeUint64_t totalNormal_;
    SafeUint64_t totalGold_;
    SafeUint64_t totalDiamond_;
    SafeUint256_t raritySeed_;
    SafeUint256_t diamondRarity_;
    SafeUint256_t goldRarity_;
    SafeAddress authorizer_;
    SafeUnorderedMap<Address, bool> minters_;

    void registerContractFunctions() override;

    Address update_(const Address& to, const uint256_t& tokenId, const Address& auth) override;

  public:
    using ConstructorArguments = std::tuple<uint256_t>;

    void MintedNFT(const EventParam<Address, false>& user, const EventParam<uint256_t, false>& tokenId, const EventParam<Rarity, false>& rarity) {
      this->emitEvent("MintedNFT", std::make_tuple(user, tokenId, rarity));
    }

    // DB Constructor for loading contract from DB.
    Pebble(const Address& address, const DB& db);

    // Create a new contract.
    Pebble(const uint256_t& maxSupply, const Address& address, const Address& creator, const uint64_t& chainId);

    void mintNFT(const Address& to, const uint64_t& num);

    std::string getTokenRarity(const uint256_t& tokenId) const;

    uint256_t totalSupply() const;

    uint256_t maxSupply() const;

    uint64_t totalNormal() const;

    uint64_t totalGold() const;

    uint64_t totalDiamond() const;

    uint256_t raritySeed() const;

    uint256_t diamondRarity() const;

    uint256_t goldRarity() const;

    void setRaritySeed(const uint256_t& seed);

    void setDiamondRarity(const uint256_t& rarity);

    void setGoldRarity(const uint256_t& rarity);

    std::string tokenURI(const uint256_t &tokenId) const final;

    Rarity determineRarity(const uint256_t& randomNumber) const;

    std::string rarityToString(const Rarity& rarity) const;

    void onlyAuthorizer() const;

    void changeAuthorizer(const Address& newAuthorizer);

    void addMinter(const Address& minter);

    void removeMinter(const Address& minter);

    void canMint(const Address& minter) const;

    Address getAuthorizer() const;

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        Pebble
      >(
        std::vector<std::string>{"maxSupply"},
        std::make_tuple("mintNFT", &Pebble::mintNFT, FunctionTypes::NonPayable, std::vector<std::string>{"to"}),
        std::make_tuple("getTokenRarity", &Pebble::getTokenRarity, FunctionTypes::View, std::vector<std::string>{"tokenId"}),
        std::make_tuple("totalSupply", &Pebble::totalSupply, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("maxSupply", &Pebble::maxSupply, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("totalNormal", &Pebble::totalNormal, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("totalGold", &Pebble::totalGold, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("totalDiamond", &Pebble::totalDiamond, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("raritySeed", &Pebble::raritySeed, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("diamondRarity", &Pebble::diamondRarity, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("goldRarity", &Pebble::goldRarity, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("setRaritySeed", &Pebble::setRaritySeed, FunctionTypes::NonPayable, std::vector<std::string>{"seed"}),
        std::make_tuple("setDiamondRarity", &Pebble::setDiamondRarity, FunctionTypes::NonPayable, std::vector<std::string>{"rarity"}),
        std::make_tuple("setGoldRarity", &Pebble::setGoldRarity, FunctionTypes::NonPayable, std::vector<std::string>{"rarity"}),
        std::make_tuple("tokenURI", &Pebble::tokenURI, FunctionTypes::View, std::vector<std::string>{"tokenId"}),
        std::make_tuple("determineRarity", &Pebble::determineRarity, FunctionTypes::View, std::vector<std::string>{"randomNumber"}),
        std::make_tuple("rarityToString", &Pebble::rarityToString, FunctionTypes::View, std::vector<std::string>{"rarity"}),
        std::make_tuple("onlyAuthorizer", &Pebble::onlyAuthorizer, FunctionTypes::NonPayable, std::vector<std::string>{}),
        std::make_tuple("changeAuthorizer", &Pebble::changeAuthorizer, FunctionTypes::NonPayable, std::vector<std::string>{"newAuthorizer"}),
        std::make_tuple("addMinter", &Pebble::addMinter, FunctionTypes::NonPayable, std::vector<std::string>{"minter"}),
        std::make_tuple("removeMinter", &Pebble::removeMinter, FunctionTypes::NonPayable, std::vector<std::string>{"minter"}),
        std::make_tuple("canMint", &Pebble::canMint, FunctionTypes::View, std::vector<std::string>{"minter"}),
        std::make_tuple("getAuthorizer", &Pebble::getAuthorizer, FunctionTypes::View, std::vector<std::string>{})
      );
      ContractReflectionInterface::registerContractEvents<Pebble>(
        std::make_tuple("MintedNFT", false, &Pebble::MintedNFT, std::vector<std::string>{"user","tokenId", "rarity"})
      );
    }

    DBBatch dump() const override;
};



#endif // PEBBLE_H
