file(
        GLOB_RECURSE EagleVMStub_SRC
        "*.h"
        "*.hpp"
        "*.cpp"
)

add_library(EagleVMStub SHARED ${EagleVMStub_SRC})

target_include_directories(EagleVMStub PUBLIC headers)
target_compile_features(EagleVMStub PRIVATE cxx_std_23)

set_target_properties(EagleVMStub PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\\bin\\${CMAKE_BUILD_TYPE})