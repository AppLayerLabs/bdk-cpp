#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/contract/templates/erc20wrapper.h" // erc20.h
#include "../sdktestsuite.hpp"
#include "contract/templates/simplecontract.h"

namespace TEVMCALL {

  Bytes erc20bytecode = Hex::toBytes("0x608060405234801561000f575f5ffd5b50604051610a83380380610a8383398101604081905261002e91610204565b604051806040016040528060098152602001682a32b9ba2a37b5b2b760b91b815250604051806040016040528060038152602001621514d560ea1b815250816003908161007b91906102b3565b50600461008882826102b3565b50505061009b33826100a160201b60201c565b50610392565b6001600160a01b0382166100cf5760405163ec442f0560e01b81525f60048201526024015b60405180910390fd5b6100da5f83836100de565b5050565b6001600160a01b038316610108578060025f8282546100fd919061036d565b909155506101789050565b6001600160a01b0383165f908152602081905260409020548181101561015a5760405163391434e360e21b81526001600160a01b038516600482015260248101829052604481018390526064016100c6565b6001600160a01b0384165f9081526020819052604090209082900390555b6001600160a01b038216610194576002805482900390556101b2565b6001600160a01b0382165f9081526020819052604090208054820190555b816001600160a01b0316836001600160a01b03167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef836040516101f791815260200190565b60405180910390a3505050565b5f60208284031215610214575f5ffd5b5051919050565b634e487b7160e01b5f52604160045260245ffd5b600181811c9082168061024357607f821691505b60208210810361026157634e487b7160e01b5f52602260045260245ffd5b50919050565b601f8211156102ae57805f5260205f20601f840160051c8101602085101561028c5750805b601f840160051c820191505b818110156102ab575f8155600101610298565b50505b505050565b81516001600160401b038111156102cc576102cc61021b565b6102e0816102da845461022f565b84610267565b6020601f821160018114610312575f83156102fb5750848201515b5f19600385901b1c1916600184901b1784556102ab565b5f84815260208120601f198516915b828110156103415787850151825560209485019460019092019101610321565b508482101561035e57868401515f19600387901b60f8161c191681555b50505050600190811b01905550565b8082018082111561038c57634e487b7160e01b5f52601160045260245ffd5b92915050565b6106e48061039f5f395ff3fe608060405234801561000f575f5ffd5b5060043610610090575f3560e01c8063313ce56711610063578063313ce567146100fa57806370a082311461010957806395d89b4114610131578063a9059cbb14610139578063dd62ed3e1461014c575f5ffd5b806306fdde0314610094578063095ea7b3146100b257806318160ddd146100d557806323b872dd146100e7575b5f5ffd5b61009c610184565b6040516100a99190610554565b60405180910390f35b6100c56100c03660046105a4565b610214565b60405190151581526020016100a9565b6002545b6040519081526020016100a9565b6100c56100f53660046105cc565b61022d565b604051601281526020016100a9565b6100d9610117366004610606565b6001600160a01b03165f9081526020819052604090205490565b61009c610250565b6100c56101473660046105a4565b61025f565b6100d961015a366004610626565b6001600160a01b039182165f90815260016020908152604080832093909416825291909152205490565b60606003805461019390610657565b80601f01602080910402602001604051908101604052809291908181526020018280546101bf90610657565b801561020a5780601f106101e15761010080835404028352916020019161020a565b820191905f5260205f20905b8154815290600101906020018083116101ed57829003601f168201915b5050505050905090565b5f3361022181858561026c565b60019150505b92915050565b5f3361023a85828561027e565b6102458585856102ff565b506001949350505050565b60606004805461019390610657565b5f336102218185856102ff565b610279838383600161035c565b505050565b6001600160a01b038381165f908152600160209081526040808320938616835292905220545f198110156102f957818110156102eb57604051637dc7a0d960e11b81526001600160a01b038416600482015260248101829052604481018390526064015b60405180910390fd5b6102f984848484035f61035c565b50505050565b6001600160a01b03831661032857604051634b637e8f60e11b81525f60048201526024016102e2565b6001600160a01b0382166103515760405163ec442f0560e01b81525f60048201526024016102e2565b61027983838361042e565b6001600160a01b0384166103855760405163e602df0560e01b81525f60048201526024016102e2565b6001600160a01b0383166103ae57604051634a1406b160e11b81525f60048201526024016102e2565b6001600160a01b038085165f90815260016020908152604080832093871683529290522082905580156102f957826001600160a01b0316846001600160a01b03167f8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b9258460405161042091815260200190565b60405180910390a350505050565b6001600160a01b038316610458578060025f82825461044d919061068f565b909155506104c89050565b6001600160a01b0383165f90815260208190526040902054818110156104aa5760405163391434e360e21b81526001600160a01b038516600482015260248101829052604481018390526064016102e2565b6001600160a01b0384165f9081526020819052604090209082900390555b6001600160a01b0382166104e457600280548290039055610502565b6001600160a01b0382165f9081526020819052604090208054820190555b816001600160a01b0316836001600160a01b03167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef8360405161054791815260200190565b60405180910390a3505050565b602081525f82518060208401528060208501604085015e5f604082850101526040601f19601f83011684010191505092915050565b80356001600160a01b038116811461059f575f5ffd5b919050565b5f5f604083850312156105b5575f5ffd5b6105be83610589565b946020939093013593505050565b5f5f5f606084860312156105de575f5ffd5b6105e784610589565b92506105f560208501610589565b929592945050506040919091013590565b5f60208284031215610616575f5ffd5b61061f82610589565b9392505050565b5f5f60408385031215610637575f5ffd5b61064083610589565b915061064e60208401610589565b90509250929050565b600181811c9082168061066b57607f821691505b60208210810361068957634e487b7160e01b5f52602260045260245ffd5b50919050565b8082018082111561022757634e487b7160e01b5f52601160045260245ffdfea2646970667358221220bb38ab6ed96fe17d4ed0a99dfd8e243ce4bbdf7ea8720be32ba90fcd2ab15d6264736f6c634300081e003300000000000000000000000000000000000000000000021e19e0c9bab2400000");

