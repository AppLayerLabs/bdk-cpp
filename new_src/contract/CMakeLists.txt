set(CONTRACT_HEADERS
  ${CMAKE_SOURCE_DIR}/new_src/contract/abi.h 
  ${CMAKE_SOURCE_DIR}/new_src/contract/contract.h 
  ${CMAKE_SOURCE_DIR}/new_src/contract/contractmanager.h
  PARENT_SCOPE
)

set(CONTRACT_SOURCES
  ${CMAKE_SOURCE_DIR}/new_src/contract/abi.cpp
  PARENT_SCOPE
)