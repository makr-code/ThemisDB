# voice module tests

file(GLOB VOICE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS VOICE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_voice_${_stem}_focused")
    set(_ctest "${_stem}_voice_FocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_voice_api_handler")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/server/voice_api_handler.cpp
        )
    endif()

    if(_stem STREQUAL "test_voice_audit_logging" OR _stem STREQUAL "test_voice_multi_session_teardown")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/voice/voice_audit_logger.cpp
        )
    endif()

    if(_stem STREQUAL "test_voice_stream_validation")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/voice/voice_stream_validator.cpp
        )
    endif()

    if(_stem STREQUAL "test_voice_wave_a8_hardening_focused")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/voice/voice_liveness_detector.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_anti_spoof_engine.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_audit_logger.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_session_manager.cpp
        )
    elseif(_stem STREQUAL "test_voice_session_chaos_isolation" OR _stem STREQUAL "test_voice_stream_validation")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/voice/voice_stream_validator.cpp
            ${THEMIS_ROOT_DIR}/src/voice/voice_liveness_checker.cpp
        )
    elseif(_stem STREQUAL "test_voice_adversarial")
        set(_extra_sources
            ${THEMIS_ROOT_DIR}/tests/utils/fault_injector.cpp
        )
    endif()

    if(NOT THEMIS_BUILD_MODULAR)
        if(_stem STREQUAL "test_voice_production")
            list(APPEND _extra_sources
                ${THEMIS_ROOT_DIR}/src/voice/audio_preprocessing.cpp
                ${THEMIS_ROOT_DIR}/src/voice/wake_word_detector.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_intent_detector.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_session_manager.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_security.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_error_handler.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_tts_customizer.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_meeting_support.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_audio_storage.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_accessibility.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_model_cache.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_batch_processor.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_authenticator.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_macro_manager.cpp
                ${THEMIS_ROOT_DIR}/src/voice/emotion_analyzer.cpp
            )
        elseif(_stem STREQUAL "test_voice_coverage")
            list(APPEND _extra_sources
                ${THEMIS_ROOT_DIR}/src/voice/voice_session_manager.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_audio_storage.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_meeting_support.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_error_handler.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_security.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_tts_customizer.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_intent_detector.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_accessibility.cpp
            )
        elseif(_stem STREQUAL "test_voice_assistant")
            list(APPEND _extra_sources
                ${THEMIS_ROOT_DIR}/src/voice/voice_authenticator.cpp
                ${THEMIS_ROOT_DIR}/src/voice/wake_word_detector.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_batch_processor.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_macro_manager.cpp
                ${THEMIS_ROOT_DIR}/src/voice/audio_preprocessing.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_intent_detector.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_session_manager.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_security.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_error_handler.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_tts_customizer.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_meeting_support.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_audio_storage.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_accessibility.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_model_cache.cpp
                ${THEMIS_ROOT_DIR}/src/voice/emotion_analyzer.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_assistant.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_assistant_llm.cpp
                ${THEMIS_ROOT_DIR}/src/content/stt_processor.cpp
                ${THEMIS_ROOT_DIR}/src/content/tts_processor.cpp
            )
        elseif(_stem STREQUAL "test_voice_browser_streaming")
            list(APPEND _extra_sources
                ${THEMIS_ROOT_DIR}/src/voice/voice_browser_streaming.cpp)
        elseif(_stem STREQUAL "test_voice_security_features")
            list(APPEND _extra_sources
                ${THEMIS_ROOT_DIR}/src/voice/voice_tts_customizer.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_model_cache.cpp
                ${THEMIS_ROOT_DIR}/src/voice/voice_browser_streaming.cpp
            )
        elseif(_stem STREQUAL "test_voice_telephony")
            list(APPEND _extra_sources
                ${THEMIS_ROOT_DIR}/src/voice/voice_telephony.cpp)
        endif()
    endif()

    if(NOT TARGET ${_target})
        add_executable(${_target}
            "${_src}"
            ${_extra_sources}
        )
    else()
        target_sources(${_target} PRIVATE
            "${_src}"
            ${_extra_sources}
        )
    endif()
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
                themis_base
                spdlog::spdlog
                Threads::Threads
            )
        else()
            target_link_libraries(${_target} PRIVATE
                GTest::gtest
                GTest::gtest_main
                ${TEST_LIBS}
                themis_core
                spdlog::spdlog
                Threads::Threads
            )
        endif()
    endif()
    if(_stem STREQUAL "test_voice_production"
        OR _stem STREQUAL "test_voice_assistant"
        OR _stem STREQUAL "test_voice_security_features"
        OR _stem STREQUAL "test_voice_telephony"
        OR _stem STREQUAL "test_voice_session_manager_focused"
        OR _stem STREQUAL "test_voice_session_manager_createSession_focused"
        OR _stem STREQUAL "test_voice_create_session_focused")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
    endif()
    if((_stem STREQUAL "test_voice_assistant_generateLLMResponse_qw37"
        OR _stem STREQUAL "test_voice_assistant_verifyIdentify_qw38")
        AND TARGET themis_voice)
        target_link_libraries(${_target} PRIVATE themis_voice)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    set(_labels "voice")
    if(_stem MATCHES "test_voice_wave_a_chaos_bundle|test_voice_stream_validation|test_voice_backend_degradation_focused|test_voice_session_chaos_isolation|test_voice_wave_a_noisy_wakeword")
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