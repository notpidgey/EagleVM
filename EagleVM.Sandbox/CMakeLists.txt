file(GLOB_RECURSE EagleVMSandbox_SRC
    "*.h"
    "*.hpp"
    "*.cpp"
)

add_executable(EagleVMSandbox ${EagleVMSandbox_SRC})

target_link_libraries(EagleVMSandbox PRIVATE EagleVMStub)
target_compile_features(EagleVMSandbox PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(EagleVMSandbox PRIVATE /Od)
    target_link_options(EagleVMSandbox PRIVATE "/MAP:${PROJECT_BINARY_DIR}/EVMS.map")
endif()