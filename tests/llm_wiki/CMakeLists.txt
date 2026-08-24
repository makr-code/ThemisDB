cmake_minimum_required(VERSION 3.20)

# LLM Wiki module focused tests

file(GLOB LLM_WIKI_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS LLM_WIKI_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_llm_wiki_${_stem}_focused")
    set(_ctest  "${_stem}_LlmWikiFocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    # Map each test to the llm_wiki source files it exercises
    if(_stem STREQUAL "test_llm_wiki_core_focused"
       OR _stem STREQUAL "test_llm_wiki_edge_cases_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/llm_wiki/workspace_state_manager.cpp
            ${THEMIS_ROOT_DIR}/src/llm_wiki/edition_gate.cpp
        )
    elseif(_stem STREQUAL "test_llm_wiki_llm_integration_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/llm_wiki/edition_gate.cpp
        )
    endif()

    add_executable(${_target} "${_src}" ${_extra_sources})
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )

    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        spdlog::spdlog
        Threads::Threads
    )

    target_compile_definitions(${_target} PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE llm_wiki
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS llm_wiki
    )
endforeach()
