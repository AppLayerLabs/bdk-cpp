#ifndef ERC721_TEST
#define ERC721_TEST


// ERC721Test derives from base ERC721
#include "erc721.h"


/*
 * ERC721Test testing class
 * This is a class to test the capabilities of the ERC721 template contract
 * The ERC721 contract is based on the OpenZeppelin ERC721 implementation
 * As the ERC721 (OpenZeppelin) contract does not have a public function for minting and burning the tokens
 * this wrapper class is used to make that functions available.
 * The mint function will use a internal counter to generate the token id
 * Anyone can mint a token and there is a limit of X tokens defined in the constructor
 * The burn function will use the token id to burn the token, the sender of the burn transaction MUST be the owner of the token
 * OR an approved operator for the token (All these cases are included in the tests) (the ERC721::_update function is used to check ownership and allowance)
 */

class ERC721Test : public ERC721 {
  private:
    SafeUint64_t tokenIdCounter_; ///< TokenId Counter for the public mint() functions.
    SafeUint64_t maxTokens_; ///< How many tokens can be minted (used by mint()).
    SafeUint64_t totalSupply_; ///< How many tokens exist.
    void registerContractFunctions() override; ///< Register contract functions.

  public:
    /**
     * ConstructorArguments is a tuple of the contract constructor arguments in
     * the order they appear in the constructor.
     */
    using ConstructorArguments =
       std::tuple<const std::string &, const std::string &, const uint64_t&>;

    /**
     * Constructor for loading contract from DB.
     * @param address The address where the contract will be deployed.
     * @param db Reference to the database object.
     */
    ERC721Test(const Address &address, const DB& db);

    /**
     * Constructor to be used when creating a new contract.
     * @param erc721name The name of the ERC721 token.
     * @param erc721symbol The symbol of the ERC721 token.
     * @param maxTokens The maximum amount of tokens that can be minted
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain where the contract wil be deployed.
     */
    ERC721Test(
      const std::string &erc721name, const std::string &erc721symbol, const uint64_t& maxTokens,
      const Address &address, const Address &creator, const uint64_t &chainId
    );

    /// Destructor.
    ~ERC721Test() override = default;

    /**
     * Mint a single token to the to address.
     * @param to Address to send the token to.
     * The mint function will use the internal tokenIdCounter to generate the token id
     */
    void mint(const Address& to);

    /**
     * Burn a single token given that token id
     * @param tokenId The token id to burn
     * The called of this function should be the owner of the token or an approved operator
     */
    void burn(const uint256_t& tokenId);

    /// Getter for the tokenIdCounter_
    uint64_t tokenIdCounter() const {
      return tokenIdCounter_.get();
    }

    /// Getter for the maxTokens_
    uint64_t maxTokens() const {
      return maxTokens_.get();
    }

    /// Getter for the totalSupply_
    uint64_t totalSupply() const {
      return totalSupply_.get();
    }

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContractMethods<
        ERC721Test, const std::string &, const std::string &, const uint64_t &,
        const Address &, const Address &,
        const uint64_t &, DB&>
      (
        std::vector<std::string>{"erc721name", "erc721symbol", "maxTokens"},
        std::make_tuple("mint", &ERC721Test::mint, FunctionTypes::NonPayable, std::vector<std::string>{"to"}),
        std::make_tuple("burn", &ERC721Test::burn, FunctionTypes::NonPayable, std::vector<std::string>{"tokenId"}),
        std::make_tuple("tokenIdCounter", &ERC721Test::tokenIdCounter, FunctionTypes::View, std::vector<std::string>{""}),
        std::make_tuple("maxTokens", &ERC721Test::maxTokens, FunctionTypes::View, std::vector<std::string>{""}),
        std::make_tuple("totalSupply", &ERC721Test::totalSupply, FunctionTypes::View, std::vector<std::string>{""})
      );
    }

    /// Dump method
    DBBatch dump() const override;
};






#endif // ERC721_TEST
