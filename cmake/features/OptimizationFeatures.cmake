# ThemisDB Performance Optimization Features
# Phase 1, 2, and 3 optimizations
#
# Design principle (inverted flags):
#   All Phase 1 features that have complete implementations are ACTIVE by default.
#   They are disabled explicitly via THEMIS_DISABLE_* when hardware is unavailable
#   or strict compatibility is required.
#
#   Phase 2/3 research features (WiscKey, DiskANN, …) remain opt-IN (OFF) because
#   they require external libraries or are not yet production-complete.

# ── Architecture-specific optimizations ──────────────────────────────────────
if(NOT DEFINED THEMIS_ENABLE_AVX2)
    option(THEMIS_ENABLE_AVX2 "Enable AVX2 SIMD" ON)
endif()

if(NOT DEFINED THEMIS_QNAP_BUILD)
    option(THEMIS_QNAP_BUILD "Build for QNAP NAS" OFF)
endif()

# ── Phase 1: fully-implemented features – ON by default ──────────────────────
# Disable explicitly when the target hardware does not support the feature
# or when strict isolation is needed (e.g. QNAP build, embedded target).

# RCU Index: lock-free reads via Read-Copy-Update
# Disable: -DTHEMIS_DISABLE_RCU_INDEX=ON
option(THEMIS_DISABLE_RCU_INDEX "Disable RCU Index (active by default)" OFF)
if(NOT THEMIS_DISABLE_RCU_INDEX)
    set(THEMIS_ENABLE_RCU_INDEX ON CACHE BOOL "RCU Index active" FORCE)
else()
    set(THEMIS_ENABLE_RCU_INDEX OFF CACHE BOOL "RCU Index disabled by THEMIS_DISABLE_RCU_INDEX" FORCE)
endif()

# LIRS cache replacement policy (scan-resistant, ~30-40% better hit rate)
# Disable: -DTHEMIS_DISABLE_LIRS_CACHE=ON
option(THEMIS_DISABLE_LIRS_CACHE "Disable LIRS cache (active by default)" OFF)
if(NOT THEMIS_DISABLE_LIRS_CACHE)
    set(THEMIS_ENABLE_LIRS_CACHE ON CACHE BOOL "LIRS cache active" FORCE)
else()
    set(THEMIS_ENABLE_LIRS_CACHE OFF CACHE BOOL "LIRS cache disabled by THEMIS_DISABLE_LIRS_CACHE" FORCE)
endif()

# Huge pages: 2 MB / 1 GB TLB optimization (Linux + Windows)
# Runtime-graceful: allocate_huge_pages() falls back to regular mmap when not
# available even if the flag is ON (no kernel panic / crash risk).
# Disable explicitly: -DTHEMIS_DISABLE_HUGE_PAGES=ON
option(THEMIS_DISABLE_HUGE_PAGES "Disable huge pages (active by default on Linux)" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR WIN32)
    if(NOT THEMIS_DISABLE_HUGE_PAGES)
        set(THEMIS_ENABLE_HUGE_PAGES ON CACHE BOOL "Huge pages active" FORCE)
        add_compile_definitions(THEMIS_USE_HUGE_PAGES=1)
    else()
        set(THEMIS_ENABLE_HUGE_PAGES OFF CACHE BOOL "Huge pages disabled by THEMIS_DISABLE_HUGE_PAGES" FORCE)
    endif()
else()
    set(THEMIS_ENABLE_HUGE_PAGES OFF CACHE BOOL "Huge pages not supported on this platform" FORCE)
endif()

# io_uring zero-copy I/O (Linux kernel ≥ 5.1)
# Runtime-graceful: IoUringZeroCopyIO::is_available() returns false and falls
# back to standard send()/recv() when io_uring is not accessible.
# Disable explicitly: -DTHEMIS_DISABLE_IO_URING=ON
option(THEMIS_DISABLE_IO_URING "Disable io_uring zero-copy I/O (active by default on Linux)" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT THEMIS_DISABLE_IO_URING)
        set(THEMIS_ENABLE_IO_URING ON CACHE BOOL "io_uring active" FORCE)
        add_compile_definitions(THEMIS_ENABLE_IO_URING=1)
    else()
        set(THEMIS_ENABLE_IO_URING OFF CACHE BOOL "io_uring disabled by THEMIS_DISABLE_IO_URING" FORCE)
    endif()
else()
    set(THEMIS_ENABLE_IO_URING OFF CACHE BOOL "io_uring not available on this platform" FORCE)
endif()

# Legacy auto-enable switch kept for backward compatibility; now a no-op since
# Phase 1 features are ON by default.
if(NOT DEFINED THEMIS_PERF_AUTO_ENABLE)
    option(THEMIS_PERF_AUTO_ENABLE "Legacy: auto-enable Phase 1 features (no-op, features are ON by default)" ON)
endif()