  /*
   * // SPDX-  License-Identifier: MIT
   * pragma solidity ^0.8.0;
   *
   *
   * interface SimpleContract {
   *     function getNameNonView() view external returns (string memory);
   * }
   *
   * contract GetName {
   *     function getName(address contractAddress) view external returns (string memory) {
   *         return SimpleContract(contractAddress).getNameNonView();
   *     }
   * }
   */
   Bytes getNameBytecode = Hex::toBytes("0x6080604052348015600e575f5ffd5b5061021f8061001c5f395ff3fe608060405234801561000f575f5ffd5b5060043610610029575f3560e01c80635fd4b08a1461002d575b5f5ffd5b61004061003b3660046100c0565b610056565b60405161004d91906100ed565b60405180910390f35b6060816001600160a01b031663eaeadcd16040518163ffffffff1660e01b81526004015f60405180830381865afa158015610093573d5f5f3e3d5ffd5b505050506040513d5f823e601f3d908101601f191682016040526100ba9190810190610136565b92915050565b5f602082840312156100d0575f5ffd5b81356001600160a01b03811681146100e6575f5ffd5b9392505050565b602081525f82518060208401528060208501604085015e5f604082850101526040601f19601f83011684010191505092915050565b634e487b7160e01b5f52604160045260245ffd5b5f60208284031215610146575f5ffd5b815167ffffffffffffffff81111561015c575f5ffd5b8201601f8101841361016c575f5ffd5b805167ffffffffffffffff81111561018657610186610122565b604051601f8201601f19908116603f0116810167ffffffffffffffff811182821017156101b5576101b5610122565b6040528181528282016020018610156101cc575f5ffd5b8160208401602083015e5f9181016020019190915294935050505056fea264697066735822122035c4d56cc58bb9efd4ce3f6d0e99ea8eddfcdff59cf5588a1ba090abef0e525f64736f6c634300081e0033");

   class SolGetName {
     public:
       std::string getName([[maybe_unused]] const Address& contractAddress) const { return ""; };
       void static registerContract() {
          ContractReflectionInterface::registerContractMethods<SolGetName>(
            std::vector<std::string>{""},
            std::make_tuple("getName", &SolGetName::getName, FunctionTypes::View, std::vector<std::string>{"address"})
          );
       }
   };

