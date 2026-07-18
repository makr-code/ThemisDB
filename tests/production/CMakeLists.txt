# production module tests

file(GLOB PRODUCTION_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS PRODUCTION_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_production_${_stem}_focused")
    set(_ctest "${_stem}_production_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    if(_stem STREQUAL "test_production_validator")
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/llm/production_validator.cpp
            ${THEMIS_ROOT_DIR}/src/llm/kernel_fusion.cpp
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

    themis_register_module_focused_test(
        MODULE production
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS production
    )
endforeach()