
set(UTILS_HEADERS
  ${CMAKE_SOURCE_DIR}/new_src/utils/utils.h
  ${CMAKE_SOURCE_DIR}/new_src/utils/strings.h
  ${CMAKE_SOURCE_DIR}/new_src/utils/hex.h
  ${CMAKE_SOURCE_DIR}/new_src/utils/json.hpp
  PARENT_SCOPE  
)

set(UTILS_SOURCES
  ${CMAKE_SOURCE_DIR}/new_src/utils/utils.cpp
  ${CMAKE_SOURCE_DIR}/new_src/utils/hex.cpp
  PARENT_SCOPE
)