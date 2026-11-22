# tests/vector_search/CMakeLists.txt
# Wave D — High-cardinality stress tests for the vector search subsystem.
# Labels: wave_d;stress;not_release_critical

# ─────────────────────────────────────────────────────────────────────────────
# Collect all stress test sources in this directory via GLOB.
# New .cpp files are automatically picked up on re-configure.
# ─────────────────────────────────────────────────────────────────────────────
file(GLOB _vs_stress_sources "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

foreach(_src IN LISTS _vs_stress_sources)
    get_filename_component(_test_name "${_src}" NAME_WE)

    if(NOT TARGET ${_test_name})
        add_executable(${_test_name} "${_src}")

        target_include_directories(${_test_name} PRIVATE
            "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/src"
            "${CMAKE_CURRENT_SOURCE_DIR}"
        )

        target_link_libraries(${_test_name} PRIVATE
            GTest::gtest
            GTest::gtest_main
            Threads::Threads
        )

        target_compile_definitions(${_test_name} PRIVATE
            THEMIS_TEST_BUILD=1
        )

        themis_register_module_test(
            MODULE vector_search
            NAME ${_test_name}
            TARGET ${_test_name}
            TIER stress
            KIND focused
            TIMEOUT 120
            LABELS wave_d stress not_release_critical
        )
    endif()
endforeach()
