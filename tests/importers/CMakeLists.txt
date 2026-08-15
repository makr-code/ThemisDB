# importers module tests — Phase 4 Block 2 hardening

file(GLOB IMPORTERS_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS IMPORTERS_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_importers_${_stem}_focused")
    set(_ctest  "${_stem}_importers_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    if(DEFINED _themis_vcpkg_include_dir AND _themis_vcpkg_include_dir)
        target_include_directories(${_target} SYSTEM PRIVATE
            ${_themis_vcpkg_include_dir}
        )
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        Threads::Threads
    )
    if(TARGET themis_core)
        target_link_libraries(${_target} PRIVATE themis_core)
    endif()
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE importers
        NAME   ${_ctest}
        TARGET ${_target}
        TIER   unit
        TIMEOUT 120
        LABELS  importers phase4
    )
endforeach()
