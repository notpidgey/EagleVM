file(GLOB_RECURSE EagleVM_SRC
    "*.h"
    "*.hpp"
    "*.cpp"
)

add_executable(EagleVM ${EagleVM_SRC})

target_include_directories(EagleVM PRIVATE Headers)
target_link_libraries(EagleVM PRIVATE Zydis)
target_compile_features(EagleVM PRIVATE cxx_std_23)

set_target_properties(EagleVM PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:EagleVM>"
)

target_compile_definitions(EagleVM PRIVATE NOMINMAX)

if(MSVC)
    target_compile_options(EagleVM PRIVATE
        # Shared flags
        /permissive-
        /sdl
        /W3
        /Zc:inline
        /Zc:wchar_t
        # Configuration-specific flags
        "$<$<CONFIG:Debug>:/FC;/JMC>"
        "$<$<NOT:$<CONFIG:Debug>>:/Gy;/O1;/Os>"
    )
endif()

set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT EagleVM)