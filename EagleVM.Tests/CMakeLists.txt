file(GLOB_RECURSE EagleVMTests_SRC
        "*.h"
        "*.hpp"
        "*.cpp"
)

add_executable(EagleVMTests ${EagleVMTests_SRC})
target_compile_features(EagleVMTests PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(EagleVMTests PRIVATE /Od)
endif()