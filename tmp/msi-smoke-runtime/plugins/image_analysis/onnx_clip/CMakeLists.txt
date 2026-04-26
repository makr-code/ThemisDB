cmake_minimum_required(VERSION 3.20)

# Compatibility shim: runtime plugin implementation has been integrated at
# src/onnx_clip. Keep this file for legacy paths under plugins/image_analysis/onnx_clip.
if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

set(_onnx_src_dir "${THEMIS_ROOT_DIR}/src/onnx_clip")

if(EXISTS "${_onnx_src_dir}/CMakeLists.txt")
    message(STATUS "plugins/image_analysis/onnx_clip: delegating build to src/onnx_clip")
    if(NOT TARGET themis_onnx_clip)
        add_subdirectory("${_onnx_src_dir}" "${CMAKE_CURRENT_BINARY_DIR}/src_onnx_clip")
    endif()
else()
    message(FATAL_ERROR "plugins/image_analysis/onnx_clip: expected integrated source at ${_onnx_src_dir}")
endif()
