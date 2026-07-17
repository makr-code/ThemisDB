# federated module tests

file(GLOB FEDERATED_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS FEDERATED_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_federated_${_stem}_focused")
    set(_ctest "${_stem}_federated_FocusedTests")
    set(_extra_sources)

    if(_stem STREQUAL "test_federated_distillation_coordinator")
        list(APPEND _extra_sources
            "${THEMIS_ROOT_DIR}/src/distributed_knowledge/federated_distillation_coordinator.cpp"
        )
    endif()

    if(_stem STREQUAL "test_federated_identity_manager")
        list(APPEND _extra_sources
            "${THEMIS_ROOT_DIR}/src/auth/federated_identity_manager.cpp"
        )
    endif()

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
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
        MODULE federated
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS federated
    )
endforeach()