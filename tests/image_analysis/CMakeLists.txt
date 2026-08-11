# Image Analysis Plugin Phase 1 focused tests
# Tests: IMP-YOL-01..08 (YOLOv8) and IMP-OCR-01..08 (Tesseract OCR)

file(GLOB IMAGE_ANALYSIS_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS IMAGE_ANALYSIS_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_image_analysis_${_stem}_focused")
    set(_ctest "${_stem}_ImageAnalysisFocusedTests")

    add_executable(${_target} "${_src}")

    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )

    # Compile plugin sources directly into the test binary so the test is
    # self-contained and does not require building the shared plugin targets.
    target_sources(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/src/image_analysis/yolov8_onnx_plugin.cpp
        ${THEMIS_ROOT_DIR}/src/image_analysis/tesseract_ocr_plugin.cpp
    )

    # Disable the DLL export macro – test binary links the plugin code statically
    target_compile_definitions(${_target} PRIVATE
        THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT
        THEMIS_TEST_BUILD=1
    )

    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        $<$<TARGET_EXISTS:themis_core>:themis_core>
        $<$<TARGET_EXISTS:nlohmann_json::nlohmann_json>:nlohmann_json::nlohmann_json>
        Threads::Threads
    )

    if(OpenCV_FOUND)
        target_compile_definitions(${_target} PRIVATE HAVE_OPENCV)
        target_link_libraries(${_target} PRIVATE
            opencv_core opencv_imgproc opencv_imgcodecs)
    endif()

    themis_register_module_focused_test(
        MODULE image_analysis
        NAME   ${_ctest}
        TARGET ${_target}
        TIER   unit
        TIMEOUT 120
    )
endforeach()
