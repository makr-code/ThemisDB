# gossip module tests

file(GLOB GOSSIP_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS GOSSIP_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_gossip_${_stem}_focused")
    set(_ctest "${_stem}_gossip_FocusedTests")

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

    if(_stem STREQUAL "test_gossip_custom_handler")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
        set_target_properties(${_target} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
        )
        add_test(NAME GossipCustomHandlerTests
            COMMAND $<TARGET_FILE:${_target}>)
        set_tests_properties(GossipCustomHandlerTests PROPERTIES
            LABELS "distributed_knowledge;gossip;custom_handler;session2;focused;unit"
            TIMEOUT 60
        )
        gtest_discover_tests(${_target}
            DISCOVERY_MODE POST_BUILD
            WORKING_DIRECTORY $<TARGET_FILE_DIR:${_target}>
            EXTRA_ARGS --gtest_color=yes
        )
    endif()

    themis_register_module_focused_test(
        MODULE gossip
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS gossip
    )
endforeach()