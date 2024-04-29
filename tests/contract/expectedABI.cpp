/*
Copyright (c) [2023-2024] [Sparq Network]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "../../src/libs/json.hpp"

using json = nlohmann::ordered_json; ///< Typedef for json.

/// Namespace for testing expected contract ABI generation outputs
namespace EXPECTED {
  namespace ERC20 {
    json transferFrom = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "from"}, {"type", "address"} },
        { {"internalType", "address"}, {"name", "to"}, {"type", "address"} },
        { {"internalType", "uint256"}, {"name", "value"}, {"type", "uint256"} }
      }},
      {"name", "transferFrom"},
      {"outputs", {
        { {"internalType", "bool"}, {"name", ""}, {"type", "bool"} }
      }},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json approve = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "spender"}, {"type", "address"} },
        { {"internalType", "uint256"}, {"name", "value"}, {"type", "uint256"} }
      }},
      {"name", "approve"},
      {"outputs", {
        { {"internalType", "bool"}, {"name", ""}, {"type", "bool"} }
      }},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json balanceOf = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "owner"}, {"type", "address"} }
      }},
      {"name", "balanceOf"},
      {"outputs", {
        { {"internalType", "uint256"}, {"name", ""}, {"type", "uint256"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json totalSupply = {
      {"inputs", json::array()},
      {"name", "totalSupply"},
      {"outputs", {
        { {"internalType", "uint256"}, {"name", ""}, {"type", "uint256"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json transfer = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "to"}, {"type", "address"} },
        { {"internalType", "uint256"}, {"name", "value"}, {"type", "uint256"} }
      }},
      {"name", "transfer"},
      {"outputs", {
        { {"internalType", "bool"}, {"name", ""}, {"type", "bool"} }
      }},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json decimals = {
      {"inputs", json::array()},
      {"name", "decimals"},
      {"outputs", {
        { {"internalType", "uint8"}, {"name", ""}, {"type", "uint8"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json symbol = {
      {"inputs", json::array()},
      {"name", "symbol"},
      {"outputs", {
        { {"internalType", "string"}, {"name", ""}, {"type", "string"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json allowance = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "owner"}, {"type", "address"} },
        { {"internalType", "address"}, {"name", "spender"}, {"type", "address"} }
      }},
      {"name", "allowance"},
      {"outputs", {
        { {"internalType", "uint256"}, {"name", ""}, {"type", "uint256"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json name = {
      {"inputs", json::array()},
      {"name", "name"},
      {"outputs", {
        { {"internalType", "string"}, {"name", ""}, {"type", "string"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };
  }

  namespace ERC20Wrapper {
    json deposit = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "token"}, {"type", "address"} },
        { {"internalType", "uint256"}, {"name", "value"}, {"type", "uint256"} }
      }},
      {"name", "deposit"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json transferTo = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "token"}, {"type", "address"} },
        { {"internalType", "address"}, {"name", "to"}, {"type", "address"} },
        { {"internalType", "uint256"}, {"name", "value"}, {"type", "uint256"} }
      }},
      {"name", "transferTo"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json withdraw = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "token"}, {"type", "address"} },
        { {"internalType", "uint256"}, {"name", "value"}, {"type", "uint256"} }
      }},
      {"name", "withdraw"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json getUserBalance = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "token"}, {"type", "address"} },
        { {"internalType", "address"}, {"name", "user"}, {"type", "address"} }
      }},
      {"name", "getUserBalance"},
      {"outputs", {
        { {"internalType", "uint256"}, {"name", ""}, {"type", "uint256"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json getContractBalance = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "token"}, {"type", "address"} }
      }},
      {"name", "getContractBalance"},
      {"outputs", {
        { {"internalType", "uint256"}, {"name", ""}, {"type", "uint256"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };
  }

  namespace NativeWrapper {
    json withdraw = {
      {"inputs", {
        { {"internalType", "uint256"}, {"name", "value"}, {"type", "uint256"} }
      }},
      {"name", "withdraw"},
      {"outputs", json::array()},
      {"stateMutability", "payable"},
      {"type", "function"}
    };

    json deposit = {
      {"inputs", json::array()},
      {"name", "deposit"},
      {"outputs", json::array()},
      {"stateMutability", "payable"},
      {"type", "function"}
    };
  }

  namespace ContractManager {
    json createNewERC20Contract = {
      {"inputs", {
        { {"internalType", "string"}, {"name", "erc20name"}, {"type", "string"} },
        { {"internalType", "string"}, {"name", "erc20symbol"}, {"type", "string"} },
        { {"internalType", "uint8"}, {"name", "erc20decimals"}, {"type", "uint8"} },
        { {"internalType", "uint256"}, {"name", "mintValue"}, {"type", "uint256"} }
      }},
      {"name", "createNewERC20Contract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewERC20WrapperContract = {
      {"inputs", json::array()},
      {"name", "createNewERC20WrapperContract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewNativeWrapperContract = {
      {"inputs", {
        { {"internalType", "string"}, {"name", "erc20_name"}, {"type", "string"} },
        { {"internalType", "string"}, {"name", "erc20_symbol"}, {"type", "string"} },
        { {"internalType", "uint8"}, {"name", "erc20_decimals"}, {"type", "uint8"} }
      }},
      {"name", "createNewNativeWrapperContract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewSimpleContractContract = {
      {"inputs", {
        { {"internalType", "string"}, {"name", "name_"}, {"type", "string"} },
        { {"internalType", "uint256"}, {"name", "number_"}, {"type", "uint256"} },
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"name", "tuple_"}, {"type", "tuple"}
        }
      }},
      {"name", "createNewSimpleContractContract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewDEXV2PairContract = {
      {"inputs", json::array()},
      {"name", "createNewDEXV2PairContract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewDEXV2FactoryContract = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "_feeToSetter"}, {"type", "address"} }
      }},
      {"name", "createNewDEXV2FactoryContract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewDEXV2Router02Contract = {
      {"inputs", {
        { {"internalType", "address"}, {"name", "factory"}, {"type", "address"} },
        { {"internalType", "address"}, {"name", "wrappedNative"}, {"type", "address"} }
      }},
      {"name", "createNewDEXV2Router02Contract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewERC721Contract = {
      {"inputs", {
        { {"internalType", "string"}, {"name", "erc721name"}, {"type", "string"} },
        { {"internalType", "string"}, {"name", "erc721symbol"}, {"type", "string"} }
      }},
      {"name", "createNewERC721Contract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewThrowTestAContract = {
      {"inputs", json::array()},
      {"name", "createNewThrowTestAContract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewThrowTestBContract = {
      {"inputs", json::array()},
      {"name", "createNewThrowTestBContract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json createNewThrowTestCContract = {
      {"inputs", json::array()},
      {"name", "createNewThrowTestCContract"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json getDeployedContracts = {
      {"inputs", json::array()},
      {"name", "getDeployedContracts"},
      {"outputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "address"}, {"type", "address"} }
          } }, {"type", "tuple[]"}
        }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json getDeployedContractsForCreator = {
      {"inputs", {
            { {"internalType", "address"}, {"name", "creator"}, {"type", "address"} }
      }},
      {"name", "getDeployedContractsForCreator"},
      {"outputs", {
          {
            {"components", {
              { {"internalType", "string"}, {"type", "string"} },
              { {"internalType", "address"}, {"type", "address"} }
            } }, {"type", "tuple[]"}
          }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };
  }

  namespace SimpleContract {
    json getNamesAndNumbersInArrayOfArrays = {
      {"inputs", {
        { {"internalType", "uint256"}, {"name", "i"}, {"type", "uint256"} }
      }},
      {"name", "getNamesAndNumbersInArrayOfArrays"},
      {"outputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"type", "tuple[][]"}
        }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json getNamesAndNumbersInTuple = {
      {"inputs", {
        { {"internalType", "uint256"}, {"name", "i"}, {"type", "uint256"} }
      }},
      {"name", "getNamesAndNumbersInTuple"},
      {"outputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"type", "tuple[]"}
        }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json getNameAndNumber = {
      {"inputs", json::array()},
      {"name", "getNameAndNumber"},
      {"outputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"type", "tuple"}
        }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json getNumbers = {
      {"inputs", {
        { {"internalType", "uint256"}, {"name", "i"}, {"type", "uint256"} }
      }},
      {"name", "getNumbers"},
      {"outputs", {
        { {"internalType", "uint256[]"}, {"name", ""}, {"type", "uint256[]"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json getNumber = {
      {"inputs", json::array()},
      {"name", "getNumber"},
      {"outputs", {
        { {"internalType", "uint256"}, {"name", ""}, {"type", "uint256"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json getName = {
      {"inputs", json::array()},
      {"name", "getName"},
      {"outputs", {
        { {"internalType", "string"}, {"name", ""}, {"type", "string"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json getTuple = {
      {"inputs", json::array()},
      {"name", "getTuple"},
      {"outputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"type", "tuple"}
        }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json setNamesAndNumbersInArrayOfArrays = {
      {"inputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"name", "argNameAndNumber"}, {"type", "tuple[][]"}
        }
      }},
      {"name", "setNamesAndNumbersInArrayOfArrays"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json setNamesAndNumbersInTuple = {
      {"inputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"name", "argNameAndNumber"}, {"type", "tuple[]"}
        }
      }},
      {"name", "setNamesAndNumbersInTuple"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json getNames = {
      {"inputs", {
        { {"internalType", "uint256"}, {"name", "i"}, {"type", "uint256"} },
      }},
      {"name", "getNames"},
      {"outputs", {
        { {"internalType", "string[]"}, {"name", ""}, {"type", "string[]"} },
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json setNumbers = {
      {"inputs", {
        { {"internalType", "uint256[]"}, {"name", "argNumber"}, {"type", "uint256[]"} },
      }},
      {"name", "setNumbers"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json setNumber = {
      {"inputs", {
        { {"internalType", "uint256"}, {"name", "argNumber"}, {"type", "uint256"} },
      }},
      {"name", "setNumber"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json setNamesAndNumbers = {
      {"inputs", {
        { {"internalType", "string[]"}, {"name", "argName"}, {"type", "string[]"} },
        { {"internalType", "uint256[]"}, {"name", "argNumber"}, {"type", "uint256[]"} }
      }},
      {"name", "setNamesAndNumbers"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json setNames = {
      {"inputs", {
        { {"internalType", "string[]"}, {"name", "argName"}, {"type", "string[]"} }
      }},
      {"name", "setNames"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json getNamesAndNumbers = {
      {"inputs", {
        { {"internalType", "uint256"}, {"name", "i"}, {"type", "uint256"} }
      }},
      {"name", "getNamesAndNumbers"},
      {"outputs", {
        {
          {"components", {
            { {"internalType", "string[]"}, {"type", "string[]"} },
            { {"internalType", "uint256[]"}, {"type", "uint256[]"} }
          } }, {"type", "tuple"}
        }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json setName = {
      {"inputs", {
        { {"internalType", "string"}, {"name", "argName"}, {"type", "string"} }
      }},
      {"name", "setName"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json getNumberOverload = {
      {"inputs", {
        { {"internalType", "uint256"}, {"name", ""}, {"type", "uint256"} }
      }},
      {"name", "getNumber"},
      {"outputs", {
        { {"internalType", "uint256"}, {"name", ""}, {"type", "uint256"} }
      }},
      {"stateMutability", "view"},
      {"type", "function"}
    };

    json setTuple = {
      {"inputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"name", "argTuple"}, {"type", "tuple"}
        }
      }},
      {"name", "setTuple"},
      {"outputs", json::array()},
      {"stateMutability", "nonpayable"},
      {"type", "function"}
    };

    json nameAndNumberTupleChanged = {
      {"anonymous", false},
      {"inputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"indexed", true}, {"name", "nameAndNumber"}, {"type", "tuple"}
        }
      }},
      {"name", "nameAndNumberTupleChanged"},
      {"type", "event"}
    };

    json nameAndNumberChanged = {
      {"anonymous", false},
      {"inputs", {
        { {"indexed", true}, {"internalType", "string"}, {"name", "name"}, {"type", "string"} },
        { {"indexed", true}, {"internalType", "uint256"}, {"name", "number"}, {"type", "uint256"} }
      }},
      {"name", "nameAndNumberChanged"},
      {"type", "event"}
    };

    json numberChanged = {
      {"anonymous", false},
      {"inputs", {
        { {"indexed", false}, {"internalType", "uint256"}, {"name", "number"}, {"type", "uint256"} }
      }},
      {"name", "numberChanged"},
      {"type", "event"}
    };

    json nameChanged = {
      {"anonymous", false},
      {"inputs", {
        { {"indexed", true}, {"internalType", "string"}, {"name", "name"}, {"type", "string"} }
      }},
      {"name", "nameChanged"},
      {"type", "event"}
    };

    json tupleChanged = {
      {"anonymous", false},
      {"inputs", {
        {
          {"components", {
            { {"internalType", "string"}, {"type", "string"} },
            { {"internalType", "uint256"}, {"type", "uint256"} }
          } }, {"indexed", true}, {"name", "tuple"}, {"type", "tuple"}
        }
      }},
      {"name", "tupleChanged"},
      {"type", "event"}
    };
  }
}

