cmake_minimum_required(VERSION 3.20)
# distributed_knowledge module tests

file(GLOB DISTRIBUTED_KNOWLEDGE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS DISTRIBUTED_KNOWLEDGE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    # Strip trailing _focused suffix (files are already named *_focused.cpp) to
    # avoid a doubled "_focused_focused" in the generated target name.
    string(REGEX REPLACE "_focused$" "" _base_stem "${_stem}")
    set(_target "module_distributed_knowledge_${_base_stem}_focused")
    set(_ctest "${_base_stem}_distributed_knowledge_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    set(_dk_focused_impl_sources)
    if(NOT TARGET themis_distributed_knowledge)
        list(APPEND _dk_focused_impl_sources
            ${THEMIS_ROOT_DIR}/src/distributed_knowledge/adapter_capability_announcement.cpp
            ${THEMIS_ROOT_DIR}/src/distributed_knowledge/lora_federation_coordinator.cpp
            ${THEMIS_ROOT_DIR}/src/distributed_knowledge/federated_distillation_coordinator.cpp
            ${THEMIS_ROOT_DIR}/src/distributed_knowledge/federated_rag_merger.cpp
            ${THEMIS_ROOT_DIR}/src/distributed_knowledge/cross_shard_feedback_sync.cpp
            ${THEMIS_ROOT_DIR}/src/governance/cross_border_transfer.cpp
            ${THEMIS_ROOT_DIR}/src/llm/decision_record_yaml_processor.cpp
        )
    endif()

    add_executable(${_target} "${_src}" ${_dk_focused_impl_sources})
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

    if(TARGET themis_distributed_knowledge)
        target_link_libraries(${_target} PRIVATE themis_distributed_knowledge)
    else()
        message(STATUS "${_target}: themis_distributed_knowledge target missing at configure time; using direct module sources")
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE distributed_knowledge
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS distributed_knowledge
    )
endforeach()
