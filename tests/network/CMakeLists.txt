# Network/transport focused module tests

file(GLOB NETWORK_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS NETWORK_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_network_${_stem}_focused")
    set(_ctest "${_stem}_network_FocusedTests")
    set(_labels network transport)

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_udp_server")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/network/udp_server.cpp
        )
    endif()
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

    if(_stem STREQUAL "test_network_contract_hardening_focused" OR
       _stem STREQUAL "test_network_transport_resilience_focused" OR
       _stem STREQUAL "test_wire_protocol_integration")
        list(APPEND _labels wave_b wave_b_boundary_api_transport)
    endif()

    themis_register_module_focused_test(
        MODULE network
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()