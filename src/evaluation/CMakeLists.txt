# EPIC 2 — Evaluation & Benchmarking module.
# Issue #5438: benchmark_matrix (EPIC 2.2) implemented.

# ============================================================================
# themis_evaluation — EPIC 2 evaluation library
# ============================================================================

set(EVALUATION_PUBLIC_HEADERS
    include/benchmark_matrix.h
)

set(EVALUATION_SOURCES
    src/benchmark_matrix.cc
)

if(NOT TARGET themis_evaluation)
    add_library(themis_evaluation STATIC ${EVALUATION_SOURCES})

    target_include_directories(themis_evaluation
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_compile_features(themis_evaluation PUBLIC cxx_std_17)

    # No external dependencies: benchmark_matrix is self-contained.
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
