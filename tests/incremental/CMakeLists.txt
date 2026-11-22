# incremental module tests

file(GLOB INCREMENTAL_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS INCREMENTAL_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_incremental_${_stem}_focused")
    set(_ctest "${_stem}_incremental_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_incremental_lora_trainer")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
        if(THEMIS_ENABLE_LLM)
            target_sources(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/training/incremental_lora_trainer.cpp
                ${THEMIS_ROOT_DIR}/src/training/lora_checkpoint_manager.cpp
                ${THEMIS_ROOT_DIR}/src/training/adapter_serving.cpp
                ${THEMIS_ROOT_DIR}/src/training/lora_data_selection.cpp
                ${THEMIS_ROOT_DIR}/src/distributed_knowledge/lora_federation_coordinator.cpp
                ${THEMIS_ROOT_DIR}/src/distributed_knowledge/adapter_capability_announcement.cpp
                ${THEMIS_ROOT_DIR}/src/utils/logger.cpp
            )
        endif()
    endif()
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
    if(_stem STREQUAL "test_incremental_lora_trainer" AND THEMIS_ENABLE_LLM)
        target_link_libraries(${_target} PRIVATE
            nlohmann_json::nlohmann_json
            $<$<TARGET_EXISTS:yaml-cpp::yaml-cpp>:yaml-cpp::yaml-cpp>
        )
        target_compile_definitions(${_target} PRIVATE THEMIS_ENABLE_LLM=1)
        add_test(NAME IncrementalLoRATrainerFederationTests
            COMMAND $<TARGET_FILE:${_target}>)
        set_tests_properties(IncrementalLoRATrainerFederationTests PROPERTIES
            LABELS "training;lora;federation;gradient;IMPL-A3;focused;unit"
            TIMEOUT 60
        )
    endif()

    themis_register_module_focused_test(
        MODULE incremental
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS incremental
    )
endforeach()
