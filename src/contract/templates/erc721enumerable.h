#ifndef ERC721ENUMERABLE_H
#define ERC721ENUMERABLE_H

#include "erc721.h"
#include "../variables/safevector.h"

/// Template for an ERC721Enumerable contract.
/// Roughly based on the OpenZeppelin implementation.
class ERC721Enumerable : virtual public ERC721 {
  protected:

    /// Solidity: mapping(address owner => mapping(uint256 index => uint256)) private _ownedTokens;
    SafeUnorderedMap<Address, std::unordered_map<uint256_t, uint256_t>> _ownedTokens;

    /// Solidity: mapping(uint256 tokenId => uint256) private _ownedTokensIndex;
    SafeUnorderedMap<uint256_t, uint256_t> _ownedTokensIndex;

    /// Solidity: uint256[] private _allTokens;
    SafeVector<uint256_t> _allTokens;

    /// Solidity: mapping(uint256 => uint256) private _allTokensIndex;
    SafeUnorderedMap<uint256_t, uint256_t> _allTokensIndex;

    /**
     * Transfers the tokenId from the current owner to the specified address.
     * @param to The address that will receive the token.
     * @param tokenId The id of the token that will be transferred.
     * @param auth The address that is authorized to transfer the token.
     * @return The address of the old
     * If auth is non-zero, the function will check if the auth address is authorized to transfer the token.
     * Solidity counterpart: function _update(address to, uint256 tokenId, address auth) internal returns (address)
     */
    virtual Address _update(const Address& to, const uint256_t& tokenId, const Address& auth) override;

    /**
     * Adds a token to the list of tokens owned by the specified address.
     * @param to
     * @param tokenId
     * Solidity: function _addTokenToOwnerEnumeration(address to, uint256 tokenId) private
     */
    void _addTokenToOwnerEnumeration(const Address& to, const uint256_t& tokenId);

    /**
     * Adds a token to the list of all tokens.
     * @param tokenId
     * Solidity: function _addTokenToAllTokensEnumeration(uint256 tokenId) private
     */
    void _addTokenToAllTokensEnumeration(const uint256_t& tokenId);

    /**
     * Removes a token from the list of tokens owned by the specified address.
     * @param from
     * @param tokenId
     * Solidity: function _removeTokenFromOwnerEnumeration(address from, uint256 tokenId) private
     */
    void _removeTokenFromOwnerEnumeration(const Address& from, const uint256_t& tokenId);

    /**
     * Removes a token from the list of all tokens.
     * @param tokenId
     * Solidity: function _removeTokenFromAllTokensEnumeration(uint256 tokenId) private
     */
    void _removeTokenFromAllTokensEnumeration(const uint256_t& tokenId);

    /**
     * Increase the balance of the specified account by the specified amount.
     * @param account
     * @param amount
     * Solidity: function _increaseBalance(address account, uint128 amount) internal virtual override
     */
    virtual void _increaseBalance(const Address& account, const uint128_t& amount) override;

    virtual void registerContractFunctions() override;

  public:

    /**
     * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
     */
    using ConstructorArguments = std::tuple<const std::string&, const std::string&>;


    /**
     * Constructor for loading contract from DB.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    ERC721Enumerable(
      ContractManagerInterface& interface,
      const Address& address, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param erc721_name The name of the ERC20 token.
     * @param erc721_symbol The symbol of the ERC20 token.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    ERC721Enumerable(
      const std::string &erc721_name, const std::string &erc721_symbol,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    /**
     * Constructor to be used when creating a new contract.
     * @param erc721_name The name of the ERC20 token.
     * @param erc721_symbol The symbol of the ERC20 token.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     * @param db Reference to the database object.
     */
    ERC721Enumerable(
      const std::string &derivedTypeName,
      const std::string &erc721_name, const std::string &erc721_symbol,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    virtual ~ERC721Enumerable() override;

    /** Returns a token ID owned by owner at a given index of its token list
     * Solidity: function tokenOfOwnerByIndex(address owner, uint256 index) public view virtual returns (uint256)
     * @param owner
     * @param index
     * @return Amount of tokens owned by the owner.
     */
    virtual uint256_t tokenOfOwnerByIndex(const Address& owner, const uint256_t& index) const;

    /** Returns the total amount of tokens stored by the contract.
     * Solidity: function totalSupply() public view virtual returns (uint256)
     * @return Total amount of tokens.
     */
    virtual uint256_t totalSupply() const;

    /**
     * Get a token ID at a given index of all the tokens stored by the contract.
     * @param index
     * Solidity: function tokenByIndex(uint256 index) public view virtual returns (uint256)
     */
    virtual uint256_t tokenByIndex(const uint256_t& index) const;

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ERC721Enumerable, const std::string &, const std::string &,
        ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc721_name", "erc721_symbol"},
        std::make_tuple("tokenOfOwnerByIndex", &ERC721Enumerable::tokenOfOwnerByIndex, "view", std::vector<std::string>{"owner","index"}),
        std::make_tuple("totalSupply", &ERC721Enumerable::totalSupply, "view", std::vector<std::string>{}),
        std::make_tuple("tokenByIndex", &ERC721Enumerable::tokenByIndex, "view", std::vector<std::string>{"index"})
      );
    }
};



#endif  // ERC721ENUMERABLE_H