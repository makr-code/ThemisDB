# CMake helper utilities for ThemisDB
# - Sets a project-wide C++ standard if not configured by the caller
# - Provides a small `themis_add_test()` wrapper to apply default TIMEOUT and LABELS

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to use for ThemisDB targets" FORCE)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Simple test helper: adds a CTest test and applies uniform properties
# Usage: themis_add_test(<name> <cmd> [args...])
function(themis_add_test name)
    if("${ARGC}" EQUAL 0)
        message(FATAL_ERROR "themis_add_test requires at least a name and a command")
    endif()
    if(NOT ARGN)
        message(FATAL_ERROR "themis_add_test ${name}: missing command/arguments")
    endif()

    # Create the test with the provided command and args
    add_test(NAME ${name} COMMAND ${ARGN})

    # Apply sane defaults: 60s timeout, label 'unit' unless labels already set later
    set_tests_properties(${name} PROPERTIES LABELS "unit" TIMEOUT 60)
endfunction()
