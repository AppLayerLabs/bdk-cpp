/*
Copyright (c) [2023] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#ifndef THROWTESTC_H
#define THROWTESTC_H

#include "../dynamiccontract.h"
#include "../../utils/utils.h"

class ThrowTestC : public DynamicContract {
  private:
    SafeUint8_t num_;
    void registerContractFunctions() override;

  public:
    using ConstructorArguments = std::tuple<>;

    ThrowTestC(ContractManagerInterface &interface, const Address& address,
      const Address& creator, const uint64_t& chainId, const std::unique_ptr<DB> &db
    );

    ThrowTestC(ContractManagerInterface &interface, const Address& address, const std::unique_ptr<DB> &db);

    // Destructor.
    ~ThrowTestC() override;

    uint8_t getNumC() const;
    void setNumC(const uint8_t& valC);

    static void registerContract() {
      ContractReflectionInterface::registerContract<
        ThrowTestC, ContractManagerInterface&, const Address&, const Address&, const uint64_t&, const std::unique_ptr<DB>&
      >(
        std::vector<std::string>{},
        std::make_tuple("getNumC", &ThrowTestC::getNumC, "view", std::vector<std::string>{}),
        std::make_tuple("setNumC", &ThrowTestC::setNumC, "nonpayable", std::vector<std::string>{"valC"})
      );
    }
};

#endif  // THROWTESTB_H
