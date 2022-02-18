file(
        GLOB_RECURSE EagleVMSandbox_SRC
        "*.h"
        "*.hpp"
        "*.cpp"
)

add_executable(EagleVMSandbox ${EagleVMSandbox_SRC})

target_link_libraries(EagleVMSandbox PRIVATE EagleVMStub)
target_compile_features(EagleVMSandbox PRIVATE cxx_std_23)

set_target_properties(EagleVMSandbox PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\\bin\\${CMAKE_BUILD_TYPE})