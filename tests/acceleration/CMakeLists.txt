# Acceleration module test registration (pilot extraction from root tests/CMakeLists.txt)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../test_backend_registry_startup.cpp")
    message(STATUS "Adding BackendRegistry startup focused tests (acceleration, Issue #37, v1.7.0)")

    add_executable(test_backend_registry_startup_focused
        ../test_backend_registry_startup.cpp
    )

    target_include_directories(test_backend_registry_startup_focused PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )

    target_link_libraries(test_backend_registry_startup_focused PRIVATE
        ${TEST_LIBS}
        themis_core
        Threads::Threads
    )

    target_compile_definitions(test_backend_registry_startup_focused PRIVATE
        THEMIS_TEST_BUILD=1
    )

    themis_register_module_focused_test(
        MODULE acceleration
        NAME BackendRegistryStartupFocusedTests
        TARGET test_backend_registry_startup_focused
        TIER unit
        TIMEOUT 60
        LABELS backend-registry capability-negotiation v1.7.0 thread-safety
    )

    message(STATUS "  BackendRegistry: initializeRuntime, default requirements, capability-driven selection, re-initialization, consistency with selectXBackendFor, thread-safety")
else()
    message(STATUS "BackendRegistry startup focused tests not found at ${CMAKE_CURRENT_LIST_DIR}/../test_backend_registry_startup.cpp")
endif()

# AUTOGEN PREFIX BLOCK: acceleration
file(GLOB ACCELERATION_AUTOGEN_PREFIX_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_acceleration_*.cpp"
)

foreach(_src IN LISTS ACCELERATION_AUTOGEN_PREFIX_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_acceleration_${_stem}_autofocused")
    set(_ctest "${_stem}_acceleration_AutoFocusedTests")

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

    themis_register_module_focused_test(
        MODULE acceleration
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS acceleration autogen
    )
endforeach()