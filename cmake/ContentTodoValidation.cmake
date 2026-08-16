# ============================================================================
# Content Module Automated TODO Validation (CMT-7502)
# ============================================================================
# 
# This CMake snippet validates that all production TODOs in the content module:
# 1. Are properly classified (Optimization/Feature/Vendor/Other)
# 2. Have GitHub issue references (#XXXX)
# 3. Have documented rationale in CONTENT_DEFERRED_FEATURES.md
#
# Integration: Add to CMakeLists.txt in content module build section
#
# Usage:
#   cmake -DTHEMIS_ENABLE_CONTENT_TODO_VALIDATION=ON ..
#   ctest --preset <name> -L content-todo-validation
#
# ============================================================================

option(THEMIS_ENABLE_CONTENT_TODO_VALIDATION 
    "Enable automated validation of content module production TODOs" ON)

if(THEMIS_ENABLE_CONTENT_TODO_VALIDATION AND EXISTS "${CMAKE_SOURCE_DIR}/cmake/scripts/validate_content_todos.py")
    
    message(STATUS "Configuring Content Module TODO Validation (CMT-7502)")
    
    # Python executable for validation
    find_program(PYTHON3_EXECUTABLE python3 python)
    
    if(PYTHON3_EXECUTABLE)
        # Add custom target for TODO validation
        add_custom_target(content-validate-todos
            COMMAND ${PYTHON3_EXECUTABLE} 
                "${CMAKE_SOURCE_DIR}/cmake/scripts/validate_content_todos.py"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "Validating content module production TODOs (CMT-7502)"
        )
        
        # Add to pre-build checks if THEMIS_BUILD_TESTS is enabled
        if(THEMIS_BUILD_TESTS)
            add_custom_target(content-validate-todos-strict
                COMMAND ${PYTHON3_EXECUTABLE}
                    "${CMAKE_SOURCE_DIR}/cmake/scripts/validate_content_todos.py" --strict
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                COMMENT "Validating content module TODOs (strict mode)"
            )
            
            # Register as a test for CI/CD integration
            add_test(
                NAME content-todo-validation
                COMMAND ${PYTHON3_EXECUTABLE}
                    "${CMAKE_SOURCE_DIR}/cmake/scripts/validate_content_todos.py"
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            )
            
            set_tests_properties(content-todo-validation PROPERTIES
                LABELS "content-todo-validation;quality"
                TIMEOUT 30
            )
        endif()
        
        message(STATUS "  ✓ Content TODO validation targets added")
        message(STATUS "  Run with: cmake --build . --target content-validate-todos")
        
    else()
        message(WARNING "Python3 not found - content TODO validation disabled")
    endif()
    
endif()

# ============================================================================
# Integration with Build System
# ============================================================================
# 
# After adding the above snippet to CMakeLists.txt, the following commands
# become available:
#
# Build-time checks:
#   cmake --build . --target content-validate-todos
#   cmake --build . --target content-validate-todos-strict
#
# Test integration:
#   ctest -L content-todo-validation
#   ctest --preset community-release -L content
#
# CI/CD Pipeline:
#   The test can be integrated into .github/workflows/ci-build.yml with:
#
#   - name: Validate Content Module TODOs
#     run: |
#       cmake --build build --target content-validate-todos || true
#       ctest --build-config Release -L content-todo-validation || true
#
# ============================================================================
