# ThemisDB – src/image_analysis
# Phase 1: YOLOv8 object detection + Tesseract OCR plugins
#
# Build flags:
#   THEMIS_PLUGIN_IMAGE_YOLOV8    – enable YOLOv8 plugin
#   THEMIS_PLUGIN_IMAGE_TESSERACT – enable Tesseract OCR plugin
#
# Both targets fall back gracefully when optional dependencies
# (onnxruntime / tesseract) are not available.

cmake_minimum_required(VERSION 3.20)

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

find_package(nlohmann_json QUIET)
find_package(OpenCV COMPONENTS core imgproc imgcodecs QUIET)

# ---------------------------------------------------------------------------
# Helper: apply common settings to a plugin target
# ---------------------------------------------------------------------------
function(themis_image_plugin_defaults tgt)
    target_include_directories(${tgt}
        PUBLIC
            $<BUILD_INTERFACE:${THEMIS_ROOT_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    if(nlohmann_json_FOUND)
        target_link_libraries(${tgt} PRIVATE nlohmann_json::nlohmann_json)
    endif()
    if(OpenCV_FOUND)
        target_compile_definitions(${tgt} PRIVATE HAVE_OPENCV)
        target_link_libraries(${tgt} PRIVATE opencv_core opencv_imgproc opencv_imgcodecs)
    endif()
    set_target_properties(${tgt} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE ON
        PREFIX ""
    )
    if(WIN32)
        target_compile_definitions(${tgt} PRIVATE THEMIS_PLUGIN_EXPORTS)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# YOLOv8 ONNX Plugin
# ---------------------------------------------------------------------------
if(THEMIS_PLUGIN_IMAGE_YOLOV8)
    message(STATUS "Configuring YOLOv8 ONNX image analysis plugin")

    find_package(onnxruntime QUIET)

    add_library(themis_image_yolov8 SHARED
        yolov8_onnx_plugin.cpp
    )
    themis_image_plugin_defaults(themis_image_yolov8)

    if(onnxruntime_FOUND)
        target_compile_definitions(themis_image_yolov8 PRIVATE HAVE_ONNXRUNTIME)
        target_link_libraries(themis_image_yolov8 PRIVATE onnxruntime::onnxruntime)
        target_include_directories(themis_image_yolov8 PRIVATE ${onnxruntime_INCLUDE_DIRS})
        message(STATUS "  YOLOv8 plugin: ONNX Runtime found – full inference enabled")
    else()
        message(WARNING "  YOLOv8 plugin: ONNX Runtime not found – stub mode (no real inference)")
    endif()

    # Disable auto-export macro in non-shared usage (e.g. test builds linking statically)
    set_property(TARGET themis_image_yolov8 APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS "THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT"
    )

    install(TARGETS themis_image_yolov8
        LIBRARY DESTINATION lib/themis/plugins
        RUNTIME DESTINATION lib/themis/plugins
        ARCHIVE DESTINATION lib/themis/plugins
    )
endif()

# ---------------------------------------------------------------------------
# Tesseract OCR Plugin
# ---------------------------------------------------------------------------
if(THEMIS_PLUGIN_IMAGE_TESSERACT)
    message(STATUS "Configuring Tesseract OCR image analysis plugin")

    find_package(Tesseract QUIET)
    # Some distributions export the target differently
    if(NOT Tesseract_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(TESSERACT QUIET tesseract)
        endif()
    endif()

    add_library(themis_image_tesseract SHARED
        tesseract_ocr_plugin.cpp
    )
    themis_image_plugin_defaults(themis_image_tesseract)

    if(Tesseract_FOUND OR TESSERACT_FOUND)
        target_compile_definitions(themis_image_tesseract PRIVATE HAVE_TESSERACT)
        if(TARGET Tesseract::libtesseract)
            target_link_libraries(themis_image_tesseract PRIVATE Tesseract::libtesseract)
        elseif(TESSERACT_LIBRARIES)
            target_link_libraries(themis_image_tesseract PRIVATE ${TESSERACT_LIBRARIES})
            target_include_directories(themis_image_tesseract PRIVATE ${TESSERACT_INCLUDE_DIRS})
        endif()
        message(STATUS "  Tesseract plugin: Tesseract found – full OCR enabled")
    else()
        message(WARNING "  Tesseract plugin: Tesseract not found – stub mode (no real OCR)")
    endif()

    set_property(TARGET themis_image_tesseract APPEND PROPERTY
        INTERFACE_COMPILE_DEFINITIONS "THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT"
    )

    install(TARGETS themis_image_tesseract
        LIBRARY DESTINATION lib/themis/plugins
        RUNTIME DESTINATION lib/themis/plugins
        ARCHIVE DESTINATION lib/themis/plugins
    )
endif()

# ---------------------------------------------------------------------------
# Static library used by unit tests (always built when tests are enabled)
# ---------------------------------------------------------------------------
if(THEMIS_BUILD_TESTS OR BUILD_TESTING)
    add_library(themis_image_analysis_test_lib STATIC
        yolov8_onnx_plugin.cpp
        tesseract_ocr_plugin.cpp
    )
    target_compile_definitions(themis_image_analysis_test_lib
        PUBLIC THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT)
    themis_image_plugin_defaults(themis_image_analysis_test_lib)
endif()
