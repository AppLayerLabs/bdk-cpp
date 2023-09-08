#ifndef PULSARNFT_H
#define PULSARNFT_H

#include "erc721enumerable.h"
#include "erc721royalty.h"
#include "erc721uristorage.h"
#include "accesscontrol.h"
#include "../variables/counters.h"
#include "../variables/pausable.h"
#include "../variables/safeenumerablemap.h"

class PulsarNft : virtual public DynamicContract,
  virtual public ERC721, virtual public ERC721Enumerable,
  virtual public ERC721URIStorage, virtual public ERC721Royalty, virtual public AccessControl {
protected:
  /// Solidity: bytes32 public constant OPERATOR = keccak256("OPERATOR");
  const Hash OPERATOR_ = Hash(Hex::toBytes("523a704056dcd17bcf83bed8b68c59416dac1119be77755efe3bde0a64e46e0c"));
  Counter _tokenIdCounter;
  SafeString _baseUri;
  SafeUint_t<256> creationBlock_;
  SafeUnorderedMap<uint256_t, bool> _tokenIdIsTransferable;
  SafeUnorderedMap<Address, bool> _frozenAccounts;
  SafeEnumerableMap<uint256_t, uint256_t> _nfts;
  SafeUnorderedMap<uint256_t, Bytes> nftAttributes_;
  struct UserInfo {
    Address user;
    uint64_t expires;
  };
  SafeUnorderedMap<uint256_t, UserInfo> _users;
  Pausable::PausableActor pausableActor_;

  std::string _baseURI() const override;

  std::string _createUriBasedOnId(const uint256_t &baseNftId, uint256_t tokenId);

  void registerContractFunctions() override;

  /// Logic of _transfer + _beforeTokenTransfer + _burn + _mintNft
  Address _update(const Address &to, const uint256_t &tokenId, const Address &auth) override;

  bool initialized;

public:
  /**
   * ConstructorArguments is a tuple of the contract constructor arguments in the order they appear in the constructor.
   */
  using ConstructorArguments = std::tuple<>;

  /**
   * Constructor for loading contract from DB.
   * @param interface Reference to the contract manager interface.
   * @param contractAddress The address where the contract will be deployed.
   * @param db Reference pointer to the database object.
   */
  PulsarNft(
    ContractManagerInterface &interface,
    const Address &contractAddress, const std::unique_ptr<DB> &db
  );

  /**
   * Constructor for building a new contract from scratch.
   * @param interface Reference to the contract manager interface.
   * @param address The address where the contract will be deployed.
   * @param creator The address of the creator of the contract.
   * @param chainId The chain id of the contract.
   * @param db Reference pointer to the database object.
   */
  PulsarNft(
    ContractManagerInterface &interface,
    const Address &address, const Address &creator,
    const uint64_t &chainId, const std::unique_ptr<DB> &db
  );

  virtual ~PulsarNft() override;


  void initialize(const std::string &name,
                  const std::string &symbol,
                  const std::string &baseUri,
                  const Address &feesCollector,
                  std::vector<Address> operators);

  const Hash OPERATOR() const;

  uint256_t creationBlock() const;

  Bytes nftAttributes(const uint256_t &tokenId) const;

  void setDefaultRoyalty(uint96_t royalty);

  void deleteDefaultRoyalty();

  void setBaseURI(const std::string &baseUri);

  void pause();

  void unpause();

  void setAccountFreezed(const Address &addr, bool value);

  bool isAccountFreezed(const Address &addr) const;

  uint256_t getNftsLength() const;

  /// std::tuple<uint256_t,uint256_t,address>
  BytesEncoded getNftByIndex(const uint256_t &index) const;

  /// std::tuple<bool, uint256_t>
  BytesEncoded getNft(const uint256_t &_tokenId) const;

  void mintFor(const Address &user, uint256_t quantity, const std::string &mintingBlob);

  void mintNftsWithAmount(uint256_t baseNftId, uint256_t amount, Address to, bool isAbleToTransfer);

  void mintNft(const Address &to, uint256_t baseNftId, bool isAbleToTransfer);

  void burnNfts(const std::vector<uint256_t> &tokenIds);

  void burnNftRanger(uint256_t fromTokenIndex, uint256_t toTokenIndex);

  void burnNft(uint256_t tokenId);

  std::string tokenURI(const uint256_t &tokenId) const override;

  void setAttribute(const uint256_t &tokenId, const Bytes &value);

  Bytes getAttributes(const uint256_t &tokenId) const;

  void removeAttributesFromTokenId(const uint256_t &tokenId);

  void setUser(const uint256_t &tokenId, const Address &user, const uint64_t &expires);

  Address userOf(const uint256_t &tokenId);

  uint256_t userExpires(const uint256_t &tokenId);

  static void registerContract() {
    ContractReflectionInterface::registerContract<
      PulsarNft,
      ContractManagerInterface &,
      const Address &, const Address &, const uint64_t &,
      const std::unique_ptr<DB> &
    >(
      std::vector<std::string>{},
      std::make_tuple("OPERATOR", &PulsarNft::OPERATOR, "view", std::vector<std::string>{}),
      std::make_tuple("creationBlock", &PulsarNft::creationBlock, "view", std::vector<std::string>{}),
      std::make_tuple("nftAttributes", &PulsarNft::nftAttributes, "view", std::vector<std::string>{"tokenId"}),
      std::make_tuple("setDefaultRoyalty", &PulsarNft::setDefaultRoyalty, "nonpayable",
                      std::vector<std::string>{"royalty"}),
      std::make_tuple("deleteDefaultRoyalty", &PulsarNft::deleteDefaultRoyalty, "nonpayable",
                      std::vector<std::string>{}),
      std::make_tuple("setBaseURI", &PulsarNft::setBaseURI, "nonpayable", std::vector<std::string>{"baseUri"}),
      std::make_tuple("pause", &PulsarNft::pause, "nonpayable", std::vector<std::string>{}),
      std::make_tuple("unpause", &PulsarNft::unpause, "nonpayable", std::vector<std::string>{}),
      std::make_tuple("setAccountFreezed", &PulsarNft::setAccountFreezed, "nonpayable",
                      std::vector<std::string>{"addr", "value"}),
      std::make_tuple("isAccountFreezed", &PulsarNft::isAccountFreezed, "view", std::vector<std::string>{"addr"}),
      std::make_tuple("getNftsLength", &PulsarNft::getNftsLength, "view", std::vector<std::string>{}),
      std::make_tuple("getNftByIndex", &PulsarNft::getNftByIndex, "view", std::vector<std::string>{"index"}),
      std::make_tuple("getNft", &PulsarNft::getNft, "view", std::vector<std::string>{"tokenId"}),
      std::make_tuple("mintFor", &PulsarNft::mintFor, "nonpayable",
                      std::vector<std::string>{"user", "quantity", "mintingBlob"}),
      std::make_tuple("mintNftsWithAmount", &PulsarNft::mintNftsWithAmount, "nonpayable",
                      std::vector<std::string>{"baseNftId", "amount", "to", "isAbleToTransfer"}),
      std::make_tuple("mintNft", &PulsarNft::mintNft, "nonpayable",
                      std::vector<std::string>{"to", "baseNftId", "isAbleToTransfer"}),
      std::make_tuple("burnNfts", &PulsarNft::burnNfts, "nonpayable", std::vector<std::string>{"tokenIds"}),
      std::make_tuple("burnNftRanger", &PulsarNft::burnNftRanger, "nonpayable",
                      std::vector<std::string>{"fromTokenIndex", "toTokenIndex"}),
      std::make_tuple("burnNft", &PulsarNft::burnNft, "nonpayable", std::vector<std::string>{"tokenId"}),
      std::make_tuple("tokenURI", &PulsarNft::tokenURI, "view", std::vector<std::string>{"tokenId"}),
      std::make_tuple("setAttribute", &PulsarNft::setAttribute, "nonpayable",
                      std::vector<std::string>{"tokenId", "value"}),
      std::make_tuple("getAttributes", &PulsarNft::getAttributes, "view", std::vector<std::string>{"tokenId"}),
      std::make_tuple("removeAttributesFromTokenId", &PulsarNft::removeAttributesFromTokenId, "nonpayable",
                      std::vector<std::string>{"tokenId"}),
      std::make_tuple("setUser", &PulsarNft::setUser, "nonpayable",
                      std::vector<std::string>{"tokenId", "user", "expires"}),
      std::make_tuple("userOf", &PulsarNft::userOf, "view", std::vector<std::string>{"tokenId"}),
      std::make_tuple("userExpires", &PulsarNft::userExpires, "view", std::vector<std::string>{"tokenId"})
    );
  };
};




#endif