  /*  DELEGATECALL Example
   * pragma solidity ^0.8.0;
   *
   * contract ERC20BalanceDelegateCaller {
   *     mapping(address account => uint256) private balances;
   *     event BalanceOfResult(uint256);
   *     function addBalance(address user, uint256 value) external {
   *         balances[user] += value;
   *     }
   *     function callBalanceOf(address erc20, address user) external returns (uint256) {
   *         bytes memory data = abi.encodeWithSignature("balanceOf(address)", user);
   *         uint256 resultBalance;
   *         assembly {
   *             // Allocate memory for the output (32 bytes)
   *             let success := delegatecall(
   *                 gas(),         // Forward all available gas
   *                 erc20,         // Address of the ERC20 contract
   *                 add(data, 32), // Input data pointer (skip length prefix)
   *                 mload(data),   // Input data size
   *                 0,             // Output location (we"ll use memory slot 0)
   *                 32             // Output size (uint256 = 32 bytes)
   *             )
   *             if eq(success, 0) {
   *                 revert(0, 0)
   *             }
   *             // Load the result from memory slot 0
   *             resultBalance := mload(0)
   *         }
   *         emit BalanceOfResult(resultBalance);
   *         return resultBalance;
   *     }
   * }
   */

  Bytes delegateCallerBytecode = Hex::toBytes("0x6080604052348015600e575f5ffd5b506102058061001c5f395ff3fe608060405234801561000f575f5ffd5b5060043610610034575f3560e01c806321e5383a14610038578063d9101f3a1461004d575b5f5ffd5b61004b610046366004610157565b610072565b005b61006061005b36600461017f565b6100a2565b60405190815260200160405180910390f35b6001600160a01b0382165f90815260208190526040812080548392906100999084906101b0565b90915550505050565b6040516001600160a01b03821660248201525f90819060440160408051601f19818403018152919052602080820180516001600160e01b03166370a0823160e01b17815282519293505f928391885af4806100fb575f5ffd5b50505f516040518181527f2f4f4b223562753e25bac297aa55820e3ea4cce9ccaaefc01a919bac735ed60a9060200160405180910390a19150505b92915050565b80356001600160a01b0381168114610152575f5ffd5b919050565b5f5f60408385031215610168575f5ffd5b6101718361013c565b946020939093013593505050565b5f5f60408385031215610190575f5ffd5b6101998361013c565b91506101a76020840161013c565b90509250929050565b8082018082111561013657634e487b7160e01b5f52601160045260245ffdfea26469706673582212200d5a47de9dbab38d735ccb3022351477cd7c7aa9be79d1311ba1523d77ff66b164736f6c634300081e0033");

  class SolERC20BalanceDelegateCaller {
    public:
      void BalanceOfResult(const EventParam<uint256_t, false>& balance) {};
      void addBalance([[maybe_unused]] const Address& user, [[maybe_unused]] const uint256_t& value) { };
      uint256_t callBalanceOf([[maybe_unused]] const Address& erc20, [[maybe_unused]] const Address& user) { return 0; };
      void static registerContract() {
        ContractReflectionInterface::registerContractMethods<SolERC20BalanceDelegateCaller>(
          std::vector<std::string>{""},
          std::make_tuple("addBalance", &SolERC20BalanceDelegateCaller::addBalance, FunctionTypes::NonPayable, std::vector<std::string>{"address", "uint256"}),
          std::make_tuple("callBalanceOf", &SolERC20BalanceDelegateCaller::callBalanceOf, FunctionTypes::NonPayable, std::vector<std::string>{"address", "address"})
        );
        ContractReflectionInterface::registerContractEvents<SolERC20BalanceDelegateCaller>(
          std::make_tuple("BalanceOfResult", false, &SolERC20BalanceDelegateCaller::BalanceOfResult, std::vector<std::string>{"balance"})
        );
      }
  };

