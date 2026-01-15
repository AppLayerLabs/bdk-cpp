# Find the cometbft-bdk executable and copy it to the build folder.
# Also register it as an additional cleanup file for `cmake --build . --target clean`.

# HACK: the right thing to do here would be to change CometImpl::doStartCometBFT()
# in core/comet.cpp, so it adds the deps bin folder to its search paths and
# executes it from there (to avoid breaching containment).
# However, that forces us to either hardcode the path, or add a variable
# in comet.h (as comet.h.in, so CMake would have to generate it every time).
# Given the executable itself is very lightweight (~25 MB), it's easier to
# just copy it over and call it a day.

find_program(COMETBFT_EXEC cometbft-bdk)

file(COPY "${COMETBFT_EXEC}" DESTINATION "${CMAKE_SOURCE_DIR}/build")

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES "cometbft-bdk")

