cmake_minimum_required(VERSION 3.20)

# Access Model Module
# Unified tier abstraction and coordination between cache and storage

# Feature flag
option(THEMISDB_ACCESS_COORDINATOR_ENABLED 
       "Enable unified access model coordinator" ON)

# Source files
set(ACCESS_MODEL_SOURCES
    access_coordinator.cpp
    promotion_demotion.cpp
    age_based_policy.cpp
    access_metrics.cpp
)

# Library target
add_library(themis_access_model ${ACCESS_MODEL_SOURCES})

# Dependencies
target_link_libraries(themis_access_model
  PUBLIC
    themis_core
    themis_logger
)

# Compile options
target_compile_definitions(themis_access_model
  PUBLIC
    $<$<BOOL:${THEMISDB_ACCESS_COORDINATOR_ENABLED}>:THEMISDB_ACCESS_COORDINATOR_ENABLED>
)

target_include_directories(themis_access_model
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

# Unit tests (auto-discovered)
if(THEMISDB_BUILD_TESTS)
  file(GLOB TEST_SOURCES "test_*.cpp")
  
  foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_STEM ${TEST_FILE} NAME_WE)
    set(TEST_TARGET module_access_model_${TEST_STEM}_focused)
    
    add_executable(${TEST_TARGET} ${TEST_FILE})
    target_link_libraries(${TEST_TARGET} 
      PRIVATE 
        themis_access_model
        themis_test_fixtures
        GTest::gtest
        GTest::gtest_main
    )
    
    themis_register_module_focused_test(
      TARGET ${TEST_TARGET}
      LABEL access_model
      TIMEOUT 120
    )
  endforeach()
endif()

# Installation
install(TARGETS themis_access_model
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(DIRECTORY ../../include/access_model/
  DESTINATION include/access_model
)
