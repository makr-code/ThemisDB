# ThemisDB Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Check if we're building tests
if(NOT THEMIS_BUILD_TESTS)
    message(STATUS "Tests disabled")
    return()
endif()

message(STATUS "Configuring ThemisDB test suite")

if(MSVC)
    message(STATUS "MSVC Runtime: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()

find_package(GTest QUIET CONFIG)
if(NOT GTest_FOUND)
    message(WARNING "GTest not found")
    return()
endif()

message(STATUS "GTest found - building unified test binary")

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${GTest_INCLUDE_DIRS}
)

file(GLOB_RECURSE ALL_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_minimal\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_manual.*\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*benchmark.*\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_wire_protocol_manual\\.cpp$")
list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX ".*test_env\\.cpp$")

# Skip feature-dependent tests when the corresponding modules are disabled
set(TEST_EXCLUDE_PATTERNS "")

if(NOT THEMIS_ENABLE_LLM)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_llm.*\\.cpp$"
        ".*test_lora.*\\.cpp$"
        ".*test_llama.*\\.cpp$"
        ".*test_kv_cache.*\\.cpp$"
        ".*test_meta_prompt.*\\.cpp$"
        ".*test_mixed_precision.*\\.cpp$"
        ".*test_multi_gpu.*\\.cpp$"
        ".*test_multi_lora.*\\.cpp$"
        ".*test_paged_attention\\.cpp$"
        ".*test_production_validator\\.cpp$"
        ".*test_prompt_optimizer\\.cpp$"
        ".*test_qlora.*\\.cpp$"
        ".*test_quantization\\.cpp$"
        ".*test_rag_judge.*\\.cpp$"
        ".*test_raid_lora_integration\\.cpp$"
        ".*test_training_service_registry\\.cpp$"
        ".*test_signature_verifier\\.cpp$"
        ".*test_ethics_aware_confidence_detector\\.cpp$"
        ".*test_ethical_guidelines_manager\\.cpp$"
        ".*test_explanation_generator\\.cpp$"
        ".*test_fewshot_optimizer\\.cpp$"
        ".*test_flash_lora\\.cpp$"
        ".*test_fused_kernels\\.cpp$"
        ".*test_gguf_loader\\.cpp$"
        ".*test_gpu_lora_layers\\.cpp$"
        ".*test_gradient_checkpointing\\.cpp$"
        ".*test_inference_engine_enhanced\\.cpp$"
        ".*test_kernel_fusion\\.cpp$"
        ".*test_model_loader_async\\.cpp$"
        ".*test_phase1_flash_attention\\.cpp$"
        ".*test_phase1_kv_cache_reuse\\.cpp$"
        ".*test_ai_decision_auditor\\.cpp$"
        ".*test_byzantine_detector\\.cpp$"
        ".*test_continuous_batch_scheduler\\.cpp$"
        ".*test_data_loader\\.cpp$"
        ".*test_directx_backend\\.cpp$"
        ".*test_docs_assistant_aql\\.cpp$"
        ".*test_claim_extractor\\.cpp$"
        ".*test_adapter_sync\\.cpp$"
        ".*test_adapter_factory\\.cpp$"
        ".*test_cross_functional_voice_observability\\.cpp$"
        ".*test_process_mining_e2e\\.cpp$"
        ".*test_extended_context\\.cpp$"
        ".*test_model_loader_error_handling\\.cpp$"
        ".*test_model_loading_from_themisdb\\.cpp$"
        ".*test_real_embeddings\\.cpp$"
        ".*test_adaptive_query_cache\\.cpp$"
    )
endif()

if(NOT THEMIS_ENABLE_CONTENT)
    list(APPEND TEST_EXCLUDE_PATTERNS
        ".*test_office_processor\\.cpp$"
        ".*test_text_processor\\.cpp$"
        ".*test_mock_clip\\.cpp$"
        ".*test_http_content\\.cpp$"
        ".*test_content_fs\\.cpp$"
        ".*test_content_policy.*\\.cpp$"
    )
endif()

# Missing implementations and unfinished features
list(APPEND TEST_EXCLUDE_PATTERNS
    ".*test_merge_operator.*\\.cpp$"
    ".*test_shard_durability.*\\.cpp$"
    ".*test_gpu_erasure_coding.*\\.cpp$"
    ".*test_mfa_authenticator.*\\.cpp$"
    ".*test_binary_integrity.*\\.cpp$"
    ".*test_enhanced_backup.*\\.cpp$"
    ".*test_gunrock.*\\.cpp$"
    ".*test_splinterdb.*\\.cpp$"
    ".*test_aql_shortestpath.*\\.cpp$"
    ".*test_phase3.*\\.cpp$"
)
foreach(pattern IN LISTS TEST_EXCLUDE_PATTERNS)
    list(FILTER ALL_TEST_SOURCES EXCLUDE REGEX "${pattern}")
endforeach()

list(LENGTH ALL_TEST_SOURCES TEST_SOURCE_COUNT)
message(STATUS "Building ${TEST_SOURCE_COUNT} test files after feature gating")

add_executable(themis_tests ${ALL_TEST_SOURCES})

target_link_libraries(themis_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    themis_core
    spdlog::spdlog
    fmt::fmt
    nlohmann_json::nlohmann_json
    RocksDB::rocksdb
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    protobuf::libprotobuf
    gRPC::grpc++
    Threads::Threads
)

if(TARGET llama)
    target_link_libraries(themis_tests PRIVATE llama)
endif()

target_compile_definitions(themis_tests PRIVATE
    THEMIS_TEST_BUILD=1
)

add_test(NAME AllTests COMMAND themis_tests)
set_tests_properties(AllTests PROPERTIES TIMEOUT 600)

add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

message(STATUS "Tests configured successfully")
