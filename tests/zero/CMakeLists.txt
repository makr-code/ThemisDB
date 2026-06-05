# zero module tests

file(GLOB ZERO_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS ZERO_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_zero_${_stem}_focused")
    set(_ctest "${_stem}_zero_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_zero_copy_blob_transfers")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/storage/zero_copy_blob_transfer.cpp
            ${THEMIS_ROOT_DIR}/src/utils/logger.cpp
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
        spdlog::spdlog
        Threads::Threads
    )
    if(_stem STREQUAL "test_zero_copy_blob_transfers")
        target_link_libraries(${_target} PRIVATE
            OpenSSL::SSL
            OpenSSL::Crypto
        )
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE zero
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS zero
    )
endforeach()