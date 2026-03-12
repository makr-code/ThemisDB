# Image Analysis Plugins – migrated to src/plugins/image_analysis
# Sources live here; public headers live in include/plugins/image_analysis/

if(NOT THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX)
    return()
endif()

message(STATUS "Configuring Image Analysis Plugins (src/plugins/image_analysis)")

find_package(nlohmann_json REQUIRED)

# ONNX CLIP Plugin
if(THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX)
    message(STATUS "  - ONNX CLIP Plugin")
    add_subdirectory(onnx_clip)
endif()
