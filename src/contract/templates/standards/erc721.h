/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef ERC721_H
#define ERC721_H

#include <memory>

#include "../../../utils/contractreflectioninterface.h"
#include "../../../utils/db.h"
#include "../../../utils/utils.h"
#include "../../abi.h"
#include "../../dynamiccontract.h"
#include "../../variables/safestring.h"
#include "../../variables/safeuint.h"
#include "../../variables/safeunorderedmap.h"

/// Template for an ERC721 contract.
/// Based on OpenZeppelin v5.0.2 ERC721 implementation.
class ERC721 : virtual public DynamicContract {
protected:
  /// Solidity: string internal name_;
  SafeString name_;

  /// Solidity: string internal symbol_;
  SafeString symbol_;

  /// Solidity: mapping(uint256 tokenId => address owner) internal owners_;
  SafeUnorderedMap<uint64_t, Address> owners_;

  /// Solidity: mapping(address => uint256) internal balances_;
  SafeUnorderedMap<Address, uint64_t> balances_;

  /// Solidity: mapping(uint256 => address) internal tokenApp;
  SafeUnorderedMap<uint64_t, Address> tokenApprovals_;

  /// Solidity: mapping(address => mapping(address => bool)) internal operatorAddressApprovals_;
  SafeUnorderedMap<Address, boost::unordered_flat_map<Address, bool, SafeHash>> operatorAddressApprovals_;

  /// Get the baseURI of the contract.
  virtual std::string baseURI_() const { return ""; }

  /**
   * Returns the owner of the Token
   * @param tokenId The id of the token.
   * @return The address of the owner of the token.
   * Solidity counterpart: function ownerOf_(uint256 tokenId) internal view virtual returns (address)
   */
  Address ownerOf_(const uint256_t &tokenId) const;

  /**
   * Returns the approved address for tokenId
   * @param tokenId The id of the token.
   * @return The address of the approved address for tokenId.
   * Solidity counterpart: function getApproved_(uint256 tokenId) internal view virtual returns (address)
   */
  Address getApproved_(const uint256_t &tokenId) const;

  /**
   * Transfers the tokenId from the current owner to the specified address.
   * @param to The address that will receive the token.
   * @param tokenId The id of the token that will be transferred.
   * @param auth The address that is authorized to transfer the token.
   * @return The address of the old
   * If auth is non-zero, the function will check if the auth address is
   * authorized to transfer the token. Solidity counterpart: function
   * update_(address to, uint256 tokenId, address auth) internal returns
   * (address)
   */
  virtual Address update_(const Address &to, const uint256_t &tokenId,
                  const Address &auth);

  /**
   * Check if the specified address is authorized to transfer the specified
   * token. Assuming the provided owner is the actual owner of the token.
   * @param owner The address of the owner of the token.
   * @param spender The address that will spend the tokens.
   * @param tokenId The id of the token.
   * Solidity counterpart: function checkAuthorized_(address owner, address
   * spender, uint256 tokenId) internal view
   */
  void checkAuthorized_(const Address &owner, const Address &spender,
                        const uint256_t &tokenId) const;

  /**
   * Returns whether the specified address is authorized to transfer the
   * specified token. Assuming the provided owner is the actual owner of the
   * token.
   * @param owner The address of the owner of the token.
   * @param spender The address that will spend the tokens.
   * @param tokenId The id of the token.
   * @return Whether the specified address is authorized to transfer the
   * specified token. Solidity counterpart: function isAuthorized_(address
   * owner, address spender, uint256 tokenId) internal view returns (bool)
   */
  bool isAuthorized_(const Address &owner, const Address &spender,
                     const uint256_t &tokenId) const;

  /**
   * Mint a new token.
   * @param to The address that will receive the token.
   * @param tokenId The id of the token that will be minted
   */
  void mint_(const Address &to, const uint256_t &tokenId);

  /**
   * Burn a token.
   * @param tokenId The id of the token that will be burned.
   */
  void burn_(const uint256_t &tokenId);

  /**
   * Transfers tokenId from a specified address to another address.
   * @param from The address that currently owns the token.
   * @param to The address that will receive the token.
   * @param tokenId The id of the token that will be transferred.
   * Solidity Counterpart: function transfer_(address from, address to, uint256
   * tokenId) internal
   */
  void transfer_(const Address &from, const Address &to,
                 const uint256_t &tokenId);

  /**
   * Approve to to operate on tokenId
   * @param to The address that will be approved to operate on tokenId.
   * @param tokenId The id of the token that will be approved.
   * @param auth The address that is authorized to approve the token.
   * @return The address of the old approved address.
   * The auth argument is optional, if the value is non 0 the function will
   * check if auth is either the owner of the token or an approved operatorAddress.
   * Solidity counterpart: function approve_(address to, uint256 tokenId,
   * address auth) internal returns (address)
   */
  Address approve_(const Address &to, const uint256_t &tokenId,
                   const Address &auth);

