# Auto-generated module tests for prefix 'document'

file(GLOB DOCUMENT_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_document_*.cpp"
)

foreach(_src IN LISTS DOCUMENT_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_document_${_stem}_focused")
    set(_ctest "${_stem}_document_FocusedTests")

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
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    # If the module library target is not available at configure time (modular
    # builds / submodule split), provide a small fallback implementation set
    # so focused test binaries link correctly on all platforms (Windows linker
    # issues otherwise). The real implementation lives in src/document/.
    if(NOT TARGET themis_document)
        target_sources(${_target} PRIVATE
            ${THEMIS_ROOT_DIR}/src/document/round_trip_editor.cpp
        )
    else()
        target_link_libraries(${_target} PRIVATE themis_document)
    endif()

    themis_register_module_focused_test(
        MODULE document
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS document
    )
endforeach()