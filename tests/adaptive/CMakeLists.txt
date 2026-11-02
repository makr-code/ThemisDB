# adaptive module tests

file(GLOB ADAPTIVE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS ADAPTIVE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_adaptive_${_stem}_focused")
    set(_ctest "${_stem}_adaptive_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        $<$<TARGET_EXISTS:themis_query>:themis_query>
        $<$<TARGET_EXISTS:themis_sharding>:themis_sharding>
        $<$<TARGET_EXISTS:themis_storage>:themis_storage>
        $<$<TARGET_EXISTS:themis_security>:themis_security>
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE adaptive
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS adaptive
    )

    if(NOT THEMIS_BUILD_MODULAR)
        if(_stem STREQUAL "test_adaptive_flush_controller")
            add_custom_target(test_adaptive_flush_controller DEPENDS ${_target})
            add_custom_target(test_adaptive_flush_controller_focused DEPENDS ${_target})
            gtest_discover_tests(${_target}
                DISCOVERY_MODE POST_BUILD
                WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
                EXTRA_ARGS --gtest_color=yes
                CONFIGURATIONS Release Debug
            )
        elseif(_stem STREQUAL "test_adaptive_shard_router")
            add_custom_target(test_adaptive_shard_router DEPENDS ${_target})
            add_custom_target(test_adaptive_shard_router_focused DEPENDS ${_target})
            gtest_discover_tests(${_target}
                DISCOVERY_MODE POST_BUILD
                WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
                EXTRA_ARGS --gtest_color=yes
            )
        endif()
    endif()
endforeach()