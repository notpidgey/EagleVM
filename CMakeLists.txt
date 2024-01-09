cmake_minimum_required(VERSION 3.17)

# Enable support for MSVC_RUNTIME_LIBRARY
cmake_policy(SET CMP0091 NEW)

# Set up a more familiar Visual Studio configuration
# Override these options with -DCMAKE_OPTION=Value
#
# See: https://cmake.org/cmake/help/latest/command/set.html#set-cache-entry
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG:FULL /INCREMENTAL:NO" CACHE STRING "")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/DEBUG:FULL /INCREMENTAL:NO" CACHE STRING "")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")

project(EagleVM
        VERSION 0.1
        DESCRIPTION "Native code virtualizer"
        LANGUAGES CXX)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)

add_subdirectory(deps/zydis)
add_subdirectory(EagleVM)
add_subdirectory(EagleVMSandbox)
add_subdirectory(EagleVMStub)