# Execution module tests (Wave D — high-cardinality stress)
# Labels: wave_d;stress;not_release_critical
# These tests are excluded from the fast release_critical gate.

file(GLOB EXECUTION_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS EXECUTION_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_execution_${_stem}_stressed")
    set(_ctest  "${_stem}_ExecutionStressTests")

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
        Threads::Threads
    )

    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    add_test(NAME ${_ctest} COMMAND ${_target})
    set_tests_properties(${_ctest} PROPERTIES
        LABELS "wave_d;stress;not_release_critical;execution"
        TIMEOUT 120
    )
endforeach()
