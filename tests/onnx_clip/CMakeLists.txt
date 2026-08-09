cmake_minimum_required(VERSION 3.20)

# ONNX CLIP focused module tests (Phase 1-4 hardening)
# Tests verify golden embeddings, performance gates, hot-swap, and memory-mapped loading
# 
# Registry:
#   OCP-IT-01..12  Integration tests with golden embeddings (Phase 1)
#   OCP-HS-01..12  Hot-swap tests (Phase 3)
#   OCP-MM-01..12  Memory-mapped loading tests (Phase 4)

# Guard: only configure if THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX is enabled
if(NOT THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX)
    message(STATUS "Skipping ONNX CLIP tests (THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX disabled)")
    return()
endif()

message(STATUS "Configuring ONNX CLIP focused test suite")

# Collect all focused test sources
file(GLOB ONNX_CLIP_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*_focused.cpp"
)

if(NOT ONNX_CLIP_TEST_SOURCES)
    message(WARNING "No ONNX CLIP focused tests found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# Register each focused test
foreach(_src IN LISTS ONNX_CLIP_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_onnx_clip_${_stem}_focused")
    set(_ctest "${_stem}_OnnxClipFocused")
    
    add_executable(${_target} "${_src}")
    
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    
    # Link GoogleTest
    target_link_libraries(${_target} PRIVATE gtest gtest_main)
    
    # Link nlohmann_json for config structures
    find_package(nlohmann_json REQUIRED)
    target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
    
    # If building modularly, compile plugin sources directly into test
    if(THEMIS_BUILD_MODULAR OR NOT TARGET themis_image_onnx_clip)
        message(STATUS "    Compiling ONNX CLIP plugin sources into test target: ${_target}")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/onnx_clip/onnx_clip_plugin.cpp
        )
        target_compile_definitions(${_target} PRIVATE
            THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT
        )
    else()
        # Link against pre-built plugin
        target_link_libraries(${_target} PRIVATE themis_image_onnx_clip)
    endif()
    
    # ONNX Runtime and OpenCV dependencies
    find_package(onnxruntime QUIET)
    if(onnxruntime_FOUND)
        target_link_libraries(${_target} PRIVATE onnxruntime::onnxruntime)
    else()
        message(STATUS "    WARNING: onnxruntime not found for ${_target} - some functionality may be disabled")
    endif()
    
    find_package(OpenCV COMPONENTS core imgproc imgcodecs QUIET)
    if(OpenCV_FOUND)
        target_link_libraries(${_target} PRIVATE
            opencv_core opencv_imgproc opencv_imgcodecs
        )
        target_compile_definitions(${_target} PRIVATE HAVE_OPENCV)
    endif()
    
    # Register as CTest with focused timeout (120 seconds per Wave 1 standard)
    themis_register_module_focused_test(
        MODULE onnx_clip
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
    
    message(STATUS "    Registered: ${_target}")
    
endforeach()

message(STATUS "ONNX CLIP focused test suite configuration complete")
