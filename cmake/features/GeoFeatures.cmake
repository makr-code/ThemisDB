# cmake/features/GeoFeatures.cmake
# Geospatial module GPU backend configuration
# Handles CUDA/HIP dispatch, fallback paths, and performance gating

# ============================================================================
# Geo GPU Backend Selection (THEMIS_GEO_CUDA takes precedence)
# ============================================================================

# THEMIS_GEO_CUDA — Enable CUDA kernel dispatch for geospatial operations
if(NOT DEFINED THEMIS_GEO_CUDA)
    # Default: ON if THEMIS_ENABLE_CUDA is ON, otherwise OFF
    if(THEMIS_ENABLE_CUDA)
        option(THEMIS_GEO_CUDA "Enable CUDA kernel dispatch in geo GPU backend" ON)
    else()
        option(THEMIS_GEO_CUDA "Enable CUDA kernel dispatch in geo GPU backend" OFF)
    endif()
endif()

# THEMIS_GEO_HIP — Enable HIP kernel dispatch for geospatial operations (AMD ROCm)
# HIP is only considered if CUDA is not enabled
if(NOT THEMIS_GEO_CUDA)
    if(NOT DEFINED THEMIS_GEO_HIP)
        if(THEMIS_ENABLE_HIP)
            option(THEMIS_GEO_HIP "Enable HIP kernel dispatch in geo GPU backend (AMD ROCm)" ON)
        else()
            option(THEMIS_GEO_HIP "Enable HIP kernel dispatch in geo GPU backend (AMD ROCm)" OFF)
        endif()
    endif()
else()
    # CUDA takes precedence: force HIP off
    set(THEMIS_GEO_HIP OFF CACHE BOOL "HIP disabled when CUDA is enabled" FORCE)
endif()

# ============================================================================
# Geo Performance Gates — Release-critical performance targets
# ============================================================================

# Gate-A-06-01: Haversine distance batch compute (1k points, CUDA)
set(THEMIS_GEO_GATE_HAVERSINE_P99_MS "10" CACHE STRING "Haversine p99 latency target (ms)")

# Gate-A-06-02: Point-in-polygon batch compute (10k points, CUDA)
set(THEMIS_GEO_GATE_PIP_P99_MS "2" CACHE STRING "Point-in-polygon p99 latency target (ms)")

# Gate-A-06-03: GeoJSON parsing (typical polygon, CPU)
set(THEMIS_GEO_GATE_GEOJSON_PARSE_P99_US "500" CACHE STRING "GeoJSON parse p99 latency target (µs)")

# Circuit-breaker: max consecutive GPU dispatch failures before fallback
set(THEMIS_GEO_CIRCUIT_BREAKER_THRESHOLD "5" CACHE STRING "Max GPU failures before circuit-breaker fallback")

# ============================================================================
# Diagnostics & Instrumentation
# ============================================================================

# Enable geo backend diagnostics logging
option(THEMIS_GEO_DIAGNOSTICS_DETAILED "Enable detailed geo backend dispatch diagnostics" ON)

# Validate geometry strictly (may incur performance cost)
option(THEMIS_GEO_STRICT_VALIDATION "Enable strict geometry validation before dispatch" ON)

# ============================================================================
# Status Report
# ============================================================================

message(STATUS "══ Geospatial (Geo) Module Features ══")

if(THEMIS_GEO_CUDA)
    message(STATUS "  GPU Backend: CUDA (THEMIS_GEO_CUDA=ON)")
    message(STATUS "    - Haversine kernel dispatch enabled")
    message(STATUS "    - Point-in-polygon kernel dispatch enabled")
    message(STATUS "    - Vincenty distance kernel dispatch enabled")
elseif(THEMIS_GEO_HIP)
    message(STATUS "  GPU Backend: HIP/ROCm (THEMIS_GEO_HIP=ON)")
    message(STATUS "    - Haversine kernel dispatch enabled (ROCm)")
    message(STATUS "    - Point-in-polygon kernel dispatch enabled (ROCm)")
else()
    message(STATUS "  GPU Backend: CPU-only fallback (THEMIS_GEO_CUDA=OFF, THEMIS_GEO_HIP=OFF)")
    message(STATUS "    - All geo operations will use CPU dispatch paths")
endif()

message(STATUS "  Performance Gates:")
message(STATUS "    - Haversine (GATE-A-06-01): p99 ≤ ${THEMIS_GEO_GATE_HAVERSINE_P99_MS}ms")
message(STATUS "    - Point-in-Polygon (GATE-A-06-02): p99 ≤ ${THEMIS_GEO_GATE_PIP_P99_MS}ms")
message(STATUS "    - GeoJSON Parse (GATE-A-06-03): p99 ≤ ${THEMIS_GEO_GATE_GEOJSON_PARSE_P99_US}µs")
message(STATUS "  Circuit-breaker threshold: ${THEMIS_GEO_CIRCUIT_BREAKER_THRESHOLD} GPU failures")

if(THEMIS_GEO_DIAGNOSTICS_DETAILED)
    message(STATUS "  Diagnostics: DETAILED (on)")
    add_compile_definitions(THEMIS_GEO_DIAGNOSTICS_DETAILED)
endif()

if(THEMIS_GEO_STRICT_VALIDATION)
    message(STATUS "  Geometry Validation: STRICT (on)")
    add_compile_definitions(THEMIS_GEO_STRICT_VALIDATION)
endif()

# ============================================================================
# Compile Definitions for Geo Module
# ============================================================================

if(THEMIS_GEO_CUDA)
    add_compile_definitions(THEMIS_GEO_CUDA)
endif()

if(THEMIS_GEO_HIP)
    add_compile_definitions(THEMIS_GEO_HIP)
endif()

# Gate threshold as compile-time constant
add_compile_definitions(THEMIS_GEO_CIRCUIT_BREAKER_THRESHOLD=${THEMIS_GEO_CIRCUIT_BREAKER_THRESHOLD})
