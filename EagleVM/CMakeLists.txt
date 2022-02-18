file(
        GLOB_RECURSE EagleVM_SRC
        "*.h"
        "*.hpp"
        "*.cpp"
)

add_executable(EagleVM ${EagleVM_SRC})
target_include_directories(EagleVM PRIVATE Headers)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(EagleVM PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE "FALSE" MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")

    target_compile_definitions(EagleVM PRIVATE "_DEBUG" "DEBUG")
    target_compile_options(EagleVM PRIVATE /JMC /permissive- /GS /W3 /Zc:wchar_t /Zc:inline /Zi /Gm- /Od /sdl /Gd /FC)

    target_link_options(EagleVM PRIVATE /SUBSYSTEM:CONSOLE)
else ()
    set_target_properties(EagleVM PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE" MSVC_RUNTIME_LIBRARY "MultiThreaded")

    target_compile_definitions(EagleVM PRIVATE "NDEBUG")
    target_compile_options(EagleVM PRIVATE /permissive- /GS /Gy /GL /W3 /Zc:wchar_t /Zc:inline /O1 /Os /Gm- /sdl /Gd)

    target_link_options(EagleVM PRIVATE /OPT:REF /OPT:ICF /LTCG)

endif ()
target_link_libraries(
        EagleVM
        PRIVATE "Zydis"
)

set_target_properties(EagleVM PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}\\bin\\${CMAKE_BUILD_TYPE})
target_compile_features(EagleVM PRIVATE cxx_std_23)