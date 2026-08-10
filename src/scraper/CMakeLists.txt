cmake_minimum_required(VERSION 3.20)

# ============================================================================
# ThemisDB Scraper Plugin
# Agentic AI module for gap-detection-driven web scraping.
# Supports: static HTML, JS-rendered pages (React/SPA), JSON REST APIs,
#           HTML search-form discovery, result-list parsing (openjur.de etc.),
#           and a built-in catalog of German federal, state, and EU sources.
# ============================================================================

project(scraper_plugin VERSION 1.0.0 LANGUAGES CXX)

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

# ── Sources ─────────────────────────────────────────────────────────────────
set(SCRAPER_SOURCES
    scraper_config.cpp
    scraper_search_engine.cpp
    gov_source_catalog.cpp
    scraper_js_renderer.cpp
    scraper_api_client.cpp
    scraper_llm_evaluator.cpp
    scraper_metadata_writer.cpp
    scraper_robots.cpp
    scraper_sitemap.cpp
    scraper_plugin.cpp
)

add_library(scraper_plugin STATIC ${SCRAPER_SOURCES})

target_include_directories(scraper_plugin
    PUBLIC
        ${THEMIS_ROOT_DIR}/include
)

target_compile_features(scraper_plugin PUBLIC cxx_std_17)

# ── Optional: yaml-cpp ──────────────────────────────────────────────────────
find_package(yaml-cpp QUIET)
if(yaml-cpp_FOUND)
    target_link_libraries(scraper_plugin PUBLIC yaml-cpp)
    target_compile_definitions(scraper_plugin PUBLIC THEMIS_ENABLE_YAML=1)
    message(STATUS "  scraper_plugin: yaml-cpp found – YAML config enabled")
else()
    message(STATUS "  scraper_plugin: yaml-cpp not found – YAML config disabled")
endif()

# ── Optional: pugixml ───────────────────────────────────────────────────────
find_package(pugixml QUIET)
if(pugixml_FOUND)
    target_link_libraries(scraper_plugin PUBLIC pugixml::pugixml)
    target_compile_definitions(scraper_plugin PUBLIC THEMIS_ENABLE_PUGIXML=1)
    message(STATUS "  scraper_plugin: pugixml found – HTML search engine enabled")
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PUGIXML QUIET pugixml)
    endif()
    if(PUGIXML_FOUND)
        target_include_directories(scraper_plugin PUBLIC ${PUGIXML_INCLUDE_DIRS})
        target_link_libraries(scraper_plugin PUBLIC ${PUGIXML_LIBRARIES})
        target_compile_definitions(scraper_plugin PUBLIC THEMIS_ENABLE_PUGIXML=1)
        message(STATUS "  scraper_plugin: pugixml found via pkg-config")
    else()
        message(STATUS "  scraper_plugin: pugixml not found – fallback HTML parsing")
    endif()
endif()

# ── Optional: libcurl ───────────────────────────────────────────────────────
find_package(CURL QUIET)
if(CURL_FOUND)
    target_link_libraries(scraper_plugin PUBLIC CURL::libcurl)
    target_compile_definitions(scraper_plugin PUBLIC THEMIS_ENABLE_CURL=1)
    message(STATUS "  scraper_plugin: libcurl found – HTTP client enabled")
else()
    message(STATUS "  scraper_plugin: libcurl not found – HTTP disabled (test-only mode)")
endif()

# ── Optional: nlohmann_json ─────────────────────────────────────────────────
find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(scraper_plugin PUBLIC nlohmann_json::nlohmann_json)
else()
    if(EXISTS "${THEMIS_ROOT_DIR}/third_party/nlohmann/json.hpp")
        target_include_directories(scraper_plugin PUBLIC
            "${THEMIS_ROOT_DIR}/third_party/nlohmann")
    endif()
endif()

# ── Optional: LLM integration ───────────────────────────────────────────────
if(THEMIS_ENABLE_LLM)
    target_compile_definitions(scraper_plugin PUBLIC THEMIS_ENABLE_LLM=1)
    message(STATUS "  scraper_plugin: THEMIS_ENABLE_LLM=ON – LLM evaluator enabled")
endif()

# ── Plugin manifest ─────────────────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../plugins/scraper/plugin.json.in")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/../../plugins/scraper/plugin.json.in"
        "${CMAKE_CURRENT_BINARY_DIR}/plugin.json"
        @ONLY
    )
endif()

message(STATUS "scraper_plugin v${PROJECT_VERSION} configured")
