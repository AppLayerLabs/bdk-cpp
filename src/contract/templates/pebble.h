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

/// Template for the Pebble NFT mining contract.
class Pebble : public virtual ERC721URIStorage, public virtual Ownable {
  public:
    /// Enum for pebble rarity. MUST be public.
    enum class Rarity { Normal, Gold, Diamond };

  protected:
    SafeUint256_t maxSupply_; ///< Max supply of tokens.
    SafeUint256_t tokenIds_;  ///< Current token id.
    SafeUnorderedMap<uint64_t, Rarity> tokenRarity_;  ///< Map of token rarities.
    SafeUint64_t totalNormal_; ///< Total number of Normal rarity tokens.
    SafeUint64_t totalGold_; ///< Total number of Gold rarity tokens.
    SafeUint64_t totalDiamond_; ///< Total number of Diamond rarity tokens.
    SafeUint256_t raritySeed_; ///< Current Normal token rarity seed.
    SafeUint256_t goldRarity_; ///< Current Gold token rarity seed.
    SafeUint256_t diamondRarity_; ///< Current Diamond token rarity seed.
    SafeAddress authorizer_;  ///< Authorizer address.
    SafeUnorderedMap<Address, bool> minters_; ///< Map of minter addresses.

    void registerContractFunctions() override; ///< Register the contract's functions.

    /**
     * Update the state of a given address related to a given token.
     * @param to The address related to the token.
     * @param tokenId The ID of the token.
     * @param auth The authorizer address.
     * @return The updated address.
     */
    Address update_(const Address& to, const uint256_t& tokenId, const Address& auth) override;

  public:
    using ConstructorArguments = std::tuple<uint256_t>; ///< The contract's constructor arguments.

    /// Event for when an NFT is minted.
    void MintedNFT(const EventParam<Address, false>& user, const EventParam<uint256_t, false>& tokenId, const EventParam<Rarity, false>& rarity) {
      this->emitEvent("MintedNFT", std::make_tuple(user, tokenId, rarity));
    }

    /**
     * Constructor for loading the contract from DB.
     * @param address The address of the contract.
     * @param db The database to use.
     */
    Pebble(const Address& address, const DB& db);

    /**
     * Constructor for creating the contract from scratch.
     * @param maxSupply Max supply of tokens.
     * @param address The address of the contract.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain ID.
     */
    Pebble(const uint256_t& maxSupply, const Address& address, const Address& creator, const uint64_t& chainId);

    /**
     * Mint an NFT to a given address.
     * @param to The address that will receive the NFT.
     * @param num The number of NFTs to mint in a single transaction (max 25).
     */
    void mintNFT(const Address& to, const uint64_t& num);

    /**
     * Get the token rarity for a given token ID.
     * @param tokenId The token ID to query.
     */
    std::string getTokenRarity(const uint256_t& tokenId) const;

    ///@{
    /** Getter. */
    uint256_t totalSupply() const;
    uint256_t maxSupply() const;
    uint64_t totalNormal() const;
    uint64_t totalGold() const;
    uint64_t totalDiamond() const;
    uint256_t raritySeed() const;
    uint256_t diamondRarity() const;
    uint256_t goldRarity() const;
    ///@}

    ///@{
    /** Setter. */
    void setRaritySeed(const uint256_t& seed);
    void setGoldRarity(const uint256_t& rarity);
    void setDiamondRarity(const uint256_t& rarity);
    ///@}

    /**
     * Get the URI of a given token.
     * @param tokenId The token ID to query.
     */
    std::string tokenURI(const uint256_t &tokenId) const final;

    /**
     * Randomly generate a rarity type.
     * @param randomNumber The random number to use.
     */
    Rarity determineRarity(const uint256_t& randomNumber) const;

    /**
     * Get a given rarity type as a string.
     * @param rarity The rarity type to convert.
     */
    std::string rarityToString(const Rarity& rarity) const;

    /**
     * Check if contract caller is the authorizer.
     * @throw DynamicException if caller is not the authorizer.
     */
    void onlyAuthorizer() const;

    /**
     * Change the authorizer address.
     * @param newAuthorizer The address that will be the new authorizer.
     */
    void changeAuthorizer(const Address& newAuthorizer);

    /**
     * Add a minter address to the map.
     * @param minter The minter address to add.
     */
    void addMinter(const Address& minter);

    /**
     * Remove a minter address to the map.
     * @param minter The minter address to remove.
     */
    void removeMinter(const Address& minter);

    /**
     * Check if a given minter address is allowed to mint.
     * @param minter The minter address to check.
     * @throw DynamicException if address is not allowed to mint.
     */
    void canMint(const Address& minter) const;

    Address getAuthorizer() const; ///< Get the authorizer address.

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<Pebble>(
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

    DBBatch dump() const override; ///< Dump method.
};

#endif // PEBBLE_H
