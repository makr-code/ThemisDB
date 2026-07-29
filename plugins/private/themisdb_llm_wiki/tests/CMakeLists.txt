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
