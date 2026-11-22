# cache module tests

file(GLOB CACHE_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS CACHE_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_cache_${_stem}_focused")
    set(_ctest "${_stem}_cache_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    set(_extra_sources)
    if(_stem STREQUAL "test_cache_replication")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/cache/cache_replication_coordinator.cpp
            ${THEMIS_ROOT_DIR}/src/utils/logger.cpp
        )
    elseif(_stem STREQUAL "test_cache_replication_coordinator")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/cache/cache_replication_coordinator.cpp
            ${THEMIS_ROOT_DIR}/src/cache/grpc_remote_cache_peer.cpp
            ${THEMIS_ROOT_DIR}/src/utils/logger.cpp
        )
    endif()

    add_executable(${_target} "${_src}" ${_extra_sources})
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
    if(_stem STREQUAL "test_cache_admin_api_handler" OR
       _stem STREQUAL "test_cache_warmup" OR
       _stem STREQUAL "test_cache_replication" OR
       _stem STREQUAL "test_cache_replication_coordinator")
        target_link_libraries(${_target} PRIVATE RocksDB::rocksdb)
    endif()
    if(_stem STREQUAL "test_cache_replication_coordinator" AND THEMIS_ENABLE_GRPC)
        target_link_libraries(${_target} PRIVATE gRPC::grpc++)
        target_compile_definitions(${_target} PRIVATE THEMIS_ENABLE_GRPC)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    # Phase 3 P3-02 tests get additional labels
    if(_stem MATCHES "test_cache_efficiency_multilevel")
        set(_labels "cache;phase3;p3-02;cache_efficiency")
    else()
        set(_labels "cache")
    endif()

    themis_register_module_focused_test(
        MODULE cache
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS ${_labels}
    )
endforeach()