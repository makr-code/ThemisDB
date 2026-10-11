# cmake/CoverageConfiguration.cmake
# Coverage instrumentation setup for code execution metrics
#
# This module enables optional coverage flag configuration for GCC/Clang
# Usage: -DTHEMIS_ENABLE_COVERAGE=ON during cmake configure
# Compatibility: GCC 7+, Clang 3.5+, MSVC (not supported, skipped)

option(THEMIS_ENABLE_COVERAGE "Enable code coverage instrumentation (GCC/Clang only)" OFF)

if(THEMIS_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add --coverage flag for instrumentation
        # This enables .gcda file generation during test execution
        add_compile_options(--coverage)
        add_link_options(--coverage)
        
        # Additional flags for Clang to generate profdata
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            add_compile_options(-fprofile-instr-generate)
            add_compile_options(-fcoverage-mapping)
        endif()
        
        message(STATUS "Coverage instrumentation enabled")
        message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
        message(STATUS "  Flags: --coverage (GCC) or -fprofile-instr-generate (Clang)")
        message(STATUS "  Data files: .gcda (GCC) or .profraw (Clang)")
        message(STATUS "  Process with: lcov or llvm-cov")
        
        # Track that coverage is enabled for downstream logic
        set(THEMIS_COVERAGE_ENABLED ON CACHE BOOL "Coverage instrumentation is active" FORCE)
        
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        message(WARNING "Code coverage not natively supported for MSVC. Consider using OpenCppCoverage tool separately.")
        set(THEMIS_COVERAGE_ENABLED OFF CACHE BOOL "Coverage instrumentation is active" FORCE)
        
    else()
        message(WARNING "Code coverage may not be supported for ${CMAKE_CXX_COMPILER_ID}. Proceeding without coverage flags.")
        set(THEMIS_COVERAGE_ENABLED OFF CACHE BOOL "Coverage instrumentation is active" FORCE)
    endif()
else()
    set(THEMIS_COVERAGE_ENABLED OFF CACHE BOOL "Coverage instrumentation is active" FORCE)
endif()

# Provide a function to mark a test for coverage tracking
# Usage: themis_add_coverage_test(target_name label_list)
function(themis_add_coverage_test test_target test_labels)
    if(THEMIS_COVERAGE_ENABLED)
        # Ensure coverage data is collected when this test runs
        set_tests_properties(${test_target} PROPERTIES 
            ENVIRONMENT "LLVM_PROFILE_FILE=${CMAKE_CURRENT_BINARY_DIR}/${test_target}-%p.profraw"
        )
        message(DEBUG "Coverage tracking enabled for test: ${test_target}")
    endif()
endfunction()

# Provide a function to register coverage-tracked executables for nightly collection
# Usage: themis_register_coverage_target(executable_name)
function(themis_register_coverage_target coverage_target)
    if(THEMIS_COVERAGE_ENABLED)
        set_property(GLOBAL APPEND PROPERTY THEMIS_COVERAGE_TARGETS ${coverage_target})
    endif()
endfunction()

# Helper function to get all coverage targets
function(themis_get_coverage_targets out_var)
    get_property(targets GLOBAL PROPERTY THEMIS_COVERAGE_TARGETS)
    set(${out_var} "${targets}" PARENT_SCOPE)
endfunction()
