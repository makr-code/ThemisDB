cmake_minimum_required(VERSION 3.20)

# Access Model Module Tests
# Unit and integration tests for access model coordinator

file(GLOB ACCESS_MODEL_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

set(ACCESS_MODEL_IMPL_SOURCES
    ${THEMIS_ROOT_DIR}/src/access_model/access_coordinator.cpp
    ${THEMIS_ROOT_DIR}/src/access_model/access_metrics.cpp
    ${THEMIS_ROOT_DIR}/src/access_model/access_model_logging.cpp
    ${THEMIS_ROOT_DIR}/src/access_model/access_model_trace.cpp
    ${THEMIS_ROOT_DIR}/src/access_model/age_based_policy.cpp
    ${THEMIS_ROOT_DIR}/src/access_model/promotion_demotion.cpp
)

foreach(_src IN LISTS ACCESS_MODEL_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_access_model_${_stem}_focused")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target}
        "${_src}"
    )
    if(TARGET themis_access_model)
        target_link_libraries(${_target} PRIVATE themis_access_model)
    else()
        target_sources(${_target} PRIVATE ${ACCESS_MODEL_IMPL_SOURCES})
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
    set(_access_model_gmock "${TEST_GMOCK_LIB}")
    if(NOT _access_model_gmock)
        if(TARGET GTest::gmock)
            set(_access_model_gmock GTest::gmock)
        elseif(TARGET gmock::gmock)
            set(_access_model_gmock gmock::gmock)
        elseif(TARGET GTest::gmock_main)
            set(_access_model_gmock GTest::gmock_main)
        elseif(TARGET gmock)
            set(_access_model_gmock gmock)
        endif()
    endif()
    if(_access_model_gmock)
        target_link_libraries(${_target} PRIVATE ${_access_model_gmock})
    endif()
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

