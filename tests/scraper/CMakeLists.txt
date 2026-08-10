# Auto-generated module tests for prefix 'scraper'

file(GLOB SCRAPER_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_scraper_*.cpp"
)

foreach(_src IN LISTS SCRAPER_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_scraper_${_stem}_focused")
    set(_ctest "${_stem}_scraper_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    set(_extra_sources)
    if("${_stem}" STREQUAL "test_scraper_plugin")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_config.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_search_engine.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/gov_source_catalog.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_api_client.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_llm_evaluator.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_metadata_writer.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_js_renderer.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_plugin.cpp
        )
    endif()
    if("${_stem}" STREQUAL "test_scraper_v12_features_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_config.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_search_engine.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/gov_source_catalog.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_api_client.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_llm_evaluator.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_metadata_writer.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_js_renderer.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_robots.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_sitemap.cpp
            ${THEMIS_ROOT_DIR}/src/scraper/scraper_plugin.cpp
        )
    endif()
    if(_extra_sources)
        target_sources(${_target} PRIVATE ${_extra_sources})
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE scraper
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS scraper
    )
endforeach()