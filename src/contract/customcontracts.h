/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "templates/standards/erc20.h"
#include "templates/erc20wrapper.h"
#include "templates/nativewrapper.h"
#include "templates/simplecontract.h"
#include "templates/standards/erc721.h"
#include "templates/standards/ierc721receiver.hpp"
#include "templates/dexv2/dexv2pair.h"
#include "templates/dexv2/dexv2factory.h"
#include "templates/dexv2/dexv2router02.h"
#include "templates/orderbook/orderbook.h"
#include "templates/throwtestA.h"
#include "templates/throwtestB.h"
#include "templates/throwtestC.h"
#include "templates/erc721test.h"
#include "templates/testThrowVars.h"
#include "templates/randomnesstest.h"
#include "templates/snailtracer.h"
#include "templates/snailtraceroptimized.h"
#include "templates/ownable.h"
#include "templates/pebble.h"
#include "templates/btvenergy.h"
#include "templates/btvplayer.h"
#include "templates/btvproposals.h"
#include "templates/buildthevoid.h"
#include "templates/mintableerc20.h"

using InterfaceTypes = std::tuple<IERC721Receiver>;

/// Typedef for the blockchain's registered contracts.
#ifdef BUILD_TESTNET
/// Typedef for the blockchain's registered contracts in TESTNET mode.
using ContractTypes = std::tuple<
  ERC20, NativeWrapper, DEXV2Pair, DEXV2Factory, DEXV2Router02, ERC721, ERC721URIStorage, Ownable, Pebble, BTVPlayer, BTVEnergy, BTVProposals, BuildTheVoid, ERC20Mintable
>;
#else
/// Typedef for the blockchain's registered contracts in normal mode.
using ContractTypes = std::tuple<
  ERC20, ERC20Wrapper, NativeWrapper, SimpleContract, DEXV2Pair, DEXV2Factory,
  DEXV2Router02, ERC721, ThrowTestA, ThrowTestB, ThrowTestC, ERC721Test, TestThrowVars,
  RandomnessTest, SnailTracer, SnailTracerOptimized, Pebble, BTVPlayer, BTVEnergy, BTVProposals, BuildTheVoid, ERC20Mintable, OrderBook, Ownable
>;
#endif