# ── Phase 2 optimizations: opt-in (require external libraries) ───────────────
if(NOT DEFINED THEMIS_ENABLE_WISCKEY)
    option(THEMIS_ENABLE_WISCKEY "Enable WiscKey" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_DOSTOEVSKY)
    option(THEMIS_ENABLE_DOSTOEVSKY "Enable Dostoevsky" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_CICADA)
    option(THEMIS_ENABLE_CICADA "Enable Cicada" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_LIGRA)
    option(THEMIS_ENABLE_LIGRA "Enable Ligra" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_RABITQ)
    option(THEMIS_ENABLE_RABITQ "Enable RaBitQ" OFF)
endif()

# ── Phase 3 optimizations: opt-in (require external libraries) ───────────────
if(NOT DEFINED THEMIS_ENABLE_DISKANN)
    option(THEMIS_ENABLE_DISKANN "Enable DiskANN" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_BWTREE)
    option(THEMIS_ENABLE_BWTREE "Enable Bw-Tree" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_SPLINTERDB)
    option(THEMIS_ENABLE_SPLINTERDB "Enable SplinterDB" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_GUNROCK)
    option(THEMIS_ENABLE_GUNROCK "Enable Gunrock" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_BAO)
    option(THEMIS_ENABLE_BAO "Enable Bao" OFF)
endif()

# ── Status summary ────────────────────────────────────────────────────────────
message(STATUS "  Performance Optimizations:")
if(THEMIS_ENABLE_AVX2)
    message(STATUS "    AVX2: ON")
endif()
if(THEMIS_QNAP_BUILD)
    message(STATUS "    QNAP build: Enabled (SSE4.2 only)")
endif()

# Phase 1 (inverted: shown as ON/DISABLED)
message(STATUS "    Phase 1 (active by default):")
if(THEMIS_ENABLE_RCU_INDEX)
    message(STATUS "      RCU Index:  ON  (disable: -DTHEMIS_DISABLE_RCU_INDEX=ON)")
else()
    message(STATUS "      RCU Index:  DISABLED")
endif()
if(THEMIS_ENABLE_LIRS_CACHE)
    message(STATUS "      LIRS Cache: ON  (disable: -DTHEMIS_DISABLE_LIRS_CACHE=ON)")
else()
    message(STATUS "      LIRS Cache: DISABLED")
endif()
if(THEMIS_ENABLE_HUGE_PAGES)
    message(STATUS "      Huge Pages: ON  (runtime-graceful fallback; disable: -DTHEMIS_DISABLE_HUGE_PAGES=ON)")
else()
    message(STATUS "      Huge Pages: DISABLED (platform not supported or disabled explicitly)")
endif()
if(THEMIS_ENABLE_IO_URING)
    message(STATUS "      io_uring:   ON  (runtime-graceful fallback; disable: -DTHEMIS_DISABLE_IO_URING=ON)")
else()
    message(STATUS "      io_uring:   DISABLED (Linux-only or disabled explicitly)")
endif()

# Phase 2 (opt-in)
if(THEMIS_ENABLE_WISCKEY OR THEMIS_ENABLE_DOSTOEVSKY OR THEMIS_ENABLE_CICADA OR
   THEMIS_ENABLE_LIGRA OR THEMIS_ENABLE_RABITQ)
    message(STATUS "    Phase 2 (opt-in, requires external libs):")
    if(THEMIS_ENABLE_WISCKEY)
        message(STATUS "      WiscKey: ON")
    endif()
    if(THEMIS_ENABLE_DOSTOEVSKY)
        message(STATUS "      Dostoevsky: ON")
    endif()
    if(THEMIS_ENABLE_CICADA)
        message(STATUS "      Cicada: ON")
    endif()
    if(THEMIS_ENABLE_LIGRA)
        message(STATUS "      Ligra: ON")
    endif()
    if(THEMIS_ENABLE_RABITQ)
        message(STATUS "      RaBitQ: ON")
    endif()
endif()

# Phase 3 (opt-in)
if(THEMIS_ENABLE_DISKANN OR THEMIS_ENABLE_BWTREE OR THEMIS_ENABLE_SPLINTERDB OR
   THEMIS_ENABLE_GUNROCK OR THEMIS_ENABLE_BAO)
    message(STATUS "    Phase 3 (opt-in, requires external libs):")
    if(THEMIS_ENABLE_DISKANN)
        message(STATUS "      DiskANN: ON")
    endif()
    if(THEMIS_ENABLE_BWTREE)
        message(STATUS "      Bw-Tree: ON")
    endif()
    if(THEMIS_ENABLE_SPLINTERDB)
        message(STATUS "      SplinterDB: ON")
    endif()
    if(THEMIS_ENABLE_GUNROCK)
        message(STATUS "      Gunrock: ON")
    endif()
    if(THEMIS_ENABLE_BAO)
        message(STATUS "      Bao: ON")
    endif()
endif()
