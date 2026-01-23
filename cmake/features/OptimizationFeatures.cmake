# ThemisDB Performance Optimization Features
# Phase 1, 2, and 3 optimizations

# Architecture-specific optimizations
if(NOT DEFINED THEMIS_ENABLE_AVX2)
    option(THEMIS_ENABLE_AVX2 "Enable AVX2 SIMD" ON)
endif()

if(NOT DEFINED THEMIS_QNAP_BUILD)
    option(THEMIS_QNAP_BUILD "Build for QNAP NAS" OFF)
endif()

# Phase 1 optimizations
if(NOT DEFINED THEMIS_ENABLE_RCU_INDEX)
    option(THEMIS_ENABLE_RCU_INDEX "Enable RCU Index for lock-free reads" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_LIRS_CACHE)
    option(THEMIS_ENABLE_LIRS_CACHE "Enable LIRS cache replacement policy" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_HUGE_PAGES)
    option(THEMIS_ENABLE_HUGE_PAGES "Enable huge pages (Linux-only)" OFF)
endif()

# Auto-enable Phase 1 features if building tests or benchmarks
if(NOT DEFINED THEMIS_PERF_AUTO_ENABLE)
    option(THEMIS_PERF_AUTO_ENABLE "Auto-enable Phase 1 features when building tests/benchmarks" ON)
endif()

if(THEMIS_PERF_AUTO_ENABLE AND (THEMIS_BUILD_TESTS OR THEMIS_BUILD_BENCHMARKS))
    set(THEMIS_ENABLE_RCU_INDEX ON CACHE BOOL "Enable RCU Index for lock-free reads" FORCE)
    set(THEMIS_ENABLE_LIRS_CACHE ON CACHE BOOL "Enable LIRS cache replacement policy" FORCE)
    message(STATUS "  >> Auto-enabling Phase 1 performance optimizations")
endif()

# Phase 2 optimizations
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

# Phase 3 optimizations
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

# Display optimization features
message(STATUS "  Performance Optimizations:")
if(THEMIS_ENABLE_AVX2)
    message(STATUS "    AVX2: Enabled")
endif()
if(THEMIS_QNAP_BUILD)
    message(STATUS "    QNAP build: Enabled (SSE4.2 only)")
endif()

# Phase 1
if(THEMIS_ENABLE_RCU_INDEX OR THEMIS_ENABLE_LIRS_CACHE OR THEMIS_ENABLE_HUGE_PAGES)
    message(STATUS "    Phase 1:")
    if(THEMIS_ENABLE_RCU_INDEX)
        message(STATUS "      RCU Index: Enabled")
    endif()
    if(THEMIS_ENABLE_LIRS_CACHE)
        message(STATUS "      LIRS Cache: Enabled")
    endif()
    if(THEMIS_ENABLE_HUGE_PAGES)
        message(STATUS "      Huge Pages: Enabled")
    endif()
endif()

# Phase 2
if(THEMIS_ENABLE_WISCKEY OR THEMIS_ENABLE_DOSTOEVSKY OR THEMIS_ENABLE_CICADA OR 
   THEMIS_ENABLE_LIGRA OR THEMIS_ENABLE_RABITQ)
    message(STATUS "    Phase 2:")
    if(THEMIS_ENABLE_WISCKEY)
        message(STATUS "      WiscKey: Enabled")
    endif()
    if(THEMIS_ENABLE_DOSTOEVSKY)
        message(STATUS "      Dostoevsky: Enabled")
    endif()
    if(THEMIS_ENABLE_CICADA)
        message(STATUS "      Cicada: Enabled")
    endif()
    if(THEMIS_ENABLE_LIGRA)
        message(STATUS "      Ligra: Enabled")
    endif()
    if(THEMIS_ENABLE_RABITQ)
        message(STATUS "      RaBitQ: Enabled")
    endif()
endif()

# Phase 3
if(THEMIS_ENABLE_DISKANN OR THEMIS_ENABLE_BWTREE OR THEMIS_ENABLE_SPLINTERDB OR 
   THEMIS_ENABLE_GUNROCK OR THEMIS_ENABLE_BAO)
    message(STATUS "    Phase 3:")
    if(THEMIS_ENABLE_DISKANN)
        message(STATUS "      DiskANN: Enabled")
    endif()
    if(THEMIS_ENABLE_BWTREE)
        message(STATUS "      Bw-Tree: Enabled")
    endif()
    if(THEMIS_ENABLE_SPLINTERDB)
        message(STATUS "      SplinterDB: Enabled")
    endif()
    if(THEMIS_ENABLE_GUNROCK)
        message(STATUS "      Gunrock: Enabled")
    endif()
    if(THEMIS_ENABLE_BAO)
        message(STATUS "      Bao: Enabled")
    endif()
endif()
