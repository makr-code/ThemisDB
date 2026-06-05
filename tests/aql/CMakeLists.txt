# AQL focused module tests (flat hierarchy under tests/aql)

file(GLOB AQL_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_aql_*.cpp"
)

foreach(_src IN LISTS AQL_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_aql_${_stem}_focused")
    set(_ctest "${_stem}_AqlFocusedTests")
    set(_extra_sources)
    string(REGEX REPLACE "^test_" "" _impl_stem "${_stem}")
    set(_impl_src "${THEMIS_ROOT_DIR}/src/aql/${_impl_stem}.cpp")
    if(EXISTS "${_impl_src}" AND NOT _stem STREQUAL "test_aql_query_validator")
        list(APPEND _extra_sources "${_impl_src}")
    endif()

    if(_stem STREQUAL "test_aql_query_validator")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/aql/aql_lora_finetuner.cpp
        )
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
        ${THEMIS_ROOT_DIR}/tests
    )
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        $<$<TARGET_EXISTS:themis_query>:themis_query>
        $<$<TARGET_EXISTS:themis_llm>:themis_llm>
        $<$<TARGET_EXISTS:themis_analytics>:themis_analytics>
        $<$<TARGET_EXISTS:themis_security>:themis_security>
        $<$<TARGET_EXISTS:themis_sharding>:themis_sharding>
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE aql
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS aql
    )
endforeach()
