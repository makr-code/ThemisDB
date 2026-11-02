# cdc module tests

file(GLOB CDC_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS CDC_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_cdc_${_stem}_focused")
    set(_ctest "${_stem}_cdc_FocusedTests")

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
    if(_stem STREQUAL "test_cdc_changefeed_core" OR
       _stem STREQUAL "test_cdc_pause_control" OR
       _stem STREQUAL "test_cdc_backpressure_signal" OR
       _stem STREQUAL "test_cdc_fan_in" OR
       _stem STREQUAL "test_cdc_event_schema" OR
       _stem STREQUAL "test_cdc_delivery_guarantee_config")
        target_link_libraries(${_target} PRIVATE nlohmann_json::nlohmann_json)
    endif()
    if(_stem STREQUAL "test_cdc_changefeed_core" OR
       _stem STREQUAL "test_cdc_pause_control" OR
       _stem STREQUAL "test_cdc_fan_in")
        target_link_libraries(${_target} PRIVATE RocksDB::rocksdb)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE cdc
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS cdc
    )
endforeach()