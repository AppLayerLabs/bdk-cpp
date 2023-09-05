#include "templates/erc20.h"
#include "templates/erc20wrapper.h"
#include "templates/erc20snapshot.h"
#include "templates/nativewrapper.h"
#include "templates/simplecontract.h"
#include "templates/erc721.h"
#include "templates/erc721uristorage.h"
#include "templates/erc721enumerable.h"
#include "templates/erc721royalty.h"
#include "templates/accesscontrol.h"
#include "templates/accesscontrolwithoperators.h"
#include "templates/erc2981.h"
#include "templates/pulsar_nft.h"
#include "templates/metacoin.h"
#include "templates/dexv2/dexv2pair.h"
#include "templates/dexv2/dexv2factory.h"
#include "templates/dexv2/dexv2router02.h"
#include "templates/throwtestA.h"
#include "templates/throwtestB.h"
#include "templates/throwtestC.h"

using ContractTypes = std::tuple<
  ERC20, ERC20Wrapper, NativeWrapper, SimpleContract, DEXV2Pair, DEXV2Factory, DEXV2Router02, ERC721,
  ThrowTestA, ThrowTestB, ThrowTestC, AccessControl, ERC721URIStorage, ERC721Enumerable, AccessControlWithOperators, ERC2981, ERC721Royalty, PulsarNft, ERC20Snapshot, MetaCoin
>;

