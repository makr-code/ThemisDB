# API module tests

file(GLOB API_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS API_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_api_${_stem}_focused")
    set(_ctest "${_stem}_api_FocusedTests")
    set(_labels api)

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_api_degraded_mode" OR _stem STREQUAL "test_api_phase4_concurrency")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/api/api_transport_policy.cpp
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

    if(_stem STREQUAL "test_api_contracts" OR
       _stem STREQUAL "test_api_llm_grpc_focused" OR
       _stem STREQUAL "test_api_grpc_server")
        list(APPEND _labels wave_b wave_b_boundary_api_transport)
    endif()

    themis_register_module_focused_test(
        MODULE api
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()