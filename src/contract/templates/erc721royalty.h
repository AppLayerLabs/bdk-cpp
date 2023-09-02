#ifndef ERC721ROYALTY_H
#define ERC721ROYALTY_H

#include "erc721.h"
#include "erc2981.h"


class ERC721Royalty : virtual public ERC721, virtual public ERC2981 {
  protected:
    /// Solidity: function _update(address to, uint256 tokenId, address auth) internal virtual override returns (address) {
    virtual Address _update(const Address& to, const uint256_t& tokenId, const Address& auth) override;

    void registerContractFunctions() override;

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
    ERC721Royalty(
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
    ERC721Royalty(
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
    ERC721Royalty(
      const std::string &derivedTypeName,
      const std::string &erc721_name, const std::string &erc721_symbol,
      ContractManagerInterface &interface,
      const Address &address, const Address &creator, const uint64_t &chainId,
      const std::unique_ptr<DB> &db
    );

    virtual ~ERC721Royalty() override;


    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ERC721Royalty, const std::string &, const std::string &,
        ContractManagerInterface &,
        const Address &, const Address &, const uint64_t &,
        const std::unique_ptr<DB> &
      >(
        std::vector<std::string>{"erc721_name", "erc721_symbol"}
      );
    }
};

#endif // ERC721ROYALTY_H