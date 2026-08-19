# Utils module test registration (pilot extraction from root tests/CMakeLists.txt)

# TimestampUtils Focused Tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_timestamp_utils.cpp")
    message(STATUS "Adding TimestampUtils focused tests")

    add_executable(test_timestamp_utils_focused
        ../test_timestamp_utils.cpp
    )

    target_link_libraries(test_timestamp_utils_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_timestamp_utils_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE utils
        NAME TimestampUtilsFocusedTests
        TARGET test_timestamp_utils_focused
        TIER unit
        KIND focused
        TIMEOUT 30
        LABELS timestamp iso8601 rfc3339
    )

    message(STATUS "  TimestampUtils: ISO 8601 format/parse round-trip, milliseconds, duration formatting")
else()
    message(STATUS "TimestampUtils focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/../test_timestamp_utils.cpp")
endif()

# UtilsRateLimiter Focused Tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_utils_rate_limiter.cpp")
    message(STATUS "Adding UtilsRateLimiter focused tests")

    add_executable(test_utils_rate_limiter_focused
        ../test_utils_rate_limiter.cpp
    )

    target_link_libraries(test_utils_rate_limiter_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_utils_rate_limiter_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE utils
        NAME UtilsRateLimiterFocusedTests
        TARGET test_utils_rate_limiter_focused
        TIER unit
        KIND focused
        TIMEOUT 30
        LABELS rate-limiter token-bucket concurrency
    )

    message(STATUS "  UtilsRateLimiter: try_acquire, acquire blocking, set_rate, available")
else()
    message(STATUS "UtilsRateLimiter focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/../test_utils_rate_limiter.cpp")
endif()

# UtilsInterfaces Focused Tests (utils_interfaces.h + utils_adapters.h contract tests)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_utils_interfaces.cpp")
    message(STATUS "Adding UtilsInterfaces focused tests")

    add_executable(test_utils_interfaces_focused
        ../test_utils_interfaces.cpp
    )

    target_link_libraries(test_utils_interfaces_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_utils_interfaces_focused PRIVATE
        THEMIS_TEST_BUILD=1
        THEMIS_TEST_MODE=1
    )

    themis_register_module_test(
        MODULE utils
        NAME UtilsInterfacesFocusedTests
        TARGET test_utils_interfaces_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS interfaces adapters pii audit hkdf sampler pipeline saga
    )

    message(STATUS "  UtilsInterfaces: IStreamingPIIDetector, IHashChainAuditLog, IHKDFKeyCache, IStructuredLogSampler, ISAGALogCompactor, IUtilsPipeline")
else()
    message(STATUS "UtilsInterfaces focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/../test_utils_interfaces.cpp")
endif()

