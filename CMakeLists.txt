cmake_minimum_required(VERSION 3.17)

project(EagleVM
        VERSION 0.1
        DESCRIPTION "Native code virtualizer"
        LANGUAGES CXX)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Let's ensure -std=c++xx instead of -std=g++xx
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
else ()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif ()

option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)

add_subdirectory(deps/zydis)
add_subdirectory(EagleVM)
add_subdirectory(EagleVMSandbox)
add_subdirectory(EagleVMStub)