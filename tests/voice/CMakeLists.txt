# voice module tests

file(GLOB VOICE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS VOICE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_voice_${_stem}_focused")
    set(_ctest "${_stem}_voice_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_voice_api_handler")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/server/voice_api_handler.cpp
        )
    endif()

    if(_stem STREQUAL "test_voice_wave_a8_hardening_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/voice/voice_liveness_detector.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_anti_spoof_engine.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_audit_logger.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_session_manager.cpp
        )
    elseif(_stem STREQUAL "test_voice_adversarial")
        set(_extra_sources
            ${THEMIS_ROOT_DIR}/tests/utils/fault_injector.cpp
        )
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    if(_stem STREQUAL "test_voice_adversarial")
        target_link_libraries(${_target} PRIVATE
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
        )
    else()
        if(_stem STREQUAL "test_voice_wave_a8_hardening_focused")
            target_link_libraries(${_target} PRIVATE
                GTest::gtest
                GTest::gtest_main
                spdlog::spdlog
                Threads::Threads
            )
        else()
            target_link_libraries(${_target} PRIVATE
                ${TEST_LIBS}
                themis_core
                spdlog::spdlog
                Threads::Threads
            )
        endif()
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    set(_labels "voice")
    if(_stem MATCHES "test_voice_wave_a_chaos_bundle|test_voice_stream_validation|test_voice_backend_degradation_focused|test_voice_session_chaos_isolation")
        list(APPEND _labels "wave_a" "release_critical")
    endif()

    themis_register_module_focused_test(
        MODULE voice
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()