  /*
   * UniversalProxy
   * // SPDX-License-Identifier: MIT
   * pragma solidity 0.8.30;
   * import "@openzeppelin/contracts/access/Ownable.sol";
   *   contract Proxy is Ownable {
   *     address private _implementation;
   *     constructor() Ownable(msg.sender) {}
   *     function setContractCodeAddress(address newImplementation) external onlyOwner {
   *       require(newImplementation != address(0), "Invalid address");
   *       _implementation = newImplementation;
   *     }
   *     function getImplementation() external view returns (address) {
   *       return _implementation;
   *     }
   *     fallback() external payable {
   *       address impl = _implementation;
   *       require(impl != address(0), "Implementation not set");
   *       assembly {
   *         let ptr := mload(0x40)
   *         calldatacopy(ptr, 0, calldatasize())
   *         let result := delegatecall(gas(), impl, ptr, calldatasize(), 0, 0)
   *         let size := returndatasize()
   *         returndatacopy(ptr, 0, size)
   *
   *         switch result
   *         case 0 { revert(ptr, size) }
   *         default { return(ptr, size) }
   *       }
   *     }
   *     receive() external payable {}
   *   }
   */

  Bytes universalProxyBytecode = Hex::toBytes("0x6080604052348015600e575f5ffd5b503380603357604051631e4fbdf760e01b81525f600482015260240160405180910390fd5b603a81603f565b50608e565b5f80546001600160a01b038381166001600160a01b0319831681178455604051919092169283917f8be0079c531659141344cd1fd0a4f28419497f9722a3daafe3b4186f6b6457e09190a35050565b6103128061009b5f395ff3fe60806040526004361061004d575f3560e01c8063715018a6146100cf5780638da5cb5b146100e3578063aaf10f4214610117578063d17ca9b314610134578063f2fde38b1461015357610054565b3661005457005b6001546001600160a01b0316806100ab5760405162461bcd60e51b8152602060048201526016602482015275125b5c1b195b595b9d185d1a5bdb881b9bdd081cd95d60521b60448201526064015b60405180910390fd5b604051365f82375f5f3683855af43d805f843e8180156100c9578184f35b8184fd5b005b3480156100da575f5ffd5b506100cd610172565b3480156100ee575f5ffd5b505f546001600160a01b03165b6040516001600160a01b03909116815260200160405180910390f35b348015610122575f5ffd5b506001546001600160a01b03166100fb565b34801561013f575f5ffd5b506100cd61014e3660046102af565b610185565b34801561015e575f5ffd5b506100cd61016d3660046102af565b6101f7565b61017a610234565b6101835f610260565b565b61018d610234565b6001600160a01b0381166101d55760405162461bcd60e51b815260206004820152600f60248201526e496e76616c6964206164647265737360881b60448201526064016100a2565b600180546001600160a01b0319166001600160a01b0392909216919091179055565b6101ff610234565b6001600160a01b03811661022857604051631e4fbdf760e01b81525f60048201526024016100a2565b61023181610260565b50565b5f546001600160a01b031633146101835760405163118cdaa760e01b81523360048201526024016100a2565b5f80546001600160a01b038381166001600160a01b0319831681178455604051919092169283917f8be0079c531659141344cd1fd0a4f28419497f9722a3daafe3b4186f6b6457e09190a35050565b5f602082840312156102bf575f5ffd5b81356001600160a01b03811681146102d5575f5ffd5b939250505056fea2646970667358221220cd1a96e234003edd3881ad6ef938a3aa4a12acf73c2a4365e6fcfa5c6cdbaee364736f6c634300081e0033");
  class SolUniversalProxy {
    public:
      void setContractCodeAddress([[maybe_unused]] const Address& newImplementation) { };
      Address getImplementation() const { return Address(); };
      void static registerContract() {
        ContractReflectionInterface::registerContractMethods<SolUniversalProxy>(
          std::vector<std::string>{""},
          std::make_tuple("setContractCodeAddress", &SolUniversalProxy::setContractCodeAddress, FunctionTypes::NonPayable, std::vector<std::string>{"address"}),
          std::make_tuple("getImplementation", &SolUniversalProxy::getImplementation, FunctionTypes::View, std::vector<std::string>{})
        );
      }
  };
  /*
   * pragma solidity 0.8.30;
   * import "@openzeppelin/contracts/token/ERC20/IERC20.sol";
   * contract ERC20Wrapper {
   *   mapping(address erc20 => mapping(address user => uint256 balance)) userBalances_;
   *   function getContractBalance(address token) view external returns (uint256) {
   *     return IERC20(token).balanceOf(address(this));
   *   }
   *   function getUserBalance(address token, address user) view external returns (uint256) {
   *     return userBalances_[token][user];
   *   }
   *   function withdraw(address token, uint256 value) external {
   *     require(userBalances_[token][msg.sender] >= value, "Not enough token deposited to withdraw");
   *     userBalances_[token][msg.sender] -= value;
   *     IERC20(token).transfer(msg.sender, value);
   *   }
   *   function transferTo(address token, address to, uint256 value) external {
   *     require(userBalances_[token][msg.sender] >= value, "Not enough token deposited to withdraw");
   *     userBalances_[token][msg.sender] -= value;
   *     IERC20(token).transfer(to, value);
   *   }
   *   function deposit(address token, uint256 value) external {
   *     IERC20(token).transferFrom(msg.sender, address(this), value);
   *     userBalances_[token][msg.sender] += value;
   *   }
   * }
   */

