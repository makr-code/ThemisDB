# EPIC 2 — Evaluation & Benchmarking
# Hardware-profile sources are now present; the remaining EPIC 2 targets stay opt-in.

set(EVALUATION_PUBLIC_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/hardware_profile.h"
)

set(EVALUATION_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hardware_profile.cc"
)

# Parent build integration remains deferred until the broader evaluation module is enabled.
