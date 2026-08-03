cmake_minimum_required(VERSION 3.20)

# Access Model Module Tests
# Unit and integration tests for access model coordinator

file(GLOB ACCESS_MODEL_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS ACCESS_MODEL_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_access_model_${_stem}_focused")

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
        gmock
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE access_model
        NAME ${_stem}_access_model_FocusedTests
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS "access_model;tier;coordinator;promotion;demotion"
    )
endforeach()