  Bytes erc20WrapperBytecode = Hex::toBytes("0x6080604052348015600e575f5ffd5b506105a18061001c5f395ff3fe608060405234801561000f575f5ffd5b5060043610610055575f3560e01c806343ab265f1461005957806347e7ef241461007e5780636805d6ad14610093578063a5f2a152146100c9578063f3fef3a3146100dc575b5f5ffd5b61006c610067366004610402565b6100ef565b60405190815260200160405180910390f35b61009161008c366004610422565b61015d565b005b61006c6100a136600461044a565b6001600160a01b039182165f9081526020818152604080832093909416825291909152205490565b6100916100d736600461047b565b61020b565b6100916100ea366004610422565b6102ff565b6040516370a0823160e01b81523060048201525f906001600160a01b038316906370a0823190602401602060405180830381865afa158015610133573d5f5f3e3d5ffd5b505050506040513d601f19601f8201168201806040525081019061015791906104b5565b92915050565b6040516323b872dd60e01b8152336004820152306024820152604481018290526001600160a01b038316906323b872dd906064016020604051808303815f875af11580156101ad573d5f5f3e3d5ffd5b505050506040513d601f19601f820116820180604052508101906101d191906104cc565b506001600160a01b0382165f90815260208181526040808320338452909152812080548392906102029084906104ff565b90915550505050565b6001600160a01b0383165f908152602081815260408083203384529091529020548111156102545760405162461bcd60e51b815260040161024b90610512565b60405180910390fd5b6001600160a01b0383165f9081526020818152604080832033845290915281208054839290610284908490610558565b909155505060405163a9059cbb60e01b81526001600160a01b0383811660048301526024820183905284169063a9059cbb906044016020604051808303815f875af11580156102d5573d5f5f3e3d5ffd5b505050506040513d601f19601f820116820180604052508101906102f991906104cc565b50505050565b6001600160a01b0382165f9081526020818152604080832033845290915290205481111561033f5760405162461bcd60e51b815260040161024b90610512565b6001600160a01b0382165f908152602081815260408083203384529091528120805483929061036f908490610558565b909155505060405163a9059cbb60e01b8152336004820152602481018290526001600160a01b0383169063a9059cbb906044016020604051808303815f875af11580156103be573d5f5f3e3d5ffd5b505050506040513d601f19601f820116820180604052508101906103e291906104cc565b505050565b80356001600160a01b03811681146103fd575f5ffd5b919050565b5f60208284031215610412575f5ffd5b61041b826103e7565b9392505050565b5f5f60408385031215610433575f5ffd5b61043c836103e7565b946020939093013593505050565b5f5f6040838503121561045b575f5ffd5b610464836103e7565b9150610472602084016103e7565b90509250929050565b5f5f5f6060848603121561048d575f5ffd5b610496846103e7565b92506104a4602085016103e7565b929592945050506040919091013590565b5f602082840312156104c5575f5ffd5b5051919050565b5f602082840312156104dc575f5ffd5b8151801515811461041b575f5ffd5b634e487b7160e01b5f52601160045260245ffd5b80820180821115610157576101576104eb565b60208082526026908201527f4e6f7420656e6f75676820746f6b656e206465706f736974656420746f20776960408201526574686472617760d01b606082015260800190565b81810381811115610157576101576104eb56fea2646970667358221220dac5d8d4481220d03f907c72eac996ce935224bf1e75cf3a38bcf7825da8313964736f6c634300081e0033");
  class SolERC20Wrapper {
    public:
      uint256_t getContractBalance([[maybe_unused]] const Address& token) const { return 0; };
      uint256_t getUserBalance([[maybe_unused]] const Address& token, [[maybe_unused]] const Address& user) const { return 0; };
      void withdraw([[maybe_unused]] const Address& token, [[maybe_unused]] const uint256_t& value) { };
      void transferTo([[maybe_unused]] const Address& token, [[maybe_unused]] const Address& to, [[maybe_unused]] const uint256_t& value) { };
      void deposit([[maybe_unused]] const Address& token, [[maybe_unused]] const uint256_t& value) { };
      void static registerContract() {
        ContractReflectionInterface::registerContractMethods<SolERC20Wrapper>(
          std::vector<std::string>{""},
          std::make_tuple("getContractBalance", &SolERC20Wrapper::getContractBalance, FunctionTypes::View, std::vector<std::string>{"address"}),
          std::make_tuple("getUserBalance", &SolERC20Wrapper::getUserBalance, FunctionTypes::View, std::vector<std::string>{"address", "address"}),
          std::make_tuple("withdraw", &SolERC20Wrapper::withdraw, FunctionTypes::NonPayable, std::vector<std::string>{"address", "uint256"}),
          std::make_tuple("transferTo", &SolERC20Wrapper::transferTo, FunctionTypes::NonPayable, std::vector<std::string>{"address", "address", "uint256"}),
          std::make_tuple("deposit", &SolERC20Wrapper::deposit, FunctionTypes::NonPayable, std::vector<std::string>{"address", "uint256"})
        );
      }
  };

