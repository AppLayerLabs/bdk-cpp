#ifndef THROWTESTA_H
#define THROWTESTA_H

#include "../dynamiccontract.h"
#include "../../utils/utils.h"
#include "throwtestB.h"

class ThrowTestA : public DynamicContract {
  private:
    SafeUint8_t num;

  public:
    using ConstructorArguments = std::tuple<>;

    ThrowTestA(ContractManagerInterface &interface, const Address& address,
      const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    );

    ThrowTestA(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db);

    // Destructor.
    ~ThrowTestA() override;

    uint8_t getNum() const;
    void setNum(const uint8_t& valA,
      const Address& addB, const uint8_t& valB,
      const Address& addC, const uint8_t& valC
    );

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ThrowTestA, ContractManagerInterface&, const Address&, const Address&, const uint64_t&, const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{"num"},
        std::make_tuple("getNum", &ThrowTestA::getNum, "view", std::vector<std::string>{}),
        std::make_tuple("setNum", &ThrowTestA::setNum, "nonpayable", std::vector<std::string>{"valA", "addB", "valB", "addC", "valC"})
      );
    }
};

#endif  // THROWTESTA_H
