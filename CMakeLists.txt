cmake_minimum_required(VERSION 3.5)

project(
    tinyrpc
    VERSION 1.0
    LANGUAGES C CXX
)

############################################################################

 # Compiler flags.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra -std=c++14 -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb -fno-omit-frame-pointer -fno-optimize-sibling-calls")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fPIC")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fPIC")
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -fPIC")

# Output directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# include directory
include_directories(${PROJECT_SOURCE_DIR}/src/include/)

############################################################################

file(GLOB TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*.cpp")

file(GLOB
    REQUERD_FILES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/*.hpp
)


foreach (trpc_test_source ${TEST_SOURCES})
    # Create a human readable name.
    get_filename_component(trpc_test_filename ${trpc_test_source} NAME)
    string(REPLACE ".cpp" "" trpc_test_name ${trpc_test_filename})
    # Add the test target separately and as part of "make check-tests".
    add_executable(${trpc_test_name}  ${PROJECT_SOURCE_DIR}/test/${trpc_test_filename} ${REQUERD_FILES})
    # Set test target properties and dependencies.
    set_target_properties(${trpc_test_name}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        COMMAND ${trpc_test_name}
    )
endforeach(trpc_test_source ${TEST_SOURCES})