  TEST_CASE("EVM <-> C++ Call Tests", "[evm][evmcall]") {
    SDKTestSuite sdk = SDKTestSuite::createNewEnvironment("testEVMCalls");

    SECTION("EVM View/Static -> C++ Non-View/Static") {
      // The EVM contract will try to call a C++ contract while the EVM function is a view only and cannot change the state
      // The C++ function is a non-const/non-view function, so it can change the state.
      // Our execution engine should automatically REVERT the transaction
      Address simpleContractAddress = sdk.deployContract<SimpleContract>(std::string("TestName"), uint256_t(1000), std::make_tuple(
        std::string("TestName"),
        uint256_t(1000)
      ));

      // Now we deploy the EVM contract
      auto evmContractAddress = sdk.deployBytecode(getNameBytecode);

      // Now, we try to call the EVM contract, it should THROW because the initial call is a STATICCALL and
      // it can only call other contracts through STATICCALL
      REQUIRE_THROWS(sdk.callViewFunction(evmContractAddress, &SolGetName::getName, simpleContractAddress));
    }
    SECTION("EVM DelegateCall -> C++") {
      // Basically, ALL DELEGATECALL towards C++ contracts SHOULD BE REJECTED
      // DELEGATECALLs are not supported in C++ contracts, so we should throw an error
      Address evmERC20contractAddress = sdk.deployBytecode(erc20bytecode);
      Address delegateCallerAddress = sdk.deployBytecode(delegateCallerBytecode);

      // Do a transaction calling callBalanceOf
      auto tx = sdk.callFunction(delegateCallerAddress, &SolERC20BalanceDelegateCaller::callBalanceOf, evmERC20contractAddress, sdk.getChainOwnerAccount().address);
      auto callEvents = sdk.getEventsEmittedByTx(tx, &SolERC20BalanceDelegateCaller::BalanceOfResult);
      REQUIRE(callEvents.size() == 1);
      REQUIRE(std::get<0>(ABI::Decoder::decodeData<uint256_t>(callEvents[0].getData())) == uint256_t(0));
      // Now, if we increase the balance of the user, we should see the new balance
      tx = sdk.callFunction(delegateCallerAddress, &SolERC20BalanceDelegateCaller::addBalance, sdk.getChainOwnerAccount().address, uint256_t(100));
      tx = sdk.callFunction(delegateCallerAddress, &SolERC20BalanceDelegateCaller::callBalanceOf, evmERC20contractAddress, sdk.getChainOwnerAccount().address);
      callEvents = sdk.getEventsEmittedByTx(tx, &SolERC20BalanceDelegateCaller::BalanceOfResult);
      REQUIRE(callEvents.size() == 1);
      REQUIRE(std::get<0>(ABI::Decoder::decodeData<uint256_t>(callEvents[0].getData())) == uint256_t(100));

      Address cppERC20contractAddress = sdk.deployContract<ERC20>(std::string("Name"), std::string("Symbol"), uint8_t(18), uint256_t(1000));
      REQUIRE(sdk.callViewFunction(cppERC20contractAddress, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t(1000));

      // Trying DELEGATECALL from EVM to C++ should throw an error
      REQUIRE_THROWS(sdk.callFunction(delegateCallerAddress, &SolERC20BalanceDelegateCaller::callBalanceOf, cppERC20contractAddress, sdk.getChainOwnerAccount().address));
    }
    SECTION("EVM DelegateCall -> EVM --(call)--> C++") {
      // Basically, delegate call should be able to call any other EVM contract
      // and that called EVM contract can directly call a C++ contract
      // what we are going to do here is have the following contract structure:
      // Universal Solidity Proxy -> SolERC20Wrapper -> C++ ERC20
      // The Universal Solidity Proxy will call the SolERC20Wrapper, which will call the C++ ERC20
      Address cppERC20contractAddress = sdk.deployContract<ERC20>(std::string("Name"), std::string("Symbol"), uint8_t(18), uint256_t("100000000000000000000"));
      Address evmERC20contractAddress = sdk.deployBytecode(erc20bytecode);
      Address erc20WrapperAddress = sdk.deployBytecode(erc20WrapperBytecode);
      Address universalProxyAddress = sdk.deployBytecode(universalProxyBytecode);
      // Now we need to set the proxy address to point towards the ERC20Wrapper
      sdk.callFunction(universalProxyAddress, &SolUniversalProxy::setContractCodeAddress, erc20WrapperAddress);

      // In order to be able to deposit, we need to approve the *proxy* contract to spend our tokens on the C++ ERC20 contract
      sdk.callFunction(cppERC20contractAddress, &ERC20::approve, universalProxyAddress, uint256_t("50000000000000000000"));
      sdk.callFunction(evmERC20contractAddress, &ERC20::approve, universalProxyAddress, uint256_t("50000000000000000000"));

      REQUIRE(sdk.callViewFunction(cppERC20contractAddress, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("100000000000000000000"));
      REQUIRE(sdk.callViewFunction(cppERC20contractAddress, &ERC20::balanceOf, universalProxyAddress) == uint256_t("0"));
      REQUIRE(sdk.callViewFunction(evmERC20contractAddress, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("10000000000000000000000"));
      REQUIRE(sdk.callViewFunction(evmERC20contractAddress, &ERC20::balanceOf, universalProxyAddress) == uint256_t("0"));
      REQUIRE(sdk.callViewFunction(cppERC20contractAddress, &ERC20::allowance, sdk.getChainOwnerAccount().address, universalProxyAddress) == uint256_t("50000000000000000000"));
      REQUIRE(sdk.callViewFunction(evmERC20contractAddress, &ERC20::allowance, sdk.getChainOwnerAccount().address, universalProxyAddress) == uint256_t("50000000000000000000"));

      // Technically, we just call the proxy contract using the functions from the ERC20Wrapper and it should work
      // We deposit 50 tokens to the proxy contract

      auto depositTx = sdk.callFunction(universalProxyAddress, &SolERC20Wrapper::deposit, cppERC20contractAddress, uint256_t("50000000000000000000"));

      // After depositing we should see the balance of the proxy contract increase
      REQUIRE(sdk.callViewFunction(cppERC20contractAddress, &ERC20::balanceOf, universalProxyAddress) == uint256_t("50000000000000000000"));
      // The balance of the user should decrease
      REQUIRE(sdk.callViewFunction(cppERC20contractAddress, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("50000000000000000000"));
      // The balance of the user in the wrapper should be 50
      REQUIRE(sdk.callViewFunction(universalProxyAddress, &SolERC20Wrapper::getUserBalance, cppERC20contractAddress, sdk.getChainOwnerAccount().address) == uint256_t("50000000000000000000"));
      // The contract balance in the proxy should be 50
      REQUIRE(sdk.callViewFunction(universalProxyAddress, &SolERC20Wrapper::getContractBalance, cppERC20contractAddress) == uint256_t("50000000000000000000"));
      // We should see a single ERC20::Transfer event with from being the user and to being the proxy contract
      auto events = sdk.getEventsEmittedByTx(depositTx, &ERC20::Transfer);
      REQUIRE(events.size() == 1);
      REQUIRE(events[0].getAddress() == cppERC20contractAddress);
      REQUIRE(std::get<0>(ABI::Decoder::decodeData<Address>(events[0].getTopics()[1].asBytes())) == sdk.getChainOwnerAccount().address);
      REQUIRE(std::get<0>(ABI::Decoder::decodeData<Address>(events[0].getTopics()[2].asBytes())) == universalProxyAddress);
      REQUIRE(std::get<0>(ABI::Decoder::decodeData<uint256_t>(events[0].getData())) == uint256_t("50000000000000000000"));

      // Safety checks on the ERC20Wrapper contract
      REQUIRE(sdk.callViewFunction(cppERC20contractAddress, &ERC20::balanceOf, erc20WrapperAddress) == uint256_t("0"));
      REQUIRE(sdk.callViewFunction(erc20WrapperAddress, &SolERC20Wrapper::getContractBalance, cppERC20contractAddress) == uint256_t("0"));
      REQUIRE(sdk.callViewFunction(erc20WrapperAddress, &SolERC20Wrapper::getUserBalance, cppERC20contractAddress, sdk.getChainOwnerAccount().address) == uint256_t("0"));

      // Now deposit the same amount of tokens to the EVM contract

      depositTx = sdk.callFunction(universalProxyAddress, &SolERC20Wrapper::deposit, evmERC20contractAddress, uint256_t("50000000000000000000"));

      // After depositing we should see the balance of the proxy contract increase
      REQUIRE(sdk.callViewFunction(evmERC20contractAddress, &ERC20::balanceOf, universalProxyAddress) == uint256_t("50000000000000000000"));
      // The balance of the user should decrease
      REQUIRE(sdk.callViewFunction(evmERC20contractAddress, &ERC20::balanceOf, sdk.getChainOwnerAccount().address) == uint256_t("9950000000000000000000"));
      // The balance of the user in the wrapper should be 50
      REQUIRE(sdk.callViewFunction(universalProxyAddress, &SolERC20Wrapper::getUserBalance, evmERC20contractAddress, sdk.getChainOwnerAccount().address) == uint256_t("50000000000000000000"));
      // The contract balance in the proxy should be 50
      REQUIRE(sdk.callViewFunction(universalProxyAddress, &SolERC20Wrapper::getContractBalance, evmERC20contractAddress) == uint256_t("50000000000000000000"));

      // We should see a single ERC20::Transfer event with from being the user and to being the proxy contract
      events = sdk.getEventsEmittedByTx(depositTx, &ERC20::Transfer);
      REQUIRE(events.size() == 1);
      REQUIRE(events[0].getAddress() == evmERC20contractAddress);
      REQUIRE(std::get<0>(ABI::Decoder::decodeData<Address>(events[0].getTopics()[1].asBytes())) == sdk.getChainOwnerAccount().address);
      REQUIRE(std::get<0>(ABI::Decoder::decodeData<Address>(events[0].getTopics()[2].asBytes())) == universalProxyAddress);
      REQUIRE(std::get<0>(ABI::Decoder::decodeData<uint256_t>(events[0].getData())) == uint256_t("50000000000000000000"));
      // Safety checks on the ERC20Wrapper contract
      REQUIRE(sdk.callViewFunction(evmERC20contractAddress, &ERC20::balanceOf, erc20WrapperAddress) == uint256_t("0"));
      REQUIRE(sdk.callViewFunction(erc20WrapperAddress, &SolERC20Wrapper::getContractBalance, evmERC20contractAddress) == uint256_t("0"));
      REQUIRE(sdk.callViewFunction(erc20WrapperAddress, &SolERC20Wrapper::getUserBalance, evmERC20contractAddress, sdk.getChainOwnerAccount().address) == uint256_t("0"));
    }
  }
}
