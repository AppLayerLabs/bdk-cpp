#include "erc721.h"

class IERC721Receiver : public DynamicContract {
  public:
    static const Bytes4& onERC721ReceiverSelector() {
      const static Bytes4 selector = Bytes4(Hex::toBytes("0x150b7a02"));
      return selector;
    }
    Bytes4 onERC721Received(const Address& op,const Address& from,const uint256_t& tokenId,const Bytes& data) {
      return Bytes4();
    }
    void static registerContract() {
      ContractReflectionInterface::registerContractMethods<
        IERC721Receiver
      >(
        std::vector<std::string>{},
        std::make_tuple("onERC721Received", &IERC721Receiver::onERC721Received, FunctionTypes::NonPayable, std::vector<std::string>{"operator", "from", "tokenId", "data"})
      );
  }
};