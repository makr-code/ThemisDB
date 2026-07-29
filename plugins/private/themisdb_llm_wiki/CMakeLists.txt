cmake_minimum_required(VERSION 3.20)
project(themisdb_llm_wiki VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Monorepo detection ─────────────────────────────────────────────────────────
# Detect whether we are being built as part of the ThemisDB monorepo by checking
# for the canonical wiki_chunk_splitter.h three levels up.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../include/llm/wiki_chunk_splitter.h")
    set(THEMISDB_MONOREPO TRUE)
    set(THEMISDB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
    message(STATUS "[llm_wiki] Monorepo build detected (THEMISDB_ROOT_DIR=${THEMISDB_ROOT_DIR})")
else()
    set(THEMISDB_MONOREPO FALSE)
    message(STATUS "[llm_wiki] Standalone/SDK build mode")
endif()

# ── External dependencies ──────────────────────────────────────────────────────
if(DEFINED THEMISDB_SDK_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${THEMISDB_SDK_DIR}")
endif()

find_package(themisdb_sdk QUIET)
find_package(nlohmann_json QUIET)
find_package(spdlog        QUIET)

# ── Plugin implementation sources (Phase 2) ────────────────────────────────────
set(LLM_WIKI_IMPL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wikipedia/wiki_workspace_orchestrator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wikipedia/llm_wiki_plugin_impl.cpp"
)

# In monorepo mode include the ThemisDB LLM core sources directly so the static
# library is fully self-contained and can be used by tests without a separate
# ThemisDB install.
if(THEMISDB_MONOREPO)
    list(APPEND LLM_WIKI_IMPL_SOURCES
        "${THEMISDB_ROOT_DIR}/src/llm/wiki_chunk_splitter.cpp"
        "${THEMISDB_ROOT_DIR}/src/llm/wiki_index_store.cpp"
        "${THEMISDB_ROOT_DIR}/src/llm/wiki_rag_source.cpp"
    )
endif()

# ── themisdb_llm_wiki_cpp — static library (test + plugin linkage) ─────────────
add_library(themisdb_llm_wiki_cpp STATIC ${LLM_WIKI_IMPL_SOURCES})

target_include_directories(themisdb_llm_wiki_cpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(THEMISDB_MONOREPO)
    target_include_directories(themisdb_llm_wiki_cpp
        PUBLIC
            $<BUILD_INTERFACE:${THEMISDB_ROOT_DIR}/include>
    )
endif()

if(themisdb_sdk_FOUND)
    target_link_libraries(themisdb_llm_wiki_cpp PUBLIC themisdb_sdk::themisdb_sdk)
endif()
if(nlohmann_json_FOUND)
    target_link_libraries(themisdb_llm_wiki_cpp PUBLIC nlohmann_json::nlohmann_json)
endif()
if(spdlog_FOUND)
    target_link_libraries(themisdb_llm_wiki_cpp PUBLIC spdlog::spdlog)
endif()

set_target_properties(themisdb_llm_wiki_cpp PROPERTIES
    CXX_STANDARD          20
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# ── themisdb_llm_wiki_plugin — shared library (runtime plugin .so / .dll) ──────
add_library(themisdb_llm_wiki_plugin SHARED
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wikipedia/llm_wiki_plugin_factory.cpp"
)

# THEMIS_PLUGIN_EXPORTS enables __declspec(dllexport) on Windows
target_compile_definitions(themisdb_llm_wiki_plugin PRIVATE THEMIS_PLUGIN_EXPORTS)

target_include_directories(themisdb_llm_wiki_plugin
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(THEMISDB_MONOREPO)
    target_include_directories(themisdb_llm_wiki_plugin
        PRIVATE
            $<BUILD_INTERFACE:${THEMISDB_ROOT_DIR}/include>
    )
endif()

# The shared library links the static impl library so the factory is the only
# translation unit compiled directly into the .so.
target_link_libraries(themisdb_llm_wiki_plugin
    PRIVATE themisdb_llm_wiki_cpp
)

set_target_properties(themisdb_llm_wiki_plugin PROPERTIES
    CXX_STANDARD          20
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME           "themisdb_llm_wiki"
    PREFIX                ""               # avoid "lib" prefix on Linux for clarity
)

# ── Tests ──────────────────────────────────────────────────────────────────────
option(THEMISDB_LLM_WIKI_BUILD_TESTS "Build llm_wiki tests" OFF)
if(THEMISDB_LLM_WIKI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ────────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS themisdb_llm_wiki_cpp themisdb_llm_wiki_plugin
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/themisdb_llm_wiki
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
