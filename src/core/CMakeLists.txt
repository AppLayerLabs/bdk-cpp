if(BUILD_AVALANCHEGO)
  set(CORE_HEADERS
     ${CMAKE_SOURCE_DIR}/src/core/blockchain.h
  #  ${CMAKE_SOURCE_DIR}/src/core/snowmanVM.h
     ${CMAKE_SOURCE_DIR}/src/core/state.h
     ${CMAKE_SOURCE_DIR}/src/core/storage.h
     ${CMAKE_SOURCE_DIR}/src/core/rdpos.h
    PARENT_SCOPE
  )

  set(CORE_SOURCES
     ${CMAKE_SOURCE_DIR}/src/core/blockchain.cpp
  #  ${CMAKE_SOURCE_DIR}/src/core/snowmanVM.cpp
     ${CMAKE_SOURCE_DIR}/src/core/state.cpp
     ${CMAKE_SOURCE_DIR}/src/core/storage.cpp
     ${CMAKE_SOURCE_DIR}/src/core/rdpos.cpp
    PARENT_SCOPE
  )
else()
  set(CORE_HEADERS
     ${CMAKE_SOURCE_DIR}/src/core/blockchain.h
     ${CMAKE_SOURCE_DIR}/src/core/state.h
     ${CMAKE_SOURCE_DIR}/src/core/storage.h
     ${CMAKE_SOURCE_DIR}/src/core/rdpos.h
    PARENT_SCOPE
  )

  set(CORE_SOURCES
     ${CMAKE_SOURCE_DIR}/src/core/blockchain.cpp
     ${CMAKE_SOURCE_DIR}/src/core/state.cpp
     ${CMAKE_SOURCE_DIR}/src/core/storage.cpp
     ${CMAKE_SOURCE_DIR}/src/core/rdpos.cpp
    PARENT_SCOPE
  )
endif()
