# Ethics AI Plugin – src/ethics_ai
# Sources and private headers live together here.
# Public interface headers remain in include/plugins/ethics_ai/ (pre-existing).

if(NOT THEMIS_PLUGIN_ETHICS_AI)
    return()
endif()

message(STATUS "Configuring Ethics AI Plugin (src/ethics_ai)")

set(ETHICS_AI_SOURCES
    ethics_ai_types.cpp
    ethics_ai_plugin.cpp
    philosophy_loader.cpp
    argument_store.cpp
    rag_context_engine.cpp
    discourse_engine.cpp
    chain_visualizer.cpp
    ethics_profile_registry.cpp
    ethics_selection_router.cpp
    discourse_orchestrator.cpp
    meta_verdict_builder.cpp
    mirror_school_handler.cpp
)

add_library(ethics_ai_plugin SHARED ${ETHICS_AI_SOURCES})

target_include_directories(ethics_ai_plugin
    PUBLIC
        $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    target_link_libraries(ethics_ai_plugin PRIVATE yaml-cpp)
    target_compile_definitions(ethics_ai_plugin PRIVATE HAVE_YAML_CPP)
    message(STATUS "  - yaml-cpp found: Enabling YAML philosophy profiles")
else()
    message(WARNING "  - yaml-cpp not found: YAML loading will be limited")
endif()

target_link_libraries(ethics_ai_plugin PRIVATE themis_core)

set_target_properties(ethics_ai_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ethics_ai_plugin"
    VERSION 1.0.0
    SOVERSION 1
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

if(MSVC)
    target_compile_options(ethics_ai_plugin PRIVATE /W4)
    target_compile_definitions(ethics_ai_plugin PRIVATE THEMIS_PLUGIN_EXPORTS)
else()
    target_compile_options(ethics_ai_plugin PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS ethics_ai_plugin
    LIBRARY DESTINATION lib/themisdb/plugins
    RUNTIME DESTINATION lib/themisdb/plugins
    ARCHIVE DESTINATION lib/themisdb/plugins
)

install(DIRECTORY ${THEMIS_ROOT_DIR}/include/plugins/ethics_ai/
    DESTINATION include/plugins/ethics_ai
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

configure_file(
    ${THEMIS_ROOT_DIR}/plugins/ethics_ai/ethics_ai_plugin.json.in
    ${CMAKE_CURRENT_BINARY_DIR}/ethics_ai_plugin.json
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ethics_ai_plugin.json
    DESTINATION lib/themisdb/plugins/
)

install(DIRECTORY ${THEMIS_ROOT_DIR}/plugins/ethics_ai/philosophies/
    DESTINATION lib/themisdb/plugins/ethics_ai/
    FILES_MATCHING PATTERN "*.yaml" PATTERN "*.yml" PATTERN "*.md"
)

if(THEMIS_BUILD_EXAMPLES)
    if(EXISTS "${THEMIS_ROOT_DIR}/plugins/ethics_ai/examples/CMakeLists.txt")
        add_subdirectory(${THEMIS_ROOT_DIR}/plugins/ethics_ai/examples
                         ${CMAKE_CURRENT_BINARY_DIR}/ethics_ai_examples)
    endif()
endif()

message(STATUS "Ethics AI Plugin configuration complete")
