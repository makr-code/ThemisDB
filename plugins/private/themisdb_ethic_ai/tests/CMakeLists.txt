cmake_minimum_required(VERSION 3.20)
project(ethics_ai_tests)

find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found; ethics_ai tests skipped.")
    return()
endif()

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")

foreach(_src IN LISTS TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "ethics_ai_${_stem}")
    add_executable(${_target} ${_src})
    target_link_libraries(${_target} PRIVATE ethics_ai_plugin GTest::gtest_main)
    add_test(NAME ${_target} COMMAND ${_target})
    set_tests_properties(${_target} PROPERTIES TIMEOUT 120 LABELS "ethics_ai")
endforeach()
