# cmake/SecurityHardening.cmake
# ──────────────────────────────────────────────────────────────────────────────
# Platform-aware security hardening flags for ThemisDB.
#
# Applied automatically when included after CompilerOptions.cmake.
# All hardening is enabled by default for Release builds; individual knobs can
# be turned off via cache variables.
#
# GCC / Clang (Linux / macOS)
# ────────────────────────────
#   -D_FORTIFY_SOURCE=3        Buffer-overflow detection via libc wrappers (GCC 12+)
#                              Falls back to level 2 on older toolchains.
#   -fstack-protector-strong   Stack canary on functions with buffers / ptrs.
#   -fPIE / -pie               Position-independent executable → ASLR-friendly.
#   -Wl,-z,relro,-z,now        Full RELRO (read-only GOT after dynamic linking).
#   -Wl,--as-needed            Strip unused dynamic library entries (reduces attack
#                              surface and startup time).
#   -fcf-protection=full       Control-flow integrity (x86 / x86-64 only, CET).
#
# MSVC (Windows)
# ───────────────
#   /GS                        Buffer Security Check (stack cookie).
#   /DYNAMICBASE               ASLR support (address space layout randomization).
#   /NXCOMPAT                  Data Execution Prevention (DEP / NX bit).
#   /CETCOMPAT                 Control-flow Enforcement Technology (CET), VS 2022+.
#
# Opt-out per knob (set BEFORE including this file or via cmake -D):
#   THEMIS_DISABLE_FORTIFY_SOURCE   OFF → _FORTIFY_SOURCE skipped
#   THEMIS_DISABLE_STACK_PROTECTOR  OFF → stack canary skipped
#   THEMIS_DISABLE_PIE              OFF → position-independent exe skipped
#   THEMIS_DISABLE_RELRO            OFF → RELRO skipped (Linux linker flag)
#   THEMIS_DISABLE_CFI              OFF → control-flow integrity skipped
# ──────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.20)

# Only meaningful for supported compilers
if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|MSVC|AppleClang"))
    message(STATUS "[SecurityHardening] Unknown compiler – skipping hardening flags.")
    return()
endif()

# ── Opt-out knobs ──────────────────────────────────────────────────────────────
option(THEMIS_DISABLE_FORTIFY_SOURCE   "Disable _FORTIFY_SOURCE libc buffer checks"     OFF)
option(THEMIS_DISABLE_STACK_PROTECTOR  "Disable stack canary (-fstack-protector-strong)" OFF)
option(THEMIS_DISABLE_PIE              "Disable position-independent executable (PIE)"   OFF)
option(THEMIS_DISABLE_RELRO            "Disable RELRO linker hardening (Linux only)"     OFF)
option(THEMIS_DISABLE_CFI              "Disable control-flow integrity flags"            OFF)

