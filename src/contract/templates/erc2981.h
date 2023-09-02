#ifndef ERC2981_H
#define ERC2981_H

#include "../dynamiccontract.h"
#include "../variables/safeaddress.h"
#include "../variables/safeuint.h"
#include "../variables/safeunorderedmap.h"

struct RoyaltyInfo {
  Address recipient;
  uint96_t royaltyFraction;
  BytesEncoded encode() const {
    BytesEncoded ret;
    ABI::Encoder encoder({recipient, uint256_t(royaltyFraction)});
    ret.data = encoder.getData();
    return ret;
  }
  static std::tuple<Address, uint256_t> decode(const BytesEncoded& bytes) {
    ABI::Decoder decoder({ABI::Types::address, ABI::Types::uint256}, bytes.data);
    return std::make_tuple<Address, uint256_t>(decoder.getData<Address>(0), decoder.getData<uint256_t>(1));
  }
};

/// Helper class for the ERC2981 royalty info.
struct SafeRoyaltyInfo {
  SafeAddress recipient;
  SafeUint_t<96> royaltyFraction;
  SafeRoyaltyInfo(DynamicContract* contract) : recipient(contract), royaltyFraction(contract) {}
  BytesEncoded encode() const {
    BytesEncoded ret;
    ABI::Encoder encoder({recipient.get(), uint256_t(royaltyFraction.get())});
    ret.data = encoder.getData();
    return ret;
  }
};

class ERC2981 : virtual public DynamicContract {
  protected:
    /// Solidity: RoyaltyInfo private _defaultRoyaltyInfo;
    SafeRoyaltyInfo _defaultRoyaltyInfo;

    /// Solidity: mapping(uint256 tokenId => RoyaltyInfo) private _tokenRoyaltyInfo;
    SafeUnorderedMap<uint256_t, RoyaltyInfo> _tokenRoyaltyInfo;

    /// Solidity: function _feeDenominator() internal pure virtual returns (uint96)
    virtual uint96_t _feeDenominator() const;

    /// Solidity: function _setDefaultRoyalty(address receiver, uint96 feeNumerator) internal virtual
    virtual void _setDefaultRoyalty(const Address& receiver, const uint96_t& feeNumerator);

    /// Solidity: function _deleteDefaultRoyalty() internal virtual
    virtual void _deleteDefaultRoyalty();

    /// Solidity: function _setTokenRoyalty(uint256 tokenId, address receiver, uint96 feeNumerator) internal virtual
    virtual void _setTokenRoyalty(const uint256_t& tokenId, const Address& receiver, const uint96_t& feeNumerator);

    /// Solidity: function _resetTokenRoyalty(uint256 tokenId) internal virtual
    virtual void _resetTokenRoyalty(const uint256_t& tokenId);

    virtual void registerContractFunctions() override;
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
    ERC2981(
      ContractManagerInterface& interface,
      const Address& contractAddress, const std::unique_ptr<DB>& db
    );

    /**
     * Constructor for building a new contract from scratch.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain id of the contract.
     * @param db Reference pointer to the database object.
     */
    ERC2981(
      ContractManagerInterface& interface,
      const Address& address, const Address& creator,
      const uint64_t& chainId, const std::unique_ptr<DB>& db
    );

    /**
     * Construtor for building a new contract from scratch with a derived type name.
     * @param derivedTypeName The name of the derived type.
     * @param interface Reference to the contract manager interface.
     * @param address The address where the contract will be deployed.
     * @param creator The address of the creator of the contract.
     * @param chainId The chain id of the contract.
     * @param db Reference pointer to the database object.
     */
    ERC2981(
      const std::string& derivedTypeName,
      ContractManagerInterface& interface,
      const Address& address, const Address& creator,
      const uint64_t& chainId, const std::unique_ptr<DB>& db
    );

    virtual ~ERC2981() override;

    /// Solidity: function royaltyInfo(uint256 tokenId, uint256 salePrice) public view virtual returns (address, uint256)
    virtual BytesEncoded royaltyInfo(const uint256_t& tokenId, const uint256_t& salePrice) const;

    /// Register contract class via ContractReflectionInterface.
    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ERC2981, ContractManagerInterface&,
        const Address&, const Address&, const uint64_t&,
        const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{},
        std::make_tuple("royaltyInfo", &ERC2981::royaltyInfo, "view", std::vector<std::string>{"tokenId", "salePrice"})
      );
    }

};




#endif // ERC2981_H