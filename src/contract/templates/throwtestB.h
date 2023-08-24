#ifndef THROWTESTB_H
#define THROWTESTB_H

#include "../dynamiccontract.h"
#include "../../utils/utils.h"
#include "throwtestC.h"

class ThrowTestB : public DynamicContract {
  private:
    SafeUint8_t num;

  public:
    using ConstructorArguments = std::tuple<>;

    ThrowTestB(ContractManagerInterface &interface, const Address& address,
      const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    );

    ThrowTestB(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db);

    // Destructor.
    ~ThrowTestB() override;

    uint8_t getNum() const;
    void setNum(const uint8_t& valB, const Address& addC, const uint8_t& valC);

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ThrowTestB, ContractManagerInterface&, const Address&, const Address&, const uint64_t&, const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{"num"},
        std::make_tuple("getNum", &ThrowTestB::getNum, "view", std::vector<std::string>{}),
        std::make_tuple("setNum", &ThrowTestB::setNum, "nonpayable", std::vector<std::string>{"valB", "addC", "valC"})
      );
    }
};

#endif  // THROWTESTB_H