# ── GCC / Clang / AppleClang ──────────────────────────────────────────────────
if(NOT MSVC)

    # ── Stack protector ────────────────────────────────────────────────────────
    if(NOT THEMIS_DISABLE_STACK_PROTECTOR)
        add_compile_options(-fstack-protector-strong)
        message(STATUS "[SecurityHardening] Stack protector: -fstack-protector-strong")
    else()
        message(STATUS "[SecurityHardening] Stack protector: DISABLED (THEMIS_DISABLE_STACK_PROTECTOR=ON)")
    endif()

    # ── _FORTIFY_SOURCE ────────────────────────────────────────────────────────
    # Only valid in optimised builds (requires -O1 or higher).
    # _FORTIFY_SOURCE=3 needs GCC ≥ 12 / Clang ≥ 17; fall back to =2 otherwise.
    if(NOT THEMIS_DISABLE_FORTIFY_SOURCE)
        if(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
            set(_fortify_level 2)
            if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
               CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.0")
                set(_fortify_level 3)
            elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang" AND
                   CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "17.0")
                set(_fortify_level 3)
            endif()

            # Remove any previous _FORTIFY_SOURCE definition to avoid -Wmacro-redefined
            add_compile_options(-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=${_fortify_level})
            message(STATUS "[SecurityHardening] _FORTIFY_SOURCE=${_fortify_level} (${CMAKE_BUILD_TYPE} build)")
        else()
            message(STATUS "[SecurityHardening] _FORTIFY_SOURCE: skipped (Debug build – requires -O1+)")
        endif()
    else()
        message(STATUS "[SecurityHardening] _FORTIFY_SOURCE: DISABLED (THEMIS_DISABLE_FORTIFY_SOURCE=ON)")
    endif()

    # ── Position-independent executable (PIE) ─────────────────────────────────
    # Not applicable on macOS (always PIC) or when building shared libs.
    if(NOT THEMIS_DISABLE_PIE AND NOT APPLE AND NOT BUILD_SHARED_LIBS)
        add_compile_options(-fPIE)
        add_link_options(-pie)
        message(STATUS "[SecurityHardening] PIE: -fPIE -pie")
    else()
        if(APPLE)
            message(STATUS "[SecurityHardening] PIE: skipped (macOS – always PIC)")
        elseif(BUILD_SHARED_LIBS)
            message(STATUS "[SecurityHardening] PIE: skipped (shared libs build)")
        else()
            message(STATUS "[SecurityHardening] PIE: DISABLED (THEMIS_DISABLE_PIE=ON)")
        endif()
    endif()

    # ── RELRO + bind-now (Linux only) ─────────────────────────────────────────
    if(NOT THEMIS_DISABLE_RELRO AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_link_options(
            -Wl,-z,relro
            -Wl,-z,now
        )
        message(STATUS "[SecurityHardening] RELRO: -Wl,-z,relro -Wl,-z,now")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(STATUS "[SecurityHardening] RELRO: DISABLED (THEMIS_DISABLE_RELRO=ON)")
    endif()

    # ── --as-needed (Linux only) ──────────────────────────────────────────────
    # Strips unused DT_NEEDED entries → smaller binary, reduced attack surface.
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_link_options(-Wl,--as-needed)
        message(STATUS "[SecurityHardening] Linker: -Wl,--as-needed")
    endif()

    # ── Control-flow integrity / Intel CET ────────────────────────────────────
    # -fcf-protection is x86 / x86-64 specific; skip on ARM / RISC-V.
    if(NOT THEMIS_DISABLE_CFI)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|x64|i686|i386)$")
            # Only GCC and Clang >= 8 support -fcf-protection
            if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
               CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8.0")
                add_compile_options(-fcf-protection=full)
                message(STATUS "[SecurityHardening] CFI: -fcf-protection=full (GCC, x86-64)")
            elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang" AND
                   CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "8.0")
                add_compile_options(-fcf-protection=full)
                message(STATUS "[SecurityHardening] CFI: -fcf-protection=full (Clang, x86-64)")
            else()
                message(STATUS "[SecurityHardening] CFI: skipped (compiler too old or unknown)")
            endif()
        else()
            message(STATUS "[SecurityHardening] CFI: skipped (non-x86 architecture: ${CMAKE_SYSTEM_PROCESSOR})")
        endif()
    else()
        message(STATUS "[SecurityHardening] CFI: DISABLED (THEMIS_DISABLE_CFI=ON)")
    endif()

# ── MSVC ──────────────────────────────────────────────────────────────────────
else()

    # /GS – Buffer Security Check (enabled by default, but explicit for clarity)
    if(NOT THEMIS_DISABLE_STACK_PROTECTOR)
        add_compile_options(/GS)
        message(STATUS "[SecurityHardening] MSVC: /GS (Buffer Security Check)")
    else()
        add_compile_options(/GS-)
        message(STATUS "[SecurityHardening] MSVC: /GS- (Buffer Security Check DISABLED)")
    endif()

    # /DYNAMICBASE – ASLR support
    if(NOT THEMIS_DISABLE_PIE)
        add_link_options(/DYNAMICBASE)
        message(STATUS "[SecurityHardening] MSVC: /DYNAMICBASE (ASLR)")
    endif()

    # /NXCOMPAT – Data Execution Prevention
    add_link_options(/NXCOMPAT)
    message(STATUS "[SecurityHardening] MSVC: /NXCOMPAT (DEP/NX)")

    # /CETCOMPAT – Control-flow Enforcement Technology (VS 2022 17.0+)
    if(NOT THEMIS_DISABLE_CFI)
        if(MSVC_VERSION GREATER_EQUAL 1930)
            add_link_options(/CETCOMPAT)
            message(STATUS "[SecurityHardening] MSVC: /CETCOMPAT (Intel CET, VS 2022+)")
        else()
            message(STATUS "[SecurityHardening] MSVC: /CETCOMPAT skipped (VS 2019 or older)")
        endif()
    endif()

    # /HIGHENTROPYVA – 64-bit ASLR high entropy
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_link_options(/HIGHENTROPYVA)
        message(STATUS "[SecurityHardening] MSVC: /HIGHENTROPYVA (64-bit high-entropy ASLR)")
    endif()

endif()

message(STATUS "[SecurityHardening] Hardening configuration complete.")
