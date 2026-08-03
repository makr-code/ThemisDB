cmake_minimum_required(VERSION 3.20)

# Access Model Module Tests
# Unit and integration tests for access model coordinator

# Source files
file(GLOB TEST_SOURCES "test_*.cpp")

# Build each test
foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_STEM ${TEST_FILE} NAME_WE)
    set(TEST_TARGET module_access_model_${TEST_STEM}_focused)
    
    add_executable(${TEST_TARGET} ${TEST_FILE})
    
    target_link_libraries(${TEST_TARGET}
      PRIVATE
        themis_access_model
        themis_core
        themis_test_fixtures
        GTest::gtest
        GTest::gtest_main
        gmock
    )
    
    # Register test with CTest
    if(DEFINED themis_register_module_focused_test)
        themis_register_module_focused_test(
            TARGET ${TEST_TARGET}
            LABEL access_model
            TIMEOUT 120
        )
    else()
        add_test(
            NAME ${TEST_TARGET}
            COMMAND ${TEST_TARGET}
        )
        set_tests_properties(${TEST_TARGET} PROPERTIES
            LABELS "access_model;tier;coordinator"
            TIMEOUT 120
        )
    endif()
endforeach()

