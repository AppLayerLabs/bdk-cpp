#ifndef _CORE_TYPEDEFS_H_
#define _CORE_TYPEDEFS_H_

// Shared typedefs between State and ContractManager.
// (ContractManager is a stateless modular extension of State and takes
// a reference of these types in its ctor; all valid machine states have
// exactly one ContractManager in State::contracts_).

using CreateContractFuncsType =
  boost::unordered_flat_map<
    Functor,
    std::function<
      void(
        const evmc_message&,
        const Address&,
        boost::unordered_flat_map<Address, std::unique_ptr<BaseContract>, SafeHash, SafeCompare>& contracts_,
        const uint64_t&,
        ContractHost*
      )>,
    SafeHash
  >;

using ContractsContainerType =
  boost::unordered_flat_map<
    Address,
    std::unique_ptr<BaseContract>,
    SafeHash,
    SafeCompare
>;

#endif // _CORE_TYPEDEFS_H_