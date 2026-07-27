# Chimera focused module tests (flat hierarchy under tests/chimera)

file(GLOB CHIMERA_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

set(_chimera_external_adapter
    "${THEMIS_ROOT_DIR}/include/chimera/database_adapter.hpp"
)

if(NOT EXISTS "${_chimera_external_adapter}")
    message(STATUS "Chimera focused tests skipped (missing external header: ${_chimera_external_adapter})")
    return()
endif()

foreach(_src IN LISTS CHIMERA_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_chimera_${_stem}_focused")
    set(_ctest "${_stem}_ChimeraFocusedTests")

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

    themis_register_module_focused_test(
        MODULE chimera
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()
