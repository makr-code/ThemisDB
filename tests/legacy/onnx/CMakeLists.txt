# onnx module tests

file(GLOB ONNX_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS ONNX_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_onnx_${_stem}_focused")
    set(_ctest "${_stem}_onnx_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(EXISTS "${THEMIS_ROOT_DIR}/src/onnx_clip/onnx_clip_plugin.cpp")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/onnx_clip/onnx_clip_plugin.cpp
        )
    endif()
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_IMAGE_PLUGIN_DISABLE_EXPORT=1
    )

    themis_register_module_focused_test(
        MODULE onnx
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS onnx
    )
endforeach()