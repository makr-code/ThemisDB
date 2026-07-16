# ONNX CLIP Image Analysis Plugin – src/onnx_clip
# Implements IImageAnalysisPlugin (include/plugins/image_analysis_interface.h)
# using ONNX Runtime + CLIP model.
# Source and implementation header live together here.

if(NOT THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX)
    return()
endif()

message(STATUS "Configuring ONNX CLIP Image Analysis Plugin (src/onnx_clip)")

set(_ONNX_CLIP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/onnx_clip_plugin.cpp")
if(NOT EXISTS "${_ONNX_CLIP_SRC}")
    message(WARNING "ONNX CLIP plugin source not found: ${_ONNX_CLIP_SRC} – skipping build")
    return()
endif()

find_package(onnxruntime QUIET)
if(NOT onnxruntime_FOUND)
    message(WARNING "onnxruntime not found – skipping ONNX CLIP plugin")
    message(WARNING "Install with: vcpkg install onnxruntime")
    return()
endif()

find_package(nlohmann_json REQUIRED)
find_package(OpenCV COMPONENTS core imgproc imgcodecs QUIET)

add_library(themis_image_onnx_clip SHARED
    onnx_clip_plugin.cpp
)

target_include_directories(themis_image_onnx_clip
    PUBLIC
        $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${onnxruntime_INCLUDE_DIRS}
)

target_link_libraries(themis_image_onnx_clip PRIVATE
    onnxruntime::onnxruntime
    nlohmann_json::nlohmann_json
)

if(OpenCV_FOUND)
    target_link_libraries(themis_image_onnx_clip PRIVATE
        opencv_core opencv_imgproc opencv_imgcodecs
    )
    target_compile_definitions(themis_image_onnx_clip PRIVATE HAVE_OPENCV)
else()
    message(STATUS "    OpenCV not found – using stb_image fallback for ONNX CLIP")
endif()

set_target_properties(themis_image_onnx_clip PROPERTIES
    PREFIX ""
    OUTPUT_NAME "themis_image_onnx_clip"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

if(WIN32)
    target_compile_definitions(themis_image_onnx_clip PRIVATE THEMIS_PLUGIN_EXPORTS)
endif()

install(TARGETS themis_image_onnx_clip
    LIBRARY DESTINATION lib/themis/plugins
    RUNTIME DESTINATION lib/themis/plugins
    ARCHIVE DESTINATION lib/themis/plugins
)

message(STATUS "ONNX CLIP Plugin configuration complete")
