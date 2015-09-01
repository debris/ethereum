cmake_minimum_required(VERSION 3.0.0)

set(ETH_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${ETH_CMAKE_DIR})

# set cmake_policies
include(EthPolicy)
eth_policy()

# project name and version should be set after cmake_policy CMP0048
project(ethereum VERSION "0.9.91")

include(EthCompilerSettings)

include(EthExecutableHelper)

include(EthDependencies)

include(EthUtils)

# core libraries
add_subdirectory(libethash)
add_subdirectory(libethash-cl)
add_subdirectory(libethcore)
add_subdirectory(libevmcore)
add_subdirectory(libevmasm)
add_subdirectory(liblll)
add_subdirectory(libevm)
add_subdirectory(libethereum)
#add_subdirectory(evmjit)

# other libraries
add_subdirectory(libnatspec)
add_subdirectory(libtestutils)

# executables
add_subdirectory(ethkey)
add_subdirectory(ethminer)
add_subdirectory(ethvm)
add_subdirectory(lllc)

# test
#add_subdirectory(test)
