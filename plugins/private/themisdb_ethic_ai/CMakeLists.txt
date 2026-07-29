cmake_minimum_required(VERSION 3.20)
project(themisdb_ethic_ai VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── ThemisDB SDK dependency ────────────────────────────────────────────────────
# Expects THEMISDB_SDK_DIR to point to the installed ThemisDB public SDK.
# Set via -DTHEMISDB_SDK_DIR=<path> or vcpkg.
if(DEFINED THEMISDB_SDK_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${THEMISDB_SDK_DIR}")
endif()

find_package(themisdb_sdk QUIET)
if(NOT themisdb_sdk_FOUND)
    message(WARNING
        "themisdb_sdk not found. "
        "Set THEMISDB_SDK_DIR to the ThemisDB install prefix or use vcpkg overlay.")
endif()

# ── Optional dependencies ─────────────────────────────────────────────────────
find_package(yaml-cpp QUIET)

# ── Sources ───────────────────────────────────────────────────────────────────
set(ETHICS_AI_SOURCES
    src/ethics_ai_plugin.cpp
    src/ethics_ai_types.cpp
    src/ethics_evaluator.cpp
    src/ethics_profile_registry.cpp
    src/ethics_selection_router.cpp
    src/discourse_engine.cpp
    src/discourse_memory_store.cpp
    src/argument_store.cpp
    src/philosophy_loader.cpp
    src/rag_context_engine.cpp
    src/chain_visualizer.cpp
    src/convergence_marker_engine.cpp
    src/cross_school_tension_resolver.cpp
    src/tournament_mode_selector.cpp
    src/synthesis_matrix_builder.cpp
    src/position_abstract_validator.cpp
    src/prior_round_compressor.cpp
    src/llm_cascade_router.cpp
)

add_library(ethics_ai_plugin SHARED ${ETHICS_AI_SOURCES})

target_include_directories(ethics_ai_plugin
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(themisdb_sdk_FOUND)
    target_link_libraries(ethics_ai_plugin PUBLIC themisdb_sdk::themisdb_sdk)
endif()

if(yaml-cpp_FOUND)
    target_link_libraries(ethics_ai_plugin PRIVATE yaml-cpp)
    target_compile_definitions(ethics_ai_plugin PRIVATE HAVE_YAML_CPP)
endif()

set_target_properties(ethics_ai_plugin PROPERTIES
    OUTPUT_NAME   "ethics_ai_plugin"
    PREFIX        ""
    VERSION       ${PROJECT_VERSION}
    SOVERSION     1
    POSITION_INDEPENDENT_CODE ON
)

# ── Tests ─────────────────────────────────────────────────────────────────────
option(THEMISDB_ETHIC_AI_BUILD_TESTS "Build ethics_ai tests" OFF)
if(THEMISDB_ETHIC_AI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS ethics_ai_plugin
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    RUNTIME  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ethics_ai
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(DIRECTORY philosophies/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/themisdb/plugins/ethics_ai/philosophies
    FILES_MATCHING PATTERN "*.yaml" PATTERN "*.yml"
)

configure_file(plugin.json.in plugin.json @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plugin.json
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb/plugins
)
