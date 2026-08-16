# CMake helper utilities for ThemisDB
# ===========================================================================
#
# This module provides reusable CMake macros and functions for:
#   1. Consistent C++ standard configuration
#   2. Consistent edition option registration (via themis_register_edition_option)
#   3. Uniform test target creation (via themis_add_test)
#
# Philosophy:
#   - Reduce boilerplate and consolidate repeated patterns
#   - Maintain readability and semantic clarity
#   - Aid contributor onboarding by providing standard idioms
#
# ===========================================================================

# ============================================================================
# C++ Standard Configuration
# ============================================================================
# 
# Ensures all targets use C++20 by default unless explicitly overridden.
# Can be customized via -DCMAKE_CXX_STANDARD=<value> at configure time.
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard to use for ThemisDB targets" FORCE)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Edition Option Registration Macro
# ============================================================================
# 
# Consolidates repeated edition option declaration pattern.
# Reduces duplication across cmake/editions/*.cmake files.
#
# Usage:
#   themis_register_edition_option(
#       OPTION_NAME <option-var-name>
#       DESCRIPTION <help-text>
#       DEFAULT <ON|OFF>
#       [ADVANCED]
#   )
#
# Example:
#   themis_register_edition_option(
#       OPTION_NAME THEMIS_ENABLE_HTTP3
#       DESCRIPTION "Enable HTTP/3 (QUIC) support"
#       DEFAULT ON
#       ADVANCED
#   )
macro(themis_register_edition_option)
    # Parse keyword arguments
    set(_opt_options ADVANCED)
    set(_opt_single OPTION_NAME DESCRIPTION DEFAULT)
    set(_opt_multi)
    cmake_parse_arguments(_ero "${_opt_options}" "${_opt_single}" "${_opt_multi}" ${ARGN})
    
    if(NOT _ero_OPTION_NAME)
        message(FATAL_ERROR "themis_register_edition_option: OPTION_NAME is required")
    endif()
    if(NOT _ero_DESCRIPTION)
        message(FATAL_ERROR "themis_register_edition_option: DESCRIPTION is required")
    endif()
    if(NOT DEFINED _ero_DEFAULT)
        message(FATAL_ERROR "themis_register_edition_option: DEFAULT (ON/OFF) is required")
    endif()
    
    # Create option
    option(${_ero_OPTION_NAME} "${_ero_DESCRIPTION}" "${_ero_DEFAULT}")
    
    # Mark as advanced if requested
    if(_ero_ADVANCED)
        mark_as_advanced(${_ero_OPTION_NAME})
    endif()
endmacro()

# ============================================================================
# Test Helper
# ============================================================================
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

# ============================================================================
# Strict Compilation Options Helper
# ============================================================================
# 
# Apply -Werror and strict compilation flags to specific targets only,
# not to external dependencies. This prevents strict mode from failing on
# warnings in vendored code (e.g. llama.cpp, ggml).
#
# Usage:
#   themis_apply_strict_build_flags(target_name)
#
# This function adds -Werror to the target's compile options if
# THEMIS_STRICT_BUILD is enabled.
#
function(themis_apply_strict_build_flags target_name)
    if(DEFINED THEMIS_STRICT_COMPILE_OPTIONS AND THEMIS_STRICT_COMPILE_OPTIONS)
        target_compile_options(${target_name} PRIVATE ${THEMIS_STRICT_COMPILE_OPTIONS})
    endif()
endfunction()
