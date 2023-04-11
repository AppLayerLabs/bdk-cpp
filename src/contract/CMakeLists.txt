set(CONTRACT_HEADERS
  ${CMAKE_SOURCE_DIR}/src/contract/abi.h
  ${CMAKE_SOURCE_DIR}/src/contract/contract.h
  ${CMAKE_SOURCE_DIR}/src/contract/contractmanager.h
  ${CMAKE_SOURCE_DIR}/src/contract/variables/safeuint64_t.h
  ${CMAKE_SOURCE_DIR}/src/contract/variables/safeuint256_t.h
  ${CMAKE_SOURCE_DIR}/src/contract/variables/safeaddress.h
  ${CMAKE_SOURCE_DIR}/src/contract/variables/safeunorderedmap.h
  ${CMAKE_SOURCE_DIR}/src/contract/variables/safebool.h
  PARENT_SCOPE
)

set(CONTRACT_SOURCES
  ${CMAKE_SOURCE_DIR}/src/contract/abi.cpp
  PARENT_SCOPE
)