  /**
   * Set or unset the approval of a third party to transfer of all tokens of a
   * specified address.
   * @param owner The address that owns the tokens.
   * @param operatorAddress The address that will be approved to operate on all tokens
   * of owner.
   * @param approved Whether the operatorAddress is approved or not.
   * Solidity counterpart: function setApprovalForAll_(address owner, address
   * operatorAddress, bool approved) internal
   */
  void setApprovalForAll_(const Address &owner, const Address &operatorAddress, bool approved);

  /**
   * Check if the token has been minted.
   * @param tokenId The id of the token.
   */
  void requireMinted_(const uint256_t &tokenId) const;

  /**
   * Check if the receipient can receive a ERC721 token.
   */
  void checkOnERC721Received_(const Address& from, const Address& to, const uint256_t& tokenId, const Bytes& data);

  void registerContractFunctions() override;

public:
  /**
   * ConstructorArguments is a tuple of the contract constructor arguments in
   * the order they appear in the constructor.
   */
  using ConstructorArguments =
      std::tuple<const std::string &, const std::string &>;

  /**
   * Constructor for loading contract from DB.
   * @param address The address where the contract will be deployed.
   * @param db Reference to the database object.
   */
  ERC721(const Address &address, const DB& db);

  /**
   * Constructor to be used when creating a new contract.
   * @param erc721name The name of the ERC721 token.
   * @param erc721symbol The symbol of the ERC721 token.
   * @param address The address where the contract will be deployed.
   * @param creator The address of the creator of the contract.
   * @param chainId The chain where the contract wil be deployed.
   */
  ERC721(const std::string &erc721name, const std::string &erc721symbol,
    const Address &address, const Address &creator, const uint64_t &chainId
  );

  /**
   * Constructor to be used when creating a new contract.
   * @param derivedTypeName The name of the derived type.
   * @param erc721name The name of the ERC721 token.
   * @param erc721symbol The symbol of the ERC721 token.
   * @param address The address where the contract will be deployed.
   * @param creator The address of the creator of the contract.
   * @param chainId The chain where the contract wil be deployed.
   */
  ERC721(const std::string &derivedTypeName, const std::string &erc721name,
    const std::string &erc721symbol, const Address &address,
    const Address &creator, const uint64_t &chainId
  );

  /// Destructor.
  ~ERC721() override = default;


  /// Event for when a token is transferred.
  /// event Transfer(address indexed from, address indexed to, uint256 indexed tokenId);
  void Transfer(const EventParam<Address, true> &from,
                const EventParam<Address, true> &to,
                const EventParam<uint256_t, true> &tokenId) {
    this->emitEvent("Transfer", std::make_tuple(from, to, tokenId));
  }

  /// Event for when a token is approved.
  /// event Approval(address indexed owner, address indexed approved, uint256 indexed tokenId);
  void Approval(const EventParam<Address, true> &owner,
                const EventParam<Address, true> &approved,
                const EventParam<uint256_t, true> &tokenId) {
    this->emitEvent("Approval", std::make_tuple(owner, approved, tokenId));
  }

  /// Event for when an operator is approved.
  /// event ApprovalForAll(address indexed owner, address indexed operator, bool approved);
  void ApprovalForAll(const EventParam<Address, true> &owner,
                      const EventParam<Address, true> &operatorAddress,
                      const EventParam<bool, false> &approved) {
    this->emitEvent("ApprovalForAll", std::make_tuple(owner, operatorAddress, approved));
  }

  /**
   * Get the name of the ERC721 token.
   * Solidity counterpart: function name() external view returns (string
   * memory);
   * @return The name of the ERC721 token.
   */
  std::string name() const;

  /**
   * Get the symbol of the ERC721 token.
   * Solidity counterpart: function symbol() external view returns (string
   * memory);
   * @return The symbol of the ERC721 token.
   */
  std::string symbol() const;

  /**
   * Get the balance of a specific address.
   * Solidity counterpart: function balanceOf(address owner) external view
   * returns (uint256 balance);
   * @param owner The address to query the balance of.
   * @return The balance of the specified address.
   */
  uint256_t balanceOf(const Address &owner) const;

  /**
   * Get the owner address of a specific tokenId.
   * Solidity counterpart: function ownerOf(uint256 tokenId) external view
   * returns (address owner);
   * @param tokenId The tokenId to query the owner of.
   * @return The owner address of the specified tokenId.
   */
  Address ownerOf(const uint256_t &tokenId) const;

  /**
   * Get the URI of a specific tokenId.
   * Solidity counterpart: function tokenURI(uint256 tokenId) external view
   * returns (string memory);
   * @param tokenId The tokenId to query the URI of.
   * @return The URI of the specified tokenId.
   */
  virtual std::string tokenURI(const uint256_t &tokenId) const;

  /**
   * Approve a token to be transferred by a third party.
   * Solidity counterpart: function approve(address to, uint256 tokenId)
   * external;
   * @param to The address to approve the transfer to.
   * @param tokenId The tokenId to approve the transfer of.
   */
  void approve(const Address &to, const uint256_t &tokenId);

  /**
   * Get the approved address for a specific tokenId.
   * Solidity counterpart: function getApproved(uint256 tokenId) external view
   * returns (address operatorAddress);
   * @param tokenId The tokenId to query the approved address of.
   * @return The approved address for the specified tokenId.
   */
  Address getApproved(const uint256_t &tokenId) const;

