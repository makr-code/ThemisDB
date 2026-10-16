# toolbox module tests

file(GLOB TOOLBOX_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS TOOLBOX_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_toolbox_${_stem}_focused")
    set(_ctest "${_stem}_toolbox_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    set(_toolbox_impl_sources
        ${THEMIS_ROOT_DIR}/src/toolbox/text_chunker.cpp
        ${THEMIS_ROOT_DIR}/src/toolbox/text_normalizer.cpp
        ${THEMIS_ROOT_DIR}/src/toolbox/content_fingerprinter.cpp
        ${THEMIS_ROOT_DIR}/src/toolbox/text_quality_scorer.cpp
        ${THEMIS_ROOT_DIR}/src/toolbox/language_detector.cpp
        ${THEMIS_ROOT_DIR}/src/toolbox/toolbox_composite.cpp
        ${THEMIS_ROOT_DIR}/src/toolbox/toolbox_streaming.cpp
        ${THEMIS_ROOT_DIR}/src/toolbox/ingestion_toolbox.cpp
        ${THEMIS_ROOT_DIR}/src/toolbox/toolbox_registry.cpp
        ${THEMIS_ROOT_DIR}/src/ingestion/ingestion_sinks.cpp
    )

    add_executable(${_target} "${_src}" ${_toolbox_impl_sources})
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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE toolbox
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS toolbox
    )
endforeach()