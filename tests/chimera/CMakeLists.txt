# Chimera focused module tests (flat hierarchy under tests/chimera)

file(GLOB CHIMERA_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

set(_chimera_external_adapter
    "${THEMIS_ROOT_DIR}/include/chimera/database_adapter.hpp"
)

set(_chimera_real_adapter_src
    "${THEMIS_ROOT_DIR}/src/chimera/themisdb_adapter.cpp"
)

set(_chimera_adapter_factory_src
    "${THEMIS_ROOT_DIR}/external/chimera/src/chimera/adapter_factory.cpp"
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
    # Prefer the real adapter implementation whenever available.
    # Keep the fallback stub only for reduced builds where adapter sources
    # are intentionally omitted.
    if(EXISTS "${_chimera_real_adapter_src}")
        target_sources(${_target} PRIVATE "${_chimera_real_adapter_src}")
        if(EXISTS "${_chimera_adapter_factory_src}")
            target_sources(${_target} PRIVATE "${_chimera_adapter_factory_src}")
        endif()
    else()
        target_sources(${_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/themis_adapter_fallback.cpp)
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
    if(NOT EXISTS "${_chimera_real_adapter_src}")
        # Mark focused chimera test builds that include the fallback stub.
        # Tools can detect `CHIMERA_FALLBACK_STUB` to avoid treating this
        # translation unit as production code.
        target_compile_definitions(${_target} PRIVATE CHIMERA_FALLBACK_STUB=1)
    endif()

    themis_register_module_focused_test(
        MODULE chimera
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
    )
endforeach()
