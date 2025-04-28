/*
  Copyright (c) [2023-2024] [AppLayer Developers]
  This software is distributed under the MIT License.
  See the LICENSE.txt file in the project root for more information.
*/

#ifndef ERC721URISTORAGE_H
#define ERC721URISTORAGE_H

#include "erc721.h"

/// Template for an ERC721URIStorage contract.
/// Roughly based on the OpenZeppelin implementation.
class ERC721URIStorage : virtual public ERC721 {
  protected:
    /// Solidity: mapping(uint256 tokenId => string) private _tokenURIs;
    SafeUnorderedMap<uint256_t, std::string> _tokenURIs;

    /**
     * Set the token URI for a given token.
     * @param tokenId
     * @param _tokenURI
     * Solidity: function _setTokenURI(uint256 tokenId, string memory _tokenURI) internal virtual
     */
    void setTokenURI_(const uint256_t &tokenId, const std::string &_tokenURI);

    /**
     * Transfers the tokenId from the current owner to the specified address.
     * @param to The address that will receive the token.
     * @param tokenId The id of the token that will be transferred.
     * @param auth The address that is authorized to transfer the token.
     * @return The address of the old
     * If auth is non-zero, the function will check if the auth address is authorized to transfer the token.
     * Solidity counterpart: function _update(address to, uint256 tokenId, address auth) internal returns (address)
     */
    Address update_(const Address& to, const uint256_t& tokenId, const Address& auth) override;

    void registerContractFunctions() override;

  public:
    /**
     * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
     */
    using ConstructorArguments = std::tuple<const std::string&, const std::string&>;

    /**
     * Constructor for loading contract from DB.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    ERC721URIStorage(const Address& address, const DB& db);

    /**
     * Constructor to be used when creating a new contract.
     * @param erc721_name The name of the ERC20 token.
     * @param erc721_symbol The symbol of the ERC20 token.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     */
    ERC721URIStorage(
      const std::string &erc721_name, const std::string &erc721_symbol,
      const Address &address, const Address &creator, const uint64_t &chainId
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param derivedTypeName The name of the derived type.
     * @param erc721_name The name of the ERC20 token.
     * @param erc721_symbol The symbol of the ERC20 token.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     */
    ERC721URIStorage(
      const std::string &derivedTypeName,
      const std::string &erc721_name, const std::string &erc721_symbol,
      const Address &address, const Address &creator, const uint64_t &chainId
    );

    /// Solidity: function tokenURI(uint256 tokenId) public view virtual override returns (string memory)
    virtual std::string tokenURI(const uint256_t &tokenId) const override;

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        ERC721URIStorage, const std::string &, const std::string &,
        const Address &, const Address &, const uint64_t &//,
        //const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc721_name", "erc721_symbol"},
        std::make_tuple("tokenOfOwnerByIndex", &ERC721URIStorage::tokenURI, FunctionTypes::View, std::vector<std::string>{"tokenId"})
      );
    }

    /// Dump method
    DBBatch dump() const override;
};

#endif // ERC721URISTORAGE_H
