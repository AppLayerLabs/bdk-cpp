#include "erc20.h"
#include "erc20wrapper.h"
#include "nativewrapper.h"
#include "dexv2/dexv2pair.h"
#include "dexv2/dexv2factory.h"
#include "dexv2/dexv2router02.h"

using ContractTypes = std::tuple<ERC20, ERC20Wrapper, NativeWrapper, DEXV2Pair, DEXV2Factory, DEXV2Router02>;