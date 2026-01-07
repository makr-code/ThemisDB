cmake_minimum_required(VERSION 3.20)
project(themis_image_onnx_clip VERSION 1.0.0)

# Find dependencies
find_package(onnxruntime REQUIRED)
find_package(nlohmann_json REQUIRED)

# Optional: OpenCV for image preprocessing (fallback to stb_image if not available)
find_package(OpenCV COMPONENTS core imgproc imgcodecs)

# Plugin library
add_library(themis_image_onnx_clip SHARED
    onnx_clip_plugin.cpp
)

target_include_directories(themis_image_onnx_clip PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${onnxruntime_INCLUDE_DIRS}
)

target_link_libraries(themis_image_onnx_clip PRIVATE
    onnxruntime::onnxruntime
    nlohmann_json::nlohmann_json
)

# Add OpenCV if available
if(OpenCV_FOUND)
    target_link_libraries(themis_image_onnx_clip PRIVATE
        opencv_core
        opencv_imgproc
        opencv_imgcodecs
    )
    target_compile_definitions(themis_image_onnx_clip PRIVATE
        HAVE_OPENCV
    )
else()
    # Use stb_image as fallback
    message(STATUS "OpenCV not found, using stb_image for image loading")
    # stb_image is header-only, no linking needed
endif()

# Set plugin properties
set_target_properties(themis_image_onnx_clip PROPERTIES
    PREFIX ""  # No 'lib' prefix on Unix
    OUTPUT_NAME "themis_image_onnx_clip"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(themis_image_onnx_clip PRIVATE
        THEMIS_PLUGIN_EXPORTS
    )
endif()

# Install
install(TARGETS themis_image_onnx_clip
    LIBRARY DESTINATION lib/themis/plugins
    RUNTIME DESTINATION lib/themis/plugins
    ARCHIVE DESTINATION lib/themis/plugins
)

# Install model files (if provided)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/models")
    install(DIRECTORY models/
        DESTINATION share/themis/models/onnx_clip
        FILES_MATCHING PATTERN "*.onnx"
    )
endif()

# Install documentation
install(FILES README.md
    DESTINATION share/doc/themis/plugins/onnx_clip
)
