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
  ${CMAKE_SOURCE_DIR}/tests/contract/abi.cpp
  ${CMAKE_SOURCE_DIR}/tests/core/rdpos.cpp
  ${CMAKE_SOURCE_DIR}/tests/core/storage.cpp
  ${CMAKE_SOURCE_DIR}/tests/net/p2p/p2p.cpp
  PARENT_SCOPE
)
