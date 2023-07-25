#include "erc20.h"
#include "erc20wrapper.h"
#include "nativewrapper.h"
#include "dexv2pair.h"
#include "dexv2factory.h"
#include "dexv2router02.h"

using ContractTypes = std::tuple<ERC20, ERC20Wrapper, NativeWrapper, DEXV2Pair, DEXV2Factory, DEXV2Router02>;