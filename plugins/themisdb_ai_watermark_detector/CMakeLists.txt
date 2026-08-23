cmake_minimum_required(VERSION 3.20)

# Plugin library name
set(PROJECT_NAME themisdb_ai_watermark_detector)

# Header files
set(AI_WATERMARK_DETECTOR_HEADERS
    include/detection_result.h
    include/detection_config.h
    include/detector_interface.h
    include/token_distribution_analyzer.h
)

# Source files
set(AI_WATERMARK_DETECTOR_SOURCES
    src/detector_impl.cpp
    src/token_analyzer.cpp
    src/detection_config.cpp
    src/detection_result.cpp
)

# Create library
add_library(${PROJECT_NAME} STATIC
    ${AI_WATERMARK_DETECTOR_HEADERS}
    ${AI_WATERMARK_DETECTOR_SOURCES}
)

# Set C++ standard
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

# Include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Dependencies: nlohmann/json (header-only, typically already included in ThemisDB)
if(TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
else()
    # Fallback: assume header-only included via vcpkg or system
    message(WARNING "nlohmann_json target not found; assuming header-only availability")
endif()

# Compiler flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Testing
if(BUILD_TESTING)
    enable_testing()
    
    # Test executable
    add_executable(test_watermark_detector
        tests/test_detector_basics.cpp
    )
    
    target_link_libraries(test_watermark_detector
        PRIVATE
            ${PROJECT_NAME}
            GTest::gtest
            GTest::gtest_main
    )
    
    target_compile_features(test_watermark_detector PRIVATE cxx_std_17)
    
    add_test(NAME WatermarkDetectorBasics
             COMMAND test_watermark_detector)
endif()

# Installation (optional)
install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

install(FILES ${AI_WATERMARK_DETECTOR_HEADERS}
        DESTINATION include/themisdb/watermark)

message(STATUS "    AI Watermark Detector plugin configured successfully")