# RateLimiterV2 Focused Tests (TokenBucketRateLimiter + PerClientRateLimiter + Redis fallback)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_rate_limiter_v2.cpp")
    message(STATUS "Adding RateLimiterV2 focused tests")

    add_executable(test_rate_limiter_v2_focused
        ../test_rate_limiter_v2.cpp
    )

    target_link_libraries(test_rate_limiter_v2_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_rate_limiter_v2_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    if(THEMIS_ENABLE_REDIS AND hiredis_FOUND)
        if(TARGET hiredis::hiredis)
            target_link_libraries(test_rate_limiter_v2_focused PRIVATE hiredis::hiredis)
        elseif(TARGET hiredis)
            target_link_libraries(test_rate_limiter_v2_focused PRIVATE hiredis)
        endif()
        target_compile_definitions(test_rate_limiter_v2_focused PRIVATE THEMIS_ENABLE_REDIS=1)
    endif()

    themis_register_module_test(
        MODULE utils
        NAME RateLimiterV2FocusedTests
        TARGET test_rate_limiter_v2_focused
        TIER unit
        KIND focused
        TIMEOUT 30
        LABELS rate-limiter token-bucket redis fallback server
    )

    message(STATUS "  RateLimiterV2: local bucket, Redis fallback, per-client, priority lanes")
else()
    message(STATUS "RateLimiterV2 focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/../test_rate_limiter_v2.cpp")
endif()

# Rate Limiting Improvements Focused Tests (adaptive + cost-based)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_rate_limiting_improvements.cpp")
    message(STATUS "Adding RateLimitingImprovements focused tests")

    add_executable(test_rate_limiting_improvements_focused
        ../test_rate_limiting_improvements.cpp
    )

    target_link_libraries(test_rate_limiting_improvements_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_rate_limiting_improvements_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE utils
        NAME RateLimitingImprovementsFocusedTests
        TARGET test_rate_limiting_improvements_focused
        TIER unit
        KIND focused
        TIMEOUT 30
        LABELS rate-limiter adaptive cost-based server
    )

    message(STATUS "  RateLimitingImprovements: adaptive limiter, cost-based limiter")
else()
    message(STATUS "RateLimitingImprovements focused tests not found")
endif()

# UtilsStandalone Focused Tests
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_utils_standalone.cpp")
    message(STATUS "Adding UtilsStandalone focused tests")

    add_executable(test_utils_standalone_focused
        ../test_utils_standalone.cpp
    )

    target_link_libraries(test_utils_standalone_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        OpenSSL::Crypto
        Threads::Threads
    )

    target_compile_definitions(test_utils_standalone_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE utils
        NAME UtilsStandaloneFocusedTests
        TARGET test_utils_standalone_focused
        TIER unit
        KIND focused
        TIMEOUT 30
        LABELS checksum file normalizer feature-flags
    )

    message(STATUS "  UtilsStandalone: calculateSHA256, calculateMD5, readFileContents, Normalizer, Phase2FeatureFlags")
else()
    message(STATUS "UtilsStandalone focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/../test_utils_standalone.cpp")
endif()

# UtilsFutureInterfaces Focused Tests (UUID v7, LZ4 codec, streaming ZSTD)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_utils_future_interfaces.cpp")
    message(STATUS "Adding UtilsFutureInterfaces focused tests")

    add_executable(test_utils_future_interfaces_focused
        ../test_utils_future_interfaces.cpp
    )

    target_link_libraries(test_utils_future_interfaces_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    if(DEFINED THEMIS_LZ4_TARGET)
        target_link_libraries(test_utils_future_interfaces_focused PRIVATE ${THEMIS_LZ4_TARGET})
        target_compile_definitions(test_utils_future_interfaces_focused PRIVATE THEMIS_HAS_LZ4)
    endif()

    if(zstd_FOUND OR TARGET zstd::libzstd_shared OR TARGET zstd::libzstd_static)
        target_compile_definitions(test_utils_future_interfaces_focused PRIVATE THEMIS_HAS_ZSTD)
    endif()

    target_compile_definitions(test_utils_future_interfaces_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE utils
        NAME UtilsFutureInterfacesFocusedTests
        TARGET test_utils_future_interfaces_focused
        TIER unit
        KIND focused
        TIMEOUT 30
        LABELS uuid lz4 zstd streaming codec
    )

    message(STATUS "  UtilsFutureInterfaces: UUID v7, LZ4 codec, streaming ZSTD compressor/decompressor")
else()
    message(STATUS "UtilsFutureInterfaces focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/../test_utils_future_interfaces.cpp")
endif()

# Privacy and Audit Hardening Focused Tests (Phase A.1)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/test_utils_privacy_audit_hardening.cpp")
    message(STATUS "Adding Privacy and Audit Hardening focused tests")

    add_executable(module_utils_test_privacy_audit_hardening_focused
        test_utils_privacy_audit_hardening.cpp
    )

    target_link_libraries(module_utils_test_privacy_audit_hardening_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(module_utils_test_privacy_audit_hardening_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_test(
        MODULE utils
        NAME PrivacyAuditHardeningFocusedTests
        TARGET module_utils_test_privacy_audit_hardening_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS privacy audit hardening pii pseudonymizer unicode rate-limiting
    )

    message(STATUS "  PrivacyAuditHardening: PH-01 Unicode, PH-02 Multibyte, PH-03 BufferOverflow, PH-04 RateLimiting, PH-05 Pseudonymization, PH-06 ConcurrentWrites, PH-07 TimeoutCancellation, PH-08 RegexBacktracking")
else()
    message(STATUS "Privacy and Audit Hardening focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/test_utils_privacy_audit_hardening.cpp")
endif()

# NERDetectionEngine Focused Tests (Phase A.2 / Phase 2.2 hardening)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_ner_detection_engine.cpp")
    add_executable(module_utils_test_ner_detection_engine_focused
        test_ner_detection_engine.cpp
    )
    target_include_directories(module_utils_test_ner_detection_engine_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_utils_test_ner_detection_engine_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(module_utils_test_ner_detection_engine_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )
    themis_register_module_test(
        MODULE utils
        NAME NERDetectionEngineFocusedTests
        TARGET module_utils_test_ner_detection_engine_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS ner detection-engine privacy model-unavailable phase2
    )
    message(STATUS "  NERDetectionEngine: model availability, gazetteer degradation, config rollback, corrupt model handling")
endif()

# RegexDetectionEngine Focused Tests (Phase A.1 / Phase 2.2 hardening)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_regex_detection_engine.cpp")
    add_executable(module_utils_test_regex_detection_engine_focused
        test_regex_detection_engine.cpp
    )
    target_include_directories(module_utils_test_regex_detection_engine_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_utils_test_regex_detection_engine_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(module_utils_test_regex_detection_engine_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )
    themis_register_module_test(
        MODULE utils
        NAME RegexDetectionEngineFocusedTests
        TARGET module_utils_test_regex_detection_engine_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS regex detection-engine privacy redos timeout phase2
    )
    message(STATUS "  RegexDetectionEngine: timeout, ReDoS detection, malformed patterns, UTF-8 validation, bounds checking")
endif()

# Serialization Focused Tests (Phase A.4 hardening)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_serialization.cpp")
    add_executable(module_utils_test_serialization_focused
        test_serialization.cpp
    )
    target_include_directories(module_utils_test_serialization_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_utils_test_serialization_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(module_utils_test_serialization_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )
    themis_register_module_test(
        MODULE utils
        NAME SerializationFocusedTests
        TARGET module_utils_test_serialization_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS serialization bounds-check overflow malformed phase2
    )
    message(STATUS "  Serialization: buffer overflow prevention, malformed data, round-trip, oversized claims")
endif()

# TSAN Stress / Concurrency Tests (Phase 2.3 / Phase 5 — intended for TSAN-enabled builds)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_tsan_stress_concurrent.cpp")
    add_executable(module_utils_test_tsan_stress_concurrent_focused
        test_tsan_stress_concurrent.cpp
    )
    target_include_directories(module_utils_test_tsan_stress_concurrent_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_utils_test_tsan_stress_concurrent_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(module_utils_test_tsan_stress_concurrent_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )
    themis_register_module_test(
        MODULE utils
        NAME TSANConcurrencyStressTests
        TARGET module_utils_test_tsan_stress_concurrent_focused
        TIER unit
        KIND stress
        TIMEOUT 120
        LABELS tsan concurrency stress audit-logger thread-pool pii-scanner phase2 phase5
    )
    message(STATUS "  TSANConcurrencyStress: audit_logger 128-writer, thread_pool saturation, pii_stream_scanner parallel slots")
endif()

# Phase 1 Resource Management Tests (Phase 4.1 — RAII, threading, exception safety)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_phase1_resource_management.cpp")
    add_executable(module_utils_test_phase1_resource_management_focused
        test_phase1_resource_management.cpp
    )
    target_include_directories(module_utils_test_phase1_resource_management_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(module_utils_test_phase1_resource_management_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )
    target_compile_definitions(module_utils_test_phase1_resource_management_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )
    themis_register_module_test(
        MODULE utils
        NAME Phase1ResourceManagementTests
        TARGET module_utils_test_phase1_resource_management_focused
        TIER unit
        KIND focused
        TIMEOUT 60
        LABELS raii resource-management threading exception-safety phase1 phase4
    )
    message(STATUS "  Phase1ResourceManagement: RAII patterns, thread management, lock contention, destructor ordering")
endif()

# AUTOGEN PREFIX BLOCK: utils
file(GLOB UTILS_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_utils_*.cpp"
)

# Exclude files that are already registered as explicit focused targets
list(REMOVE_ITEM UTILS_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_utils_privacy_audit_hardening.cpp"
)

foreach(_src IN LISTS UTILS_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_utils_${_stem}_autofocused")
    set(_ctest "${_stem}_utils_AutoFocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
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
        MODULE utils
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS utils autogen
    )
endforeach()