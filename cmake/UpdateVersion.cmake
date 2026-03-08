# This script runs at BUILD time to regenerate version.h with the current git hash.

execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

if(NOT GIT_COMMIT_HASH)
    set(GIT_COMMIT_HASH "nogit")
endif()

set(APP_VERSION_FULL "${PROJECT_VERSION}-${GIT_COMMIT_HASH}")

configure_file(
    ${SOURCE_DIR}/cmake/version.h.in
    ${BINARY_DIR}/generated/version.h
)
