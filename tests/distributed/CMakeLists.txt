# distributed module tests

file(GLOB DISTRIBUTED_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS DISTRIBUTED_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_distributed_${_stem}_focused")
    set(_ctest "${_stem}_distributed_FocusedTests")

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

    if(_stem STREQUAL "test_distributed_knowledge")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
        if(TARGET themis_distributed_knowledge)
            target_link_libraries(${_target} PRIVATE themis_distributed_knowledge)
        else()
            target_sources(${_target} PRIVATE
                ${THEMIS_ROOT_DIR}/src/distributed_knowledge/adapter_capability_announcement.cpp
                ${THEMIS_ROOT_DIR}/src/distributed_knowledge/lora_federation_coordinator.cpp
                ${THEMIS_ROOT_DIR}/src/distributed_knowledge/federated_rag_merger.cpp
                ${THEMIS_ROOT_DIR}/src/distributed_knowledge/cross_shard_feedback_sync.cpp
            )
        endif()
        set_target_properties(${_target} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
        )
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
    elseif(_stem STREQUAL "test_distributed_hybrid_search")
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
            CONFIGURATIONS Release Debug
        )
    endif()

    themis_register_module_focused_test(
        MODULE distributed
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS distributed
    )
endforeach()