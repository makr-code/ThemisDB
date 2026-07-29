cmake_minimum_required(VERSION 3.20)
project(themisdb_llm_wiki_tests)

# ── Python tests ──────────────────────────────────────────────────────────────
find_program(PYTHON_EXECUTABLE NAMES python3 python)
if(PYTHON_EXECUTABLE)
    add_test(
        NAME llm_wiki_mvp_python_tests
        COMMAND ${PYTHON_EXECUTABLE} -m pytest
            ${CMAKE_CURRENT_SOURCE_DIR}/test_llm_wiki_mvp.py -v
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
    set_tests_properties(llm_wiki_mvp_python_tests PROPERTIES
        TIMEOUT 120
        LABELS  "llm_wiki;python"
    )
endif()

# ── C++ legacy test ───────────────────────────────────────────────────────────
find_package(GTest QUIET)
if(GTest_FOUND AND TARGET themisdb_llm_wiki_cpp)
    add_executable(test_wikipedia_ingestion_legacy
        legacy/test_wikipedia_ingestion_plugin.cpp
    )
    target_link_libraries(test_wikipedia_ingestion_legacy
        PRIVATE themisdb_llm_wiki_cpp GTest::gtest_main)
    add_test(NAME test_wikipedia_ingestion_legacy
        COMMAND test_wikipedia_ingestion_legacy)
    set_tests_properties(test_wikipedia_ingestion_legacy PROPERTIES
        TIMEOUT 120
        LABELS  "llm_wiki;wikipedia")
endif()

# ── LWP focused C++ unit tests (Phase 2 — LWP-01..LWP-08) ────────────────────
if(GTest_FOUND AND TARGET themisdb_llm_wiki_cpp)
    # Resolve the monorepo root (three levels above tests/)
    get_filename_component(_plugin_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    get_filename_component(_monorepo_root "${_plugin_root}/../../.." ABSOLUTE)

    add_executable(module_lwp_test_lwp_plugin_focused_focused
        "${CMAKE_CURRENT_SOURCE_DIR}/test_lwp_plugin_focused.cpp"
    )

    target_include_directories(module_lwp_test_lwp_plugin_focused_focused
        PRIVATE
            # Private plugin headers (wiki_workspace_orchestrator, llm_wiki_plugin_impl)
            "${_plugin_root}/include"
            # ThemisDB public headers (llm_wiki_plugin_interface, wiki_chunk_splitter, …)
            "${_monorepo_root}/include"
    )

    target_link_libraries(module_lwp_test_lwp_plugin_focused_focused
        PRIVATE
            themisdb_llm_wiki_cpp
            GTest::gtest_main
    )

    set_target_properties(module_lwp_test_lwp_plugin_focused_focused PROPERTIES
        CXX_STANDARD          20
        CXX_STANDARD_REQUIRED ON
    )

    add_test(
        NAME    test_lwp_plugin_focused
        COMMAND module_lwp_test_lwp_plugin_focused_focused
            --gtest_output=xml:${CMAKE_BINARY_DIR}/module_lwp_test_lwp_plugin_focused_focused_results.xml
    )
    set_tests_properties(test_lwp_plugin_focused PROPERTIES
        TIMEOUT 120
        LABELS  "llm_wiki;unit;lwp"
    )
endif()
