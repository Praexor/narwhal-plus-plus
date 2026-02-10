cmake_minimum_required(VERSION 3.20)
project(narwhal_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuration options
option(BUILD_TESTS "Build tests" ON)
option(USE_MOCKS "Use internal mocks for RocksDB, Sodium, and SSL" ON)

# Include directories
include_directories(include)

# Find Packages or Use Mocks
if(NOT USE_MOCKS)
    find_package(Threads REQUIRED)
    find_package(RocksDB REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(Sodium REQUIRED)
    find_package(Boost REQUIRED COMPONENTS system)
else()
    message(STATUS "Build Mode: INTERNAL MOCK (No external dependencies required)")
    add_compile_definitions(USE_INTERNAL_MOCKS)
    find_package(Threads REQUIRED)
endif()

# Source files
set(CRYPTO_SOURCES src/crypto.cpp)
set(STORE_SOURCES src/store.cpp)
set(NETWORK_SOURCES src/network.cpp)
set(CONSENSUS_SOURCES src/consensus.cpp)

# Libraries (STATIC to avoid DLL export issues on Windows)
add_library(narwhal_crypto STATIC ${CRYPTO_SOURCES})
add_library(narwhal_store STATIC ${STORE_SOURCES})
add_library(narwhal_network STATIC ${NETWORK_SOURCES})
add_library(narwhal_consensus STATIC ${CONSENSUS_SOURCES})

if(NOT USE_MOCKS)
    target_link_libraries(narwhal_crypto PUBLIC Sodium::Sodium)
    target_link_libraries(narwhal_store PUBLIC RocksDB::rocksdb)
    target_link_libraries(narwhal_network PUBLIC OpenSSL::SSL Boost::boost)
endif()

# Common links
target_link_libraries(narwhal_consensus PUBLIC narwhal_crypto narwhal_store narwhal_network Threads::Threads)

# Executables
add_executable(primary_node src/primary.cpp)
target_link_libraries(primary_node PRIVATE narwhal_consensus)

add_executable(worker_node src/worker.cpp)
target_link_libraries(worker_node PRIVATE narwhal_consensus)

if(BUILD_TESTS)
    enable_testing()
endif()
