set(TESTS_HEADERS
  ""
  PARENT_SCOPE
)

set (TESTS_SOURCES
  ""
  ${CMAKE_SOURCE_DIR}/new_tests/db.cpp
  ${CMAKE_SOURCE_DIR}/new_tests/utils.cpp
  ${CMAKE_SOURCE_DIR}/new_tests/hex.cpp
  ${CMAKE_SOURCE_DIR}/new_tests/strings.cpp
  ${CMAKE_SOURCE_DIR}/new_tests/merkle.cpp
  ${CMAKE_SOURCE_DIR}/new_tests/ecdsa.cpp
  ${CMAKE_SOURCE_DIR}/new_tests/randomgen.cpp
  ${CMAKE_SOURCE_DIR}/new_tests/p2p.cpp
  ${CMAKE_SOURCE_DIR}/new_tests/tx.cpp
  PARENT_SCOPE
)
