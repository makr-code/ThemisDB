cmake_minimum_required(VERSION 3.20)

# Compatibility shim: compiled image-analysis plugins now build from src/* and
# are gated through cmake/features/PluginFeatures.cmake. Keep this entry point
# for legacy workflows that still reference plugins/image_analysis.

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

message(STATUS "plugins/image_analysis: processing legacy compatibility entry point")

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/onnx_clip/CMakeLists.txt")
    add_subdirectory(onnx_clip)
endif()

foreach(_legacy_subdir IN ITEMS llamacpp_vision opencv_dnn stable_diffusion_onnx)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_legacy_subdir}/CMakeLists.txt")
        message(STATUS "plugins/image_analysis: retaining legacy-only subdirectory ${_legacy_subdir}")
        add_subdirectory(${_legacy_subdir})
    endif()
endforeach()