  /**
   * Set or unset the approval of all tokens to be transferred by a third party.
   * Solidity counterpart: function setApprovalForAll(address operatorAddress, bool
   * approved) external;
   * @param operatorAddress The address to set the approval for.
   * @param approved Whether the address is approved or not.
   */
  void setApprovalForAll(const Address &operatorAddress, const bool &approved);

  /**
   * Get the approval status of all tokens to be transferred by a third party.
   * Solidity counterpart: function isApprovedForAll(address owner, address
   * operatorAddress) external view returns (bool);
   * @param owner The address to query the approval status for.
   * @param operatorAddress The address to query the approval status of.
   * @return The approval status of all tokens to be transferred by a third
   * party.
   */
  bool isApprovedForAll(const Address &owner, const Address &operatorAddress) const;

  /**
   * Transfer a token from one address to another.
   * Solidity counterpart: function transferFrom(address from, address to,
   * uint256 tokenId) external;
   * @param from The address to transfer the token from.
   * @param to The address to transfer the token to.
   * @param tokenId The tokenId to transfer.
   */
  void transferFrom(const Address &from, const Address &to,
                    const uint256_t &tokenId);

  /**
   * Safely transfer a token from one address to another.
   * Solidity counterpart: function safeTransferFrom(address from, address to, uint256 tokenId, bytes calldata data) external;
   * @param from The address to transfer the token from.
   * @param to The address to transfer the token to.
   * @param tokenId The tokenId to transfer.
   * @param data The data to pass to the recipient contract.
   */
  void safeTransferFrom(const Address &from, const Address &to, const uint256_t &tokenId, const Bytes& data);

  /**
   * Safely transfer a token from one address to another.
   * Solidity counterpart: function safeTransferFrom(address from, address to, uint256 tokenId) external;
   * @param from The address to transfer the token from.
   * @param to The address to transfer the token to.
   * @param tokenId The tokenId to transfer.
   */
  void safeTransferFrom(const Address &from, const Address &to, const uint256_t &tokenId);

  /// Register contract class via ContractReflectionInterface.
  static void registerContract() {
    static std::once_flag once;
    std::call_once(once, []() {
      DynamicContract::registerContractMethods<ERC721>(
          std::vector<std::string>{"erc721name", "erc721symbol"},
          std::make_tuple("name", &ERC721::name, FunctionTypes::View,
                          std::vector<std::string>{}),
          std::make_tuple("symbol", &ERC721::symbol, FunctionTypes::View,
                          std::vector<std::string>{}),
          std::make_tuple("balanceOf", &ERC721::balanceOf, FunctionTypes::View,
                          std::vector<std::string>{"owner"}),
          std::make_tuple("ownerOf", &ERC721::ownerOf, FunctionTypes::View,
                          std::vector<std::string>{"tokenId"}),
          std::make_tuple("tokenURI", &ERC721::tokenURI, FunctionTypes::View,
                          std::vector<std::string>{"tokenId"}),
          std::make_tuple("approve", &ERC721::approve, FunctionTypes::NonPayable,
                          std::vector<std::string>{"to", "tokenId"}),
          std::make_tuple("getApproved", &ERC721::getApproved, FunctionTypes::View,
                          std::vector<std::string>{"tokenId"}),
          std::make_tuple("setApprovalForAll", &ERC721::setApprovalForAll,
                          FunctionTypes::NonPayable,
                          std::vector<std::string>{"operatorAddress", "approved"}),
          std::make_tuple("isApprovedForAll", &ERC721::isApprovedForAll, FunctionTypes::View,
                          std::vector<std::string>{"owner", "operatorAddress"}),
          std::make_tuple("transferFrom", &ERC721::transferFrom, FunctionTypes::NonPayable,
                          std::vector<std::string>{"from", "to", "tokenId"}),
          std::make_tuple("safeTransferFrom",
                          static_cast<void(ERC721::*)(const Address&, const Address&, const uint256_t&, const Bytes&)>(&ERC721::safeTransferFrom),
                          FunctionTypes::NonPayable,
                          std::vector<std::string>{"from", "to", "tokenId", "data"}),
          std::make_tuple("safeTransferFrom",
                          static_cast<void(ERC721::*)(const Address&, const Address&, const uint256_t&)>(&ERC721::safeTransferFrom),
                          FunctionTypes::NonPayable,
                          std::vector<std::string>{"from", "to", "tokenId"}));
      ContractReflectionInterface::registerContractEvents<ERC721>(
          std::make_tuple("Transfer", false, &ERC721::Transfer, std::vector<std::string>{"from","to", "tokenId"}),
          std::make_tuple("Approval", false, &ERC721::Approval, std::vector<std::string>{"owner","approved", "tokenId"}),
          std::make_tuple("ApprovalForAll", false, &ERC721::ApprovalForAll, std::vector<std::string>{"owner","operatorAddress", "approved"})
      );
    });
  }

   /// Dump method
   DBBatch dump() const override;
};

#endif // ERC721_H
