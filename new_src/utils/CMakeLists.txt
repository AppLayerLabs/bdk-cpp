
set(UTILS_HEADERS
  ${CMAKE_SOURCE_DIR}/new_src/utils/utils.h
  ${CMAKE_SOURCE_DIR}/new_src/utils/strings.h
  ${CMAKE_SOURCE_DIR}/new_src/utils/hex.h
  ${CMAKE_SOURCE_DIR}/new_src/utils/json.hpp
  ${CMAKE_SOURCE_DIR}/new_src/utils/merkle.h
  PARENT_SCOPE
)

set(UTILS_SOURCES
  ${CMAKE_SOURCE_DIR}/new_src/utils/utils.cpp
  ${CMAKE_SOURCE_DIR}/new_src/utils/hex.cpp
  ${CMAKE_SOURCE_DIR}/new_src/utils/merkle.cpp
  PARENT_SCOPE
)