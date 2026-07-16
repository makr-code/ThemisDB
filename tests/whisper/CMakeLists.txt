# Auto-generated module tests for prefix 'whisper'

file(GLOB WHISPER_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_whisper_*.cpp"
)

foreach(_src IN LISTS WHISPER_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_whisper_${_stem}_focused")
    set(_ctest "${_stem}_whisper_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_whisper_stub_transcribe_bridge")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/whisper/whisper_plugin.cpp
            ${THEMIS_ROOT_DIR}/src/whisper/whisper_config.cpp
            ${THEMIS_ROOT_DIR}/src/whisper/audio_chunk_reader.cpp
            ${THEMIS_ROOT_DIR}/src/utils/logger.cpp
        )
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE whisper
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS whisper
    )
endforeach()