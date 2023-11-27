set(TESTS_HEADERS
  ""
  PARENT_SCOPE
)

set (TESTS_SOURCES
  ""
  ${CMAKE_SOURCE_DIR}/tests/utils/block.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/block_throw.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/db.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/ecdsa.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/hex.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/merkle.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/randomgen.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/strings.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/tx.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/tx_throw.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/utils.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils/options.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/abi.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/erc20.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/contractmanager.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/erc20wrapper.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/nativewrapper.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/contractabigenerator.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/dexv2.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/simplecontract.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safeaddress.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safearray.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safebool.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safeint_t_boost.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safeint_t_c++.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safemultiset.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safestring.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safeuint_t_boost.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safeuint_t_c++.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safeunorderedmap.cpp
  ${CMAKE_SOURCE_DIR}/tests/contract/variables/safevector.cpp
  ${CMAKE_SOURCE_DIR}/tests/core/rdpos.cpp
  ${CMAKE_SOURCE_DIR}/tests/core/storage.cpp
  ${CMAKE_SOURCE_DIR}/tests/core/state.cpp
  # ${CMAKE_SOURCE_DIR}/tests/core/blockchain.cpp # TODO: Blockchain is failing due to rdPoSWorker.
  ${CMAKE_SOURCE_DIR}/tests/net/p2p/p2p.cpp
  ${CMAKE_SOURCE_DIR}/tests/net/http/httpjsonrpc.cpp
  PARENT_SCOPE
)

