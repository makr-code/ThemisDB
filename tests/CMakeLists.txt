# ThemisDB Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building tests
if(NOT THEMIS_BUILD_TESTS)
    message(STATUS "Tests disabled (THEMIS_BUILD_TESTS=OFF)")
    return()
endif()

message(STATUS "Configuring ThemisDB test suite")

# Find GTest
find_package(GTest QUIET CONFIG)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found - tests will not be built")
    message(WARNING "Install GTest or use vcpkg: vcpkg install gtest")
    return()
endif()

message(STATUS "GTest found - building tests")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${GTest_INCLUDE_DIRS}
)

# Common test libraries
set(TEST_LIBS
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
)

# ============================================================================
# LoRA Framework Tests (NEW)
# ============================================================================

# Check for LoRA framework test file
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework.cpp")
    message(STATUS "Adding LoRA framework tests")
    
    add_executable(test_lora_framework
        test_lora_framework.cpp
    )
    
    target_link_libraries(test_lora_framework PRIVATE
        ${TEST_LIBS}
        # Add ThemisDB core libraries when available
        # themis_core
        # themis_llm
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_lora_framework PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME LoRAFrameworkTests COMMAND test_lora_framework)
    
    # Set test properties
    set_tests_properties(LoRAFrameworkTests PROPERTIES
        LABELS "lora;llm;framework"
        TIMEOUT 300
    )
else()
    message(STATUS "LoRA framework tests not found at ${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework.cpp")
endif()

# ============================================================================
# LoRA Framework Comprehensive Tests (NEW - Issue #XXX)
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_lora_framework_comprehensive.cpp")
    message(STATUS "Adding LoRA framework comprehensive tests")
    
    add_executable(test_lora_framework_comprehensive
        test_lora_framework_comprehensive.cpp
    )
    
    target_link_libraries(test_lora_framework_comprehensive PRIVATE
        ${TEST_LIBS}
        # Add ThemisDB core libraries when available
        # themis_core
        # themis_llm
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        RocksDB::rocksdb
    )
    
    target_compile_definitions(test_lora_framework_comprehensive PRIVATE
        THEMIS_TEST_BUILD=1
    )
    
    # Add to CTest
    add_test(NAME LoRAFrameworkComprehensiveTests COMMAND test_lora_framework_comprehensive)
    
    # Set test properties
    set_tests_properties(LoRAFrameworkComprehensiveTests PROPERTIES
        LABELS "lora;llm;framework;comprehensive"
        TIMEOUT 600
    )
    
    message(STATUS "  - Comprehensive LoRA tests: 50+ test cases covering:")
    message(STATUS "    * Storage operations (save, load, delete, versioning)")
    message(STATUS "    * Adapter lifecycle (load, unload, hot-swap, caching)")
    message(STATUS "    * Training service (on-the-fly, batch, callbacks)")
    message(STATUS "    * Multi-adapter management (quantization, multi-GPU)")
    message(STATUS "    * Thread-safety (concurrent operations)")
    message(STATUS "    * Error handling and edge cases")
    message(STATUS "    * Memory management and leak detection")
    message(STATUS "    * Performance benchmarks")
else()
    message(STATUS "LoRA framework comprehensive tests not found")
endif()

# ============================================================================
# Existing Tests
# ============================================================================

# Add existing test files here as they are discovered
# Example pattern:
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp")
#     file(GLOB TEST_SOURCES "test_*.cpp")
#     foreach(TEST_SOURCE ${TEST_SOURCES})
#         get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
#         add_executable(${TEST_NAME} ${TEST_SOURCE})
#         target_link_libraries(${TEST_NAME} PRIVATE ${TEST_LIBS})
#         add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#     endforeach()
# endif()

# ============================================================================
# Test Discovery & Reporting
# ============================================================================

# Enable CTest XML output
set(CTEST_OUTPUT_ON_FAILURE ON)

# Add custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
    DEPENDS test_lora_framework
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all ThemisDB tests..."
)

message(STATUS "Tests configured successfully")
message(STATUS "  - Build tests with: cmake --build . --target test_lora_framework")
message(STATUS "  - Run tests with: ctest --output-on-failure")
message(STATUS "  - Or use: make run_tests")
