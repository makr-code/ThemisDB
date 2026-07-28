# EPIC 1 Retrieval tests — lora_package_test.cc + Phase A gate tests

# Guard: only build when the retrieval library target is available.
if(NOT TARGET themis_retrieval)
    return()
endif()

set(EPIC1_RETRIEVAL_TESTS
    lora_package_test.cc
    test_ann_frontdoor_single_shard.cpp
)

foreach(_src IN LISTS EPIC1_RETRIEVAL_TESTS)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "epic1_retrieval_${_stem}_focused")
    set(_ctest  "${_stem}_epic1_retrieval_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/src/retrieval/include
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_link_libraries(${_target} PRIVATE
        themis_retrieval
        $<$<TARGET_EXISTS:themis_index>:themis_index>
        ${TEST_LIBS}
        Threads::Threads
    )
    target_compile_features(${_target} PRIVATE cxx_std_17)
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    # Per-source labels
    if(_stem STREQUAL "test_ann_frontdoor_single_shard")
        set(_extra_labels epic1_phase_a ann_frontdoor)
    else()
        set(_extra_labels lora_package)
    endif()

    themis_register_module_focused_test(
        MODULE epic1_retrieval
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS epic1_retrieval ${_extra_labels}
    )
endforeach()
