#include "templates/erc20.h"
#include "templates/erc20wrapper.h"
#include "templates/nativewrapper.h"
#include "templates/simplecontract.h"
#include "templates/erc721.h"
#include "templates/dexv2/dexv2pair.h"
#include "templates/dexv2/dexv2factory.h"
#include "templates/dexv2/dexv2router02.h"

using ContractTypes = std::tuple<
  ERC20, ERC20Wrapper, NativeWrapper, SimpleContract, DEXV2Pair, DEXV2Factory, DEXV2Router02, ERC721
>;

