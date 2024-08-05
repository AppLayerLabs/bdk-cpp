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
      Silver,
      Gold,
      Diamond
    };
  protected:
    SafeUint256_t maxSupply_;
    SafeUint256_t tokenIds_;
    SafeUnorderedMap<uint256_t, Rarity> tokenRarity_;

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

    void mintNFT(const Address& to);

    std::string getTokenRarity(const uint256_t& tokenId) const;

    uint256_t totalSupply() const;

    uint256_t maxSupply() const;

    static Rarity determineRarity_(const uint256_t& randomNumber);

    static std::string rarityToString_(const Rarity& rarity);

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        Pebble
      >(
        std::vector<std::string>{"maxSupply"},
        std::make_tuple("mintNFT", &Pebble::mintNFT, FunctionTypes::NonPayable, std::vector<std::string>{"to"}),
        std::make_tuple("getTokenRarity", &Pebble::getTokenRarity, FunctionTypes::View, std::vector<std::string>{"tokenId"}),
        std::make_tuple("totalSupply", &Pebble::totalSupply, FunctionTypes::View, std::vector<std::string>{}),
        std::make_tuple("maxSupply", &Pebble::maxSupply, FunctionTypes::View, std::vector<std::string>{})
      );
      ContractReflectionInterface::registerContractEvents<Pebble>(
        std::make_tuple("MintedNFT", false, &Pebble::MintedNFT, std::vector<std::string>{"user","tokenId", "rarity"})
      );
    }

    DBBatch dump() const override;
};



#endif // PEBBLE_H
