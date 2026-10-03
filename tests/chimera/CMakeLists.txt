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
    # Each module-scoped focused test needs the concrete adapter implementation
    # that the test exercises; otherwise the binary links against only the generic
    # ThemisDB adapter and leaves the specific backend symbols unresolved.
    set(_chimera_impl_sources "")
    if(EXISTS "${_chimera_real_adapter_src}")
        list(APPEND _chimera_impl_sources "${_chimera_real_adapter_src}")
        if(EXISTS "${_chimera_adapter_factory_src}")
            list(APPEND _chimera_impl_sources "${_chimera_adapter_factory_src}")
        endif()
    endif()

    if(EXISTS "${THEMIS_ROOT_DIR}/src/chimera/transaction.cpp")
        list(APPEND _chimera_impl_sources "${THEMIS_ROOT_DIR}/src/chimera/transaction.cpp")
    endif()

    if(_stem MATCHES "mongo")
        list(APPEND _chimera_impl_sources "${THEMIS_ROOT_DIR}/src/chimera/mongodb_adapter.cpp")
    endif()

    if(NOT _chimera_impl_sources)
        target_sources(${_target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/themis_adapter_fallback.cpp)
    else()
        target_sources(${_target} PRIVATE ${_chimera_impl_sources})
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
