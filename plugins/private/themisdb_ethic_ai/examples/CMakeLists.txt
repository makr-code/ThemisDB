cmake_minimum_required(VERSION 3.20)

find_package(GTest QUIET)
if(NOT GTest_FOUND)
    return()
endif()

file(GLOB EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
foreach(_src IN LISTS EXAMPLE_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    add_executable(ethics_ai_example_${_stem} ${_src})
    target_link_libraries(ethics_ai_example_${_stem} PRIVATE ethics_ai_plugin)
endforeach()
