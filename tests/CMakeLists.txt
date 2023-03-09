set(TESTS_HEADERS
  ""
  PARENT_SCOPE
)

set (TESTS_SOURCES
  ""
  ${CMAKE_SOURCE_DIR}/tests/db.cpp
  ${CMAKE_SOURCE_DIR}/tests/utils.cpp
  ${CMAKE_SOURCE_DIR}/tests/hex.cpp
  ${CMAKE_SOURCE_DIR}/tests/strings.cpp
  ${CMAKE_SOURCE_DIR}/tests/merkle.cpp
  ${CMAKE_SOURCE_DIR}/tests/ecdsa.cpp
  ${CMAKE_SOURCE_DIR}/tests/randomgen.cpp
  ${CMAKE_SOURCE_DIR}/tests/p2p.cpp
  ${CMAKE_SOURCE_DIR}/tests/tx.cpp
  ${CMAKE_SOURCE_DIR}/tests/block.cpp
  PARENT_SCOPE
)
