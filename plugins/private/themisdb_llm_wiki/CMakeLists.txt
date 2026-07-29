cmake_minimum_required(VERSION 3.20)
project(themisdb_llm_wiki VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── ThemisDB SDK dependency ────────────────────────────────────────────────────
if(DEFINED THEMISDB_SDK_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${THEMISDB_SDK_DIR}")
endif()
find_package(themisdb_sdk QUIET)

# ── Wikipedia ingestion C++ sources ───────────────────────────────────────────
file(GLOB WIKI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/wikipedia/*.cpp"
)

if(WIKI_SOURCES)
    add_library(themisdb_llm_wiki_cpp STATIC ${WIKI_SOURCES})

    target_include_directories(themisdb_llm_wiki_cpp
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )

    if(themisdb_sdk_FOUND)
        target_link_libraries(themisdb_llm_wiki_cpp PUBLIC themisdb_sdk::themisdb_sdk)
    endif()

    set_target_properties(themisdb_llm_wiki_cpp PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
option(THEMISDB_LLM_WIKI_BUILD_TESTS "Build llm_wiki tests" OFF)
if(THEMISDB_LLM_WIKI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)

if(WIKI_SOURCES)
    install(TARGETS themisdb_llm_wiki_cpp
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/themisdb
    )
    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/themisdb_llm_wiki
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()
