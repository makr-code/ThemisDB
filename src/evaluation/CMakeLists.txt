# EPIC 2 — Evaluation & Benchmarking module.
# Includes benchmark_matrix (EPIC 2.2, Issue #5438) and hardware_profile sources.

# ============================================================================
# themis_evaluation — EPIC 2 evaluation library
# ============================================================================

set(EVALUATION_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/benchmark_matrix.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/hardware_profile.h"
)

set(EVALUATION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark_matrix.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hardware_profile.cc"
)

if(NOT TARGET themis_evaluation)
    add_library(themis_evaluation STATIC ${EVALUATION_SOURCES})

    target_include_directories(themis_evaluation
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include/evaluation>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_features(themis_evaluation PUBLIC cxx_std_17)

    # No external dependencies: evaluation library is self-contained.
    target_compile_definitions(themis_evaluation PRIVATE THEMIS_EVALUATION_BUILD=1)

    install(TARGETS themis_evaluation
        ARCHIVE DESTINATION lib
        COMPONENT evaluation
    )
    install(FILES ${EVALUATION_PUBLIC_HEADERS}
        DESTINATION include/evaluation
        COMPONENT evaluation
    )
endif()
