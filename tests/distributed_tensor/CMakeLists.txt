cmake_minimum_required(VERSION 3.20)
# Distributed tensor module focused test registration.

set(DIST_TENSOR_BLOCK_A_TESTS
    test_tensor_allocation.cpp
    test_tensor_operations.cpp
    test_tensor_performance.cpp
)

foreach(_src_name IN LISTS DIST_TENSOR_BLOCK_A_TESTS)
    set(_src "${CMAKE_CURRENT_SOURCE_DIR}/${_src_name}")
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_distributed_tensor_${_stem}_focused")
    set(_ctest "${_stem}_distributed_tensor_FocusedTests")

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
        themis_distributed_tensor
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE distributed_tensor
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS distributed_tensor
    )
endforeach()
