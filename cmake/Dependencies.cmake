# ThemisDB External Dependencies Management

# vcpkg Integration
if(DEFINED VCPKG_ROOT_DIR)
    set(_the_vcpkg_root "${VCPKG_ROOT_DIR}")
elseif(DEFINED ENV{VCPKG_ROOT})
    set(_the_vcpkg_root "$ENV{VCPKG_ROOT}")
else()
    set(_the_vcpkg_root "${CMAKE_SOURCE_DIR}/vcpkg")
endif()

if(EXISTS "${_the_vcpkg_root}")
    set(CMAKE_TOOLCHAIN_FILE "${_the_vcpkg_root}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    if(WIN32)
        file(GLOB _vcpkg_packages "${_the_vcpkg_root}/packages/*_x64-windows")
    else()
        file(GLOB _vcpkg_packages "${_the_vcpkg_root}/packages/*_x64-linux")
    endif()
    message(STATUS "vcpkg root: ${_the_vcpkg_root}")
    message(STATUS "vcpkg package dirs: ${_vcpkg_packages}")
    foreach(_pkg_dir ${_vcpkg_packages})
        list(APPEND CMAKE_PREFIX_PATH 
            "${_pkg_dir}/lib/cmake"
            "${_pkg_dir}/share"
            "${_pkg_dir}/lib"
        )
        list(APPEND CMAKE_LIBRARY_PATH "${_pkg_dir}/lib")
        list(APPEND CMAKE_INCLUDE_PATH "${_pkg_dir}/include")
    endforeach()

    # Pre-seed ZLIB variables from vcpkg package layout if present
    set(_vcpkg_zlib_pkg "${_the_vcpkg_root}/packages/zlib_x64-windows")
    if(EXISTS "${_vcpkg_zlib_pkg}")
        set(_zlib_lib_path "${_vcpkg_zlib_pkg}/lib/zlib.lib")
        set(_zlib_inc_path "${_vcpkg_zlib_pkg}/include")
        if(EXISTS "${_zlib_lib_path}")
            set(ZLIB_LIBRARY "${_zlib_lib_path}" CACHE FILEPATH "zlib library (preseeded from vcpkg)")
        endif()
        if(EXISTS "${_zlib_inc_path}")
            set(ZLIB_INCLUDE_DIR "${_zlib_inc_path}" CACHE PATH "zlib include dir (preseeded from vcpkg)")
        endif()
    endif()
else()
    message(STATUS "vcpkg root not found at ${_the_vcpkg_root}; skipping vcpkg package prefix setup")
endif()

# Prefer CONFIG packages (vcpkg) over FindXXX modules
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# ============================================================================
# RELEASE BUILD MODE DETECTION AND VALIDATION
# ============================================================================

# Automatically detect release mode based on CMAKE_BUILD_TYPE
if(CMAKE_BUILD_TYPE MATCHES "^(Release|RelWithDebInfo|MinSizeRel)$")
    set(_themis_default_release_build ON)
else()
    set(_themis_default_release_build OFF)
endif()

option(THEMIS_RELEASE_BUILD
    "Enforce strict dependency checking for production/release builds. Automatically ON for Release/RelWithDebInfo/MinSizeRel builds."
    ${_themis_default_release_build}
)

# Validate incompatible flag combinations for release builds
if(THEMIS_RELEASE_BUILD AND THEMIS_ALLOW_MISSING_ROCKSDB)
    message(FATAL_ERROR
        "INVALID CONFIGURATION: THEMIS_RELEASE_BUILD=ON and THEMIS_ALLOW_MISSING_ROCKSDB=ON are mutually exclusive. "
        "Release builds cannot tolerate missing dependencies. Use THEMIS_ALLOW_MISSING_ROCKSDB=ON only in diagnostic/development builds with CMAKE_BUILD_TYPE=Debug."
    )
endif()

if(THEMIS_RELEASE_BUILD)
    message(STATUS "RELEASE BUILD MODE: Enforcing strict dependency checking. All required dependencies must be available.")
endif()

# ============================================================================
# REQUIRED DEPENDENCIES (core functionality)
# ============================================================================

# First try with CONFIG package  
find_package(OpenSSL CONFIG QUIET)
if(NOT OpenSSL_FOUND)
    # Try MODULE search
    find_package(OpenSSL MODULE QUIET)
endif()

# If still not found, skip OpenSSL (not all features require it)
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
else()
    message(WARNING "OpenSSL not found - some features may be disabled")
endif()

message(STATUS "Preseed ZLIB_LIBRARY=${ZLIB_LIBRARY} ZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIR}")
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    find_package(ZLIB REQUIRED)
endif()
message(STATUS "ZLIB found: ${ZLIB_VERSION}")

# Create ZLIB::ZLIB target if it doesn't exist (needed for RocksDB and CURL compatibility)
if(ZLIB_FOUND AND NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB INTERFACE IMPORTED)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${ZLIB_LIBRARY}"
    )
    message(STATUS "Created ZLIB::ZLIB imported target")
endif()

# zstd (compression codec) - must be found before RocksDB
find_package(zstd QUIET CONFIG)
if(zstd_FOUND)
    message(STATUS "zstd found - enabling Zstandard compression")
    # Create zstd::zstd alias for RocksDB compatibility
    if(TARGET zstd::libzstd_shared AND NOT TARGET zstd::zstd)
        add_library(zstd::zstd ALIAS zstd::libzstd_shared)
    elseif(TARGET zstd::libzstd_static AND NOT TARGET zstd::zstd)
        add_library(zstd::zstd ALIAS zstd::libzstd_static)
    endif()
else()
    # Try pkg-config as fallback
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(zstd QUIET libzstd)
        if(zstd_FOUND)
            message(STATUS "zstd found via pkg-config")
            # Create imported target for compatibility
            add_library(zstd::zstd INTERFACE IMPORTED)
            set_target_properties(zstd::zstd PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${zstd_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${zstd_LIBRARIES}"
            )
        else()
            message(STATUS "zstd not found - using fallback compression")
        endif()
    else()
        message(STATUS "zstd not found - using fallback compression")
    endif()
endif()
# RocksDB: Prefer CONFIG (vcpkg) and fallback to unofficial target if provided by vcpkg
option(THEMIS_ALLOW_MISSING_ROCKSDB
    "Allow CMake configure to continue without a detected RocksDB install (build of RocksDB-dependent targets may fail)."
    OFF
)
find_package(RocksDB CONFIG QUIET)
if(RocksDB_FOUND)
   message(STATUS "RocksDB found via CONFIG")
   # CHECK if the CONFIG version is static and override it with the dynamic library if available
   find_library(_ROCKSDB_DYNAMIC NAMES librocksdb.so rocksdb.so)
   if(_ROCKSDB_DYNAMIC)
       message(STATUS "RocksDB CONFIG found, checking for dynamic library: ${_ROCKSDB_DYNAMIC}")
       # Modify the existing target to use the dynamic library instead
       if(TARGET RocksDB::rocksdb)
           # Update the target to point to the shared library
           set_target_properties(RocksDB::rocksdb PROPERTIES
               IMPORTED_LOCATION "${_ROCKSDB_DYNAMIC}"
               IMPORTED_NO_SONAME_RELEASE FALSE
           )
           message(STATUS "Updated RocksDB to use dynamic library: ${_ROCKSDB_DYNAMIC}")
       endif()
   endif()
else()
   # vcpkg often provides 'unofficial-rocksdb' with target 'unofficial::rocksdb'
   find_package(unofficial-rocksdb CONFIG QUIET)
   if(unofficial-rocksdb_FOUND)
       add_library(RocksDB::rocksdb ALIAS unofficial::rocksdb)
       message(STATUS "RocksDB found via vcpkg (unofficial)")
   else()
       # Try pkg-config as fallback (covers system-installed librocksdb-dev)
       find_package(PkgConfig QUIET)
       if(PkgConfig_FOUND)
           pkg_check_modules(RocksDB_PC QUIET rocksdb)
       endif()
       if(RocksDB_PC_FOUND)
           # Check if dynamic library exists
           find_library(_ROCKSDB_DYNAMIC NAMES librocksdb.so rocksdb.so PATHS ${RocksDB_PC_LIBRARY_DIRS} NO_DEFAULT_PATH)
            
           if(_ROCKSDB_DYNAMIC)
               # Use dynamic library explicitly
               message(STATUS "RocksDB found as dynamic library: ${_ROCKSDB_DYNAMIC}")
               add_library(RocksDB::rocksdb SHARED IMPORTED)
               set_target_properties(RocksDB::rocksdb PROPERTIES
                   IMPORTED_LOCATION "${_ROCKSDB_DYNAMIC}"
                   INTERFACE_INCLUDE_DIRECTORIES "${RocksDB_PC_INCLUDE_DIRS}"
               )
           else()
               # Try to find it manually if pkg-config didn't find the dynamic version
               find_library(_ROCKSDB_FALLBACK NAMES librocksdb.so rocksdb.so)
               if(_ROCKSDB_FALLBACK)
                   message(STATUS "RocksDB found at: ${_ROCKSDB_FALLBACK}")
                   add_library(RocksDB::rocksdb SHARED IMPORTED)
                   set_target_properties(RocksDB::rocksdb PROPERTIES
                       IMPORTED_LOCATION "${_ROCKSDB_FALLBACK}"
                       INTERFACE_INCLUDE_DIRECTORIES "${RocksDB_PC_INCLUDE_DIRS}"
                   )
               else()
                   # Fall back to interface library with pkg-config (for static linking if shared not available)
                   message(STATUS "RocksDB found via pkg-config: ${RocksDB_PC_LIBRARIES}")
                   add_library(RocksDB::rocksdb INTERFACE IMPORTED)
                   set_target_properties(RocksDB::rocksdb PROPERTIES
                       INTERFACE_INCLUDE_DIRECTORIES "${RocksDB_PC_INCLUDE_DIRS}"
                       INTERFACE_LINK_LIBRARIES     "${RocksDB_PC_LIBRARIES}"
                       INTERFACE_LINK_DIRECTORIES   "${RocksDB_PC_LIBRARY_DIRS}"
                   )
               endif()
           endif()
           set(RocksDB_FOUND TRUE)
           message(STATUS "RocksDB found via pkg-config: ${RocksDB_PC_VERSION}")
       else()
           if(THEMIS_ALLOW_MISSING_ROCKSDB)
               message(WARNING
                   "RocksDB not found. Continuing configure because THEMIS_ALLOW_MISSING_ROCKSDB=ON. "
                   "Install via vcpkg (rocksdb) or system package librocksdb-dev before building "
                   "RocksDB-dependent targets."
               )
               if(NOT TARGET RocksDB::rocksdb)
                   add_library(RocksDB::rocksdb INTERFACE IMPORTED)
               endif()
               set(RocksDB_FOUND FALSE)
           else()
               message(FATAL_ERROR "RocksDB not found. Install via vcpkg (rocksdb) or system package librocksdb-dev.")
           endif()
       endif()
   endif()
endif()

# Define THEMIS_ROCKSDB_AVAILABLE for conditional compilation when RocksDB is available
if(RocksDB_FOUND)
    add_compile_definitions(THEMIS_ROCKSDB_AVAILABLE)
endif()

find_package(simdjson CONFIG)
if(simdjson_FOUND)
    message(STATUS "simdjson found")
else()
    message(WARNING "simdjson not found - some features may be disabled")
endif()

find_package(TBB CONFIG)
if(TBB_FOUND)
    message(STATUS "TBB found")
else()
    message(WARNING "TBB not found - using fallback threading")
endif()

# fmt: Try CONFIG first (vcpkg), fall back to MODULE (system packages)
find_package(fmt CONFIG QUIET)
if(NOT fmt_FOUND)
    find_package(fmt MODULE QUIET)
endif()
if(fmt_FOUND)
    message(STATUS "fmt found")
else()
    # fmt is a CRITICAL dependency - ALWAYS fail if not found
    message(FATAL_ERROR 
        "fmt library not found. This is a critical dependency and cannot be skipped. Install via:\n"
        "  - vcpkg: vcpkg install fmt\n"
        "  - Debian/Ubuntu: sudo apt-get install libfmt-dev\n"
        "  - Fedora/RHEL: sudo dnf install fmt-devel\n"
        "  - macOS: brew install fmt"
    )
endif()

# spdlog: Try CONFIG first (vcpkg), fall back to MODULE (system packages)
find_package(spdlog CONFIG QUIET)
if(NOT spdlog_FOUND)
    find_package(spdlog MODULE QUIET)
endif()
if(spdlog_FOUND)
    message(STATUS "spdlog found")
else()
    # spdlog is a CRITICAL dependency - ALWAYS fail if not found
    message(FATAL_ERROR 
        "spdlog library not found. This is a critical dependency and cannot be skipped. Install via:\n"
        "  - vcpkg: vcpkg install spdlog\n"
        "  - Debian/Ubuntu: sudo apt-get install libspdlog-dev\n"
        "  - Fedora/RHEL: sudo dnf install spdlog-devel\n"
        "  - macOS: brew install spdlog"
    )
endif()

# Disable spdlog compile-time format string checks for better compatibility with runtime format strings
if(spdlog_FOUND AND NOT MSVC)
    add_compile_definitions(SPDLOG_USE_SPDLOG_FMT_EXT=0)
    add_compile_definitions(SPDLOG_DISABLE_DEFAULT_LOGGER)
    add_compile_definitions(SPDLOG_NO_EXCEPTIONS)
    add_compile_definitions(SPDLOG_FMT_EXTERNAL)
    # Note: Do not use SPDLOG_USE_STD_FORMAT with fmt namespace - causes incompatibilities
endif()

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    find_package(nlohmann_json MODULE QUIET)
endif()
if(nlohmann_json_FOUND)
    message(STATUS "nlohmann_json found")
else()
    # nlohmann_json is a CRITICAL dependency - ALWAYS fail if not found
    message(FATAL_ERROR 
        "nlohmann_json library not found. This is a critical dependency and cannot be skipped. Install via:\n"
        "  - vcpkg: vcpkg install nlohmann-json\n"
        "  - Debian/Ubuntu: sudo apt-get install nlohmann-json3-dev\n"
        "  - Fedora/RHEL: sudo dnf install nlohmann-json-devel\n"
        "  - macOS: brew install nlohmann-json"
    )
endif()

# Boost: Try CONFIG first, fall back to MODULE if not found
find_package(Boost 1.70 CONFIG COMPONENTS system filesystem QUIET)
if(NOT Boost_FOUND)
    find_package(Boost 1.70 MODULE QUIET COMPONENTS system filesystem)
endif()
if(Boost_FOUND)
    message(STATUS "Boost found: ${Boost_VERSION}")
else()
    message(WARNING "Boost not found - some features may be disabled")
endif()

find_package(Threads REQUIRED)
message(STATUS "Threads found")

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found")
else()
    message(WARNING "OpenMP not found - parallel features will be disabled")
endif()

# Protobuf (required for gRPC and general serialization)
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
    find_package(Protobuf QUIET)
endif()
if(Protobuf_FOUND)
    message(STATUS "Protobuf found: ${Protobuf_VERSION}")
else()
    message(WARNING "Protobuf not found - gRPC features will be disabled")
endif()

# gRPC (inter-shard communication)
# Priority: CONFIG, then pkg-config, then fallback
if(THEMIS_ENABLE_GRPC)
    find_package(gRPC QUIET CONFIG)
    if(NOT gRPC_FOUND)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(gRPC QUIET grpc++ grpc)
            if(gRPC_FOUND)
                message(STATUS "gRPC found via pkg-config")
            endif()
        endif()
    endif()
    
    if(NOT gRPC_FOUND)
        message(WARNING "gRPC not found - gRPC features will be disabled. Install grpc-devel or configure VCPKG_ROOT")
        set(THEMIS_ENABLE_GRPC OFF CACHE BOOL "Disabled due to missing gRPC" FORCE)
    else()
        message(STATUS "gRPC found")
    endif()
else()
    message(STATUS "gRPC support disabled (THEMIS_ENABLE_GRPC=OFF)")
endif()

# GTest (unit testing framework - required for tests)
if(THEMIS_BUILD_TESTS)
    find_package(GTest QUIET CONFIG)
    if(GTest_FOUND)
        message(STATUS "GTest found - tests enabled")
        add_compile_definitions(THEMIS_HAS_GTEST=1)
    else()
        message(WARNING "GTest not found - tests will not be built")
        message(WARNING "Install with: vcpkg install gtest OR apt-get install libgtest-dev")
        set(THEMIS_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
    endif()
else()
    message(STATUS "Tests disabled (THEMIS_BUILD_TESTS=OFF)")
endif()

# Google Benchmark (performance testing - required for benchmarks)
if(THEMIS_BUILD_BENCHMARKS)
    find_package(benchmark QUIET CONFIG)
    if(benchmark_FOUND)
        message(STATUS "Google Benchmark found - benchmarks enabled")
        add_compile_definitions(THEMIS_HAS_BENCHMARK=1)
    else()
        message(WARNING "Google Benchmark not found - Google-Benchmark-dependent benchmarks will not be built")
        message(WARNING "Install with: vcpkg install benchmark OR apt-get install libbenchmark-dev")
        set(THEMIS_HAS_BENCHMARK OFF)
    endif()
else()
    message(STATUS "Benchmarks disabled (THEMIS_BUILD_BENCHMARKS=OFF)")
endif()

# Prometheus C++ Client (metrics - optional for LoRA framework)
find_package(prometheus-cpp QUIET CONFIG)
if(prometheus-cpp_FOUND)
    message(STATUS "Prometheus C++ client found - metrics enabled")
    add_compile_definitions(THEMIS_HAS_PROMETHEUS=1)
else()
    message(STATUS "Prometheus C++ client not found - metrics collection disabled")
    message(STATUS "Install with: vcpkg install prometheus-cpp (optional)")
    # Provide optional fallback targets so optional test/link declarations do not
    # fail configure when prometheus-cpp is not installed on CI runners.
    foreach(_prom_tgt core pull push util)
        if(NOT TARGET prometheus-cpp::${_prom_tgt})
            add_library(prometheus-cpp::${_prom_tgt} INTERFACE IMPORTED GLOBAL)
        endif()
    endforeach()
endif()

# ============================================================================
# OPTIONAL DEPENDENCIES (features)
# ============================================================================

# CURL (HTTP client, optional - some features disabled if missing)
# When found, defines THEMIS_HAS_CURL=1 globally.
# Consumer: src/process/fim_importer.cpp — FimImporter::makeCurlHttpFetchFn()
#           returns a real libcurl-backed HttpFetchFn when this flag is set.
#           Call importer.setHttpFetchFn(FimImporter::makeCurlHttpFetchFn()) at startup.
find_package(CURL QUIET CONFIG)
if(NOT CURL_FOUND)
    find_package(CURL QUIET)
endif()

if(CURL_FOUND)
    message(STATUS "CURL found - enabling HTTP client features")
    add_compile_definitions(THEMIS_HAS_CURL=1)
else()
    message(WARNING "CURL not found - some HTTP features will be disabled")
endif()

# cpp-httplib (built-in HTTP server)
find_package(httplib QUIET CONFIG)
if(httplib_FOUND)
    message(STATUS "cpp-httplib found - enabling built-in HTTP server")
    add_compile_definitions(THEMIS_HAS_HTTPLIB=1)
else()
    message(WARNING "cpp-httplib not found - built-in HTTP server features may be limited")
endif()

# MessagePack (binary buffer protocol)
# Try msgpack (primary) then msgpack-cxx (alternate package name)
find_package(msgpack QUIET CONFIG)
if(NOT msgpack_FOUND)
    find_package(msgpack-cxx QUIET CONFIG)
endif()
if(msgpack_FOUND OR msgpack-cxx_FOUND)
    message(STATUS "MessagePack found - enabling binary buffer protocol")
    add_compile_definitions(THEMIS_HAS_MSGPACK=1)
else()
    message(WARNING "MessagePack not found - binary buffer protocol disabled")
endif()

# Kerberos/GSSAPI (enterprise SSO authentication - optional)
# Kerberos/GSSAPI - PERMANENTLY DISABLED on Windows
# Kerberos is not available on Windows and causes build issues.
# For Windows deployments, use alternative authentication (LDAP, OAuth2, SAML, etc.)
option(THEMIS_ENABLE_KERBEROS "Enable Kerberos/GSSAPI authentication support if available" ON)
if(THEMIS_ENABLE_KERBEROS AND NOT WIN32)  # Kerberos not supported on Windows
    # Try to find Kerberos using pkg-config first (most reliable on Unix)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(KRB5 QUIET krb5 krb5-gssapi)
        if(KRB5_FOUND)
            message(STATUS "Kerberos found via pkg-config")
            add_compile_definitions(THEMIS_HAS_KERBEROS=1)
            
            # Create imported target for compatibility
            if(NOT TARGET KRB5::krb5)
                add_library(KRB5::krb5 INTERFACE IMPORTED)
                set_target_properties(KRB5::krb5 PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${KRB5_INCLUDE_DIRS}"
                    INTERFACE_LINK_LIBRARIES "${KRB5_LIBRARIES}"
                )
            endif()
            
            if(NOT TARGET KRB5::gssapi)
                add_library(KRB5::gssapi INTERFACE IMPORTED)
                set_target_properties(KRB5::gssapi PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${KRB5_INCLUDE_DIRS}"
                    INTERFACE_LINK_LIBRARIES "${KRB5_LIBRARIES}"
                )
            endif()
        endif()
    endif()
    
    # If pkg-config didn't work, try FindKerberos module
    if(NOT KRB5_FOUND)
        find_package(Kerberos QUIET)
        if(Kerberos_FOUND)
            message(STATUS "Kerberos found via FindKerberos")
            add_compile_definitions(THEMIS_HAS_KERBEROS=1)
            
            # Create aliases for consistency
            if(NOT TARGET KRB5::krb5)
                add_library(KRB5::krb5 ALIAS Kerberos::Kerberos)
            endif()
            if(NOT TARGET KRB5::gssapi)
                add_library(KRB5::gssapi ALIAS Kerberos::Kerberos)
            endif()
        endif()
    endif()
    
    if(NOT KRB5_FOUND AND NOT Kerberos_FOUND)
        message(WARNING "Kerberos not found - enterprise SSO authentication disabled")
        message(STATUS "Install with: apt-get install libkrb5-dev (Ubuntu/Debian)")
        message(STATUS "            : yum install krb5-devel (RHEL/CentOS)")
        message(STATUS "            : brew install krb5 (macOS)")
        set(THEMIS_ENABLE_KERBEROS OFF)
    else()
        # Kerberos was found - already ON from option()
        message(STATUS "Kerberos/GSSAPI authentication enabled")
    endif()
else()
    message(STATUS "Kerberos support disabled (THEMIS_ENABLE_KERBEROS=OFF or Windows platform)")
endif()

# LDAP / Active Directory direct-bind authentication (optional)
option(THEMIS_ENABLE_LDAP "Enable LDAP/Active Directory direct-bind authentication" ON)
if(THEMIS_ENABLE_LDAP)
    if(WIN32)
        # WinLDAP (wldap32) is part of the Windows SDK — always available
        message(STATUS "LDAP support: using built-in WinLDAP (wldap32)")
        add_compile_definitions(THEMIS_HAS_LDAP=1)
        set(THEMIS_LDAP_LIBRARIES wldap32)
        set(THEMIS_LDAP_FOUND TRUE)
    else()
        # Unix: try pkg-config first, then a plain find
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(LDAP QUIET ldap lber)
        endif()
        if(LDAP_FOUND)
            message(STATUS "OpenLDAP found via pkg-config")
            add_compile_definitions(THEMIS_HAS_LDAP=1)
            set(THEMIS_LDAP_LIBRARIES ${LDAP_LIBRARIES})
            set(THEMIS_LDAP_INCLUDE_DIRS ${LDAP_INCLUDE_DIRS})
            set(THEMIS_LDAP_FOUND TRUE)
        else()
            find_library(LDAP_LIB     NAMES ldap  DOC "OpenLDAP library")
            find_library(LBER_LIB     NAMES lber  DOC "OpenLDAP BER library")
            find_path(LDAP_INCLUDE_DIR NAMES ldap.h
                HINTS /usr/include /usr/local/include /usr/include/openldap)
            if(LDAP_LIB AND LDAP_INCLUDE_DIR)
                message(STATUS "OpenLDAP found: ${LDAP_LIB}")
                add_compile_definitions(THEMIS_HAS_LDAP=1)
                set(THEMIS_LDAP_LIBRARIES ${LDAP_LIB})
                if(LBER_LIB)
                    list(APPEND THEMIS_LDAP_LIBRARIES ${LBER_LIB})
                endif()
                set(THEMIS_LDAP_INCLUDE_DIRS ${LDAP_INCLUDE_DIR})
                set(THEMIS_LDAP_FOUND TRUE)
            else()
                message(WARNING
                    "OpenLDAP not found - LDAP/AD direct-bind authentication will be "
                    "compiled as a no-op stub.  "
                    "Install with: apt-get install libldap-dev (Debian/Ubuntu) "
                    "or yum install openldap-devel (RHEL/CentOS)")
                set(THEMIS_LDAP_FOUND FALSE)
            endif()
        endif()
    endif()
else()
    message(STATUS "LDAP support disabled (THEMIS_ENABLE_LDAP=OFF)")
    set(THEMIS_LDAP_FOUND FALSE)
endif()

# ONNX Runtime (ML model inference)
# Guard repeated CONFIG loads: onnxruntimeConfig.cmake defines
# safeint_interface without existence checks.
if(NOT onnxruntime_FOUND AND NOT TARGET onnxruntime::onnxruntime AND NOT TARGET safeint_interface)
    find_package(onnxruntime QUIET CONFIG)
endif()
if(onnxruntime_FOUND OR TARGET onnxruntime::onnxruntime)
    message(STATUS "ONNX Runtime found - enabling ONNX model serving backend")
    add_compile_definitions(THEMIS_HAS_ONNX=1)
else()
    message(STATUS "ONNX Runtime not found - ONNX serving backend disabled "
                   "(install via vcpkg: onnxruntime)")
endif()

# TensorFlow Serving REST client (requires libcurl)
# Enable explicitly with -DTHEMIS_ENABLE_TF_SERVING=ON
option(THEMIS_ENABLE_TF_SERVING "Enable TensorFlow Serving REST API backend" OFF)
if(THEMIS_ENABLE_TF_SERVING)
    if(CURL_FOUND)
        message(STATUS "TF Serving backend enabled (libcurl available)")
        add_compile_definitions(THEMIS_HAS_TF_SERVING=1)
    else()
        message(WARNING "TF Serving requested but libcurl not found - backend disabled")
    endif()
endif()

# Arrow + Parquet (Parquet export support)
find_package(Arrow QUIET CONFIG)
find_package(Parquet QUIET CONFIG)
find_package(ArrowFlight QUIET CONFIG)

if(Arrow_FOUND)
    message(STATUS "Arrow found - enabling Parquet export")
    add_compile_definitions(THEMIS_HAS_ARROW=1)
    if(Parquet_FOUND)
        add_compile_definitions(THEMIS_HAS_PARQUET=1)
    endif()
    if(ArrowFlight_FOUND OR TARGET Arrow::arrow_flight_shared OR TARGET Arrow::arrow_flight_static)
        message(STATUS "Arrow Flight found - enabling native Arrow Flight RPC transport")
        add_compile_definitions(THEMIS_HAS_ARROW_FLIGHT=1)
    else()
        message(STATUS "Arrow Flight not found - using in-process transport only")
    endif()
else()
    message(STATUS "Arrow not found - Parquet export and native Arrow Flight disabled")
endif()

# YAML (configuration parsing)
find_package(yaml-cpp QUIET CONFIG)
if(yaml-cpp_FOUND)
    message(STATUS "yaml-cpp found")
else()
    message(WARNING "yaml-cpp not found - configuration features may be limited")
endif()

# (zstd is handled earlier, before RocksDB)

# FFmpeg (video processing and audio conversion - optional for content/voice plugins)
# When found, defines THEMIS_HAS_FFMPEG=1 globally.
# Consumer: src/voice/voice_assistant.cpp — VoiceAssistant::makeFFmpegAudioConvertFn()
#           returns a real libavformat/libavcodec AudioConvertFn when this flag is set.
#           Call assistant.setAudioConvertFn(VoiceAssistant::makeFFmpegAudioConvertFn()) at startup.
find_package(PkgConfig QUIET)
set(FFMPEG_FOUND FALSE)

if(PkgConfig_FOUND)
    pkg_check_modules(FFMPEG QUIET 
        libavformat 
        libavcodec 
        libswscale 
        libavutil
    )
endif()

# Fallback for vcpkg/Windows: Check if FFmpeg libraries exist
if(NOT FFMPEG_FOUND)
    find_library(AVFORMAT_LIB NAMES avformat HINTS ${CMAKE_PREFIX_PATH}/lib)
    find_library(AVCODEC_LIB NAMES avcodec HINTS ${CMAKE_PREFIX_PATH}/lib)
    find_library(SWSCALE_LIB NAMES swscale HINTS ${CMAKE_PREFIX_PATH}/lib)
    find_library(AVUTIL_LIB NAMES avutil HINTS ${CMAKE_PREFIX_PATH}/lib)
    
    if(AVFORMAT_LIB AND AVCODEC_LIB AND SWSCALE_LIB AND AVUTIL_LIB)
        set(FFMPEG_FOUND TRUE)
        message(STATUS "FFmpeg found via vcpkg - enabling real video processing")
        add_compile_definitions(THEMIS_HAS_FFMPEG=1)
    endif()
endif()

if(FFMPEG_FOUND)
    # Create imported targets for FFmpeg libraries
    if(NOT TARGET FFmpeg::avformat)
        add_library(FFmpeg::avformat INTERFACE IMPORTED)
        if(PkgConfig_FOUND AND FFMPEG_INCLUDE_DIRS)
            set_target_properties(FFmpeg::avformat PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}"
            )
        else()
            # vcpkg/Windows: Use direct library paths
            set_target_properties(FFmpeg::avformat PROPERTIES
                INTERFACE_LINK_LIBRARIES "${AVFORMAT_LIB};${AVCODEC_LIB};${SWSCALE_LIB};${AVUTIL_LIB}"
            )
        endif()
    endif()
else()
    message(STATUS "FFmpeg not found - video processor will use simulation mode")
    if(WIN32)
        message(STATUS "Install with: vcpkg install ffmpeg:x64-windows")
    else()
        message(STATUS "Install with: apt-get install libavformat-dev libavcodec-dev libswscale-dev libavutil-dev")
    endif()
endif()


# HNSW library (vector indexing)
find_package(hnswlib QUIET CONFIG)
if(hnswlib_FOUND AND NOT THEMIS_ENABLE_GPU)
    message(STATUS "hnswlib found - enabling HNSW vector search")
else()
    if(THEMIS_ENABLE_GPU)
        message(STATUS "GPU enabled - using GPU vector search, HNSW optional")
    else()
        message(STATUS "hnswlib not found - using fallback vector search")
    endif()
endif()

# mimalloc (fast memory allocator, optional)
if(THEMIS_ENABLE_MIMALLOC)
    find_package(mimalloc QUIET CONFIG)
    if(mimalloc_FOUND)
        message(STATUS "mimalloc found - enabling high-performance memory allocation")
        add_compile_definitions(THEMIS_HAS_MIMALLOC=1)
    else()
        # If explicitly enabled, it should always be available
        message(FATAL_ERROR "THEMIS_ENABLE_MIMALLOC=ON but mimalloc not found. Install via: vcpkg install mimalloc OR apt install libmimalloc-dev")
    endif()
endif()

# jemalloc (alternative memory allocator, Linux/Mac only, best fragmentation resistance)
if(THEMIS_ENABLE_JEMALLOC)
    if(NOT WIN32)
        find_package(jemalloc QUIET CONFIG)
        if(NOT jemalloc_FOUND)
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(jemalloc QUIET jemalloc)
            endif()
        endif()
        if(jemalloc_FOUND OR jemalloc_LIBRARIES)
            message(STATUS "jemalloc found - enabling jemalloc allocator")
            add_compile_definitions(THEMIS_HAS_JEMALLOC=1)
        else()
            message(FATAL_ERROR "THEMIS_ENABLE_JEMALLOC=ON but jemalloc not found. Install: vcpkg install jemalloc OR apt install libjemalloc-dev")
        endif()
    endif()
endif()

# OpenTelemetry (distributed tracing and observability)
if(THEMIS_ENABLE_TRACING)
    find_package(opentelemetry-cpp REQUIRED CONFIG)
    message(STATUS "OpenTelemetry-cpp found - enabling distributed tracing")
    add_compile_definitions(THEMIS_ENABLE_TRACING=1)
endif()

# ============================================================================
# PROTOCOL-SPECIFIC DEPENDENCIES
# ============================================================================

# HTTP/2 support
if(THEMIS_ENABLE_HTTP2)
    find_package(nghttp2 QUIET)
    if(nghttp2_FOUND)
        message(STATUS "nghttp2 found - enabling HTTP/2 support")
        add_compile_definitions(THEMIS_ENABLE_HTTP2=1)
    else()
        message(FATAL_ERROR "THEMIS_ENABLE_HTTP2=ON but nghttp2 not found")
    endif()
endif()

# HTTP/3 support
if(THEMIS_ENABLE_HTTP3)
    find_package(nghttp3 QUIET)
    find_package(ngtcp2 QUIET)
    if(nghttp3_FOUND AND ngtcp2_FOUND)
        message(STATUS "nghttp3 + ngtcp2 found - enabling HTTP/3 support")
        add_compile_definitions(THEMIS_ENABLE_HTTP3=1)
    else()
        message(FATAL_ERROR "THEMIS_ENABLE_HTTP3=ON but nghttp3 or ngtcp2 not found")
    endif()
endif()

# ============================================================================
# Redis – distributed cache coordination (optional, via hiredis)
# ============================================================================
option(THEMIS_ENABLE_REDIS "Enable Redis-compatible distributed cache coordination (hiredis)" OFF)
if(THEMIS_ENABLE_REDIS)
    find_package(hiredis CONFIG QUIET)
    if(hiredis_FOUND)
        message(STATUS "hiredis found – enabling Redis distributed cache coordination")
        add_compile_definitions(THEMIS_ENABLE_REDIS=1)
    else()
        message(WARNING "THEMIS_ENABLE_REDIS=ON but hiredis not found – Redis transport disabled")
        set(THEMIS_ENABLE_REDIS OFF CACHE BOOL "Disabled: hiredis not found" FORCE)
    endif()
else()
    message(STATUS "Redis distributed cache coordination disabled (THEMIS_ENABLE_REDIS=OFF)")
endif()

# ============================================================================
# HARDWARE ACCELERATION DEPENDENCIES
# ============================================================================

# CUDA (GPU acceleration)
# GPU acceleration support (FAISS for vector search)
if(THEMIS_ENABLE_GPU)
    # FAISS is always required for GPU vector search (CPU or CUDA backend)
    find_package(faiss QUIET)
    if(faiss_FOUND)
        message(STATUS "FAISS found - enabling GPU-accelerated vector search")
        add_compile_definitions(THEMIS_HAS_FAISS=1)
    else()
        message(WARNING "FAISS not found - GPU vector search will be limited")
    endif()
endif()

if(THEMIS_ENABLE_CUDA)
    find_package(CUDA QUIET)
    find_package(CUDAToolkit QUIET)
    if(CUDA_FOUND AND CUDAToolkit_FOUND)
        message(STATUS "CUDA Toolkit found: ${CUDAToolkit_VERSION}")
        add_compile_definitions(THEMIS_ENABLE_CUDA=1)
    else()
        message(WARNING "THEMIS_ENABLE_CUDA=ON but CUDA toolkit components were not found. Disabling CUDA backend.")
        set(THEMIS_ENABLE_CUDA OFF CACHE BOOL "CUDA disabled: toolkit not found" FORCE)
    endif()
endif()

if(THEMIS_ENABLE_CUDA)
    
    # Optional: FAISS for GPU-accelerated vector search
    find_package(faiss QUIET)
    if(faiss_FOUND)
        message(STATUS "FAISS found - enabling GPU vector search")
        add_compile_definitions(THEMIS_HAS_FAISS=1)
    else()
        message(STATUS "FAISS not found - using CuBLAS for vector operations")
    endif()

    if(THEMIS_ENABLE_CUVS)
        find_package(raft QUIET)
        if(raft_FOUND)
            message(STATUS "RAFT found - enabling CUDA/cuVS ANN dispatch path")
            add_compile_definitions(THEMIS_ENABLE_CUVS=1)
        else()
            message(WARNING "THEMIS_ENABLE_CUVS=ON but RAFT was not found. Disabling cuVS gate and keeping CPU fallback.")
            set(THEMIS_ENABLE_CUVS OFF CACHE BOOL "cuVS gate disabled: RAFT not found" FORCE)
        endif()
    endif()
    
    # Optional: NCCL for multi-GPU communication (v2.5+)
    if(THEMIS_ENABLE_NCCL)
        find_library(NCCL_LIBRARIES nccl)
        find_path(NCCL_INCLUDE_DIRS nccl.h)
        if(NCCL_LIBRARIES AND NCCL_INCLUDE_DIRS)
            message(STATUS "NCCL found - enabling multi-GPU vector indexing")
            add_compile_definitions(THEMIS_ENABLE_NCCL=1)
            # Export for use in targets
            set(NCCL_FOUND TRUE)
        else()
            message(WARNING "NCCL not found - multi-GPU features will be limited")
            set(THEMIS_ENABLE_NCCL OFF CACHE BOOL "NCCL not available" FORCE)
            set(NCCL_FOUND FALSE)
        endif()
    endif()
endif()

# HIP (AMD GPU acceleration) - optional alternative to CUDA
if(THEMIS_ENABLE_HIP)
    find_package(HIP QUIET)
    if(HIP_FOUND)
        message(STATUS "HIP found - enabling AMD GPU support")
        add_compile_definitions(THEMIS_ENABLE_HIP=1)
    else()
        message(WARNING "THEMIS_ENABLE_HIP=ON but HIP/ROCm was not found. Disabling HIP backend.")
        set(THEMIS_ENABLE_HIP OFF CACHE BOOL "HIP disabled: toolkit not found" FORCE)
    endif()
endif()

if(THEMIS_ENABLE_HIP)
    
    # Optional: RCCL for multi-GPU communication on AMD (v2.5+)
    if(THEMIS_ENABLE_RCCL)
        find_library(RCCL_LIBRARIES rccl PATHS /opt/rocm/lib)
        find_path(RCCL_INCLUDE_DIRS rccl/rccl.h PATHS /opt/rocm/include)
        if(RCCL_LIBRARIES AND RCCL_INCLUDE_DIRS)
            message(STATUS "RCCL found - enabling multi-GPU vector indexing (AMD)")
            add_compile_definitions(THEMIS_ENABLE_RCCL=1)
            # Export for use in targets
            set(RCCL_FOUND TRUE)
        else()
            message(WARNING "RCCL not found - multi-GPU features will be limited (AMD)")
            set(THEMIS_ENABLE_RCCL OFF CACHE BOOL "RCCL not available" FORCE)
            set(RCCL_FOUND FALSE)
        endif()
    endif()
endif()

if(THEMIS_ENABLE_VULKAN)
    find_package(Vulkan QUIET)
    if(NOT Vulkan_FOUND)
        message(WARNING "THEMIS_ENABLE_VULKAN=ON but Vulkan SDK was not found. Disabling Vulkan backend.")
        set(THEMIS_ENABLE_VULKAN OFF CACHE BOOL "Vulkan disabled: SDK not found" FORCE)
    endif()
endif()

# ============================================================================
# LLM DEPENDENCIES
# ============================================================================

if(THEMIS_ENABLE_LLM)
    # Ensure C language is enabled so OpenMP::OpenMP_C target exists
    enable_language(C)
    # Try to find OpenMP - optional for LLM support (llama.cpp can work without it with reduced parallelism)
    find_package(OpenMP)
    if(OpenMP_FOUND)
        message(STATUS "OpenMP found for LLM support")
    else()
        message(WARNING "OpenMP not found - llama.cpp will use single-threaded inference")
    endif()
    
    # =========================================================================
    # LLAMA.CPP INTEGRATION WITH DEPENDENCY PINNING
    # =========================================================================
    # Use FetchContent for reproducible builds with pinned commit
    # Pinned commit: b7974 (Feb 2026 - stable release with Flash Attention support)
    # To update: Change GIT_TAG to desired commit hash and test thoroughly
    
    include(FetchContent)
    
    set(LLAMA_CPP_GIT_TAG "b7974" CACHE STRING "llama.cpp commit hash for reproducible builds")
    
    message(STATUS "Fetching llama.cpp (pinned commit: ${LLAMA_CPP_GIT_TAG})")
    
    # Configure llama.cpp build options (set before FetchContent)
    set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Build llama tests" FORCE)
    set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Build llama examples" FORCE)
    set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "Build llama tools" FORCE)
    set(LLAMA_BUILD_COMMON OFF CACHE BOOL "Build llama common utils" FORCE)
    set(LLAMA_BUILD_SERVER OFF CACHE BOOL "Build llama server" FORCE)
    set(LLAMA_INSTALL OFF CACHE BOOL "Install llama" FORCE)

    # Keep llama.cpp backend options aligned with Themis GPU toggles.
    # Both LLAMA_* and GGML_* are set for compatibility across llama.cpp revisions.
    if(THEMIS_ENABLE_VULKAN)
        set(LLAMA_VULKAN ON CACHE BOOL "Enable Vulkan backend in llama.cpp" FORCE)
        set(GGML_VULKAN ON CACHE BOOL "Enable Vulkan backend in ggml" FORCE)

        # ggml-vulkan requires glslc at configure time. In local vcpkg setups,
        # the executable is provided by shaderc under installed/tools.
        if(DEFINED VCPKG_HOST_TRIPLET)
            set(_themis_glslc_candidate
                "${PROJECT_SOURCE_DIR}/vcpkg_installed/${VCPKG_HOST_TRIPLET}/tools/shaderc/glslc${CMAKE_EXECUTABLE_SUFFIX}")
            if(EXISTS "${_themis_glslc_candidate}")
                set(Vulkan_GLSLC_EXECUTABLE "${_themis_glslc_candidate}" CACHE FILEPATH
                    "Path to glslc executable for Vulkan shader compilation" FORCE)
                message(STATUS "llama.cpp backend: using glslc from vcpkg shaderc (${Vulkan_GLSLC_EXECUTABLE})")
            endif()
            unset(_themis_glslc_candidate)
        endif()

        message(STATUS "llama.cpp backend: Vulkan ENABLED")
    else()
        set(LLAMA_VULKAN OFF CACHE BOOL "Enable Vulkan backend in llama.cpp" FORCE)
        set(GGML_VULKAN OFF CACHE BOOL "Enable Vulkan backend in ggml" FORCE)
    endif()
    
    # =========================================================================
    # MSVC C++20 char8_t COMPATIBILITY FIX (PR #LLAMA-CPP-MSVC)
    # =========================================================================
    # llama-chat.cpp uses u8"..." literals which MSVC C++20 strictly differentiates
    # from const char*. We need to add compiler flags to handle this.
    if(MSVC)
        # Suppress char8_t strict checking and allow implicit conversions
        set(LLAMA_CXXFLAGS "/permissive- /Zc:char8_t-")
        set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} ${LLAMA_CXXFLAGS}")
        message(STATUS "llama.cpp: Applied MSVC char8_t compatibility flags")
    endif()
    
    # =========================================================================
    # PERFORMANCE OPTIMIZATIONS - PR #1022 CRITICAL FIXES
    # =========================================================================
    # Flash Attention: +15-25% performance improvement
    # Continuous Batching: +8x throughput for parallel requests
    
    if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
        # Enable Flash Attention for Release builds (15-25% performance gain)
        set(LLAMA_FLASH_ATTN ON CACHE BOOL "Enable Flash Attention optimization" FORCE)
        message(STATUS "Flash Attention: ENABLED (Release build)")
    else()
        # Optional for Debug builds to maintain debuggability
        set(LLAMA_FLASH_ATTN OFF CACHE BOOL "Enable Flash Attention optimization" FORCE)
        message(STATUS "Flash Attention: DISABLED (Debug build)")
    endif()
    
    # Enable Continuous Batching for all builds (8x throughput improvement)
    set(LLAMA_CONTINUOUS_BATCHING ON CACHE BOOL "Enable continuous batching" FORCE)
    message(STATUS "Continuous Batching: ENABLED (+8x throughput)")
    
    # Prefer repository-vendored llama.cpp when present to avoid fragile
    # FetchContent git clone/update behavior in source archives/submodule snapshots.
    if(EXISTS "${PROJECT_SOURCE_DIR}/llama.cpp/CMakeLists.txt")
        message(STATUS "llama.cpp: using local vendored source")
        FetchContent_Declare(
            llama_cpp
            SOURCE_DIR "${PROJECT_SOURCE_DIR}/llama.cpp"
            DOWNLOAD_COMMAND ""
            UPDATE_COMMAND ""
        )
    else()
        FetchContent_Declare(
            llama_cpp
            GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
            GIT_TAG ${LLAMA_CPP_GIT_TAG}
            GIT_SHALLOW FALSE  # Need full history for commit verification
            SOURCE_DIR "${PROJECT_SOURCE_DIR}/llama.cpp"
        )
    endif()
    
    FetchContent_MakeAvailable(llama_cpp)
    
    # =========================================================================
    # APPLY MSVC CHAR8_T FIXES TO LLAMA TARGETS
    # =========================================================================
    # After FetchContent_MakeAvailable, apply the compatibility flags
    if(MSVC AND TARGET llama)
        target_compile_options(llama PRIVATE /Zc:char8_t-)
        message(STATUS "Applied /Zc:char8_t- to llama target for MSVC compatibility")
    endif()
    
    if(MSVC AND TARGET ggml)
        target_compile_options(ggml PRIVATE /Zc:char8_t-)
        message(STATUS "Applied /Zc:char8_t- to ggml target for MSVC compatibility")
    endif()

    # Fix: CompilerOptions.cmake adds -ffast-math globally for Release builds.
    # -ffast-math implies -ffinite-math-only, which breaks ggml-cpu.c/vec.cpp/ops.cpp
    # that explicitly require non-finite math (NaN/Inf).
    # See: https://github.com/ggml-org/llama.cpp/pull/7154#issuecomment-2143844461
    if(NOT MSVC)
        foreach(_ggml_fix_target IN ITEMS ggml ggml-cpu ggml-alloc ggml-backend ggml-backend-reg)
            if(TARGET ${_ggml_fix_target})
                target_compile_options(${_ggml_fix_target} PRIVATE -fno-finite-math-only)
            endif()
        endforeach()
        message(STATUS "llama.cpp: Applied -fno-finite-math-only to ggml targets (Release -ffast-math override)")
    endif()

    # Ensure OpenMP is linked to llama target (only if found)
    if(TARGET llama)
        if(OpenMP_FOUND)
            target_link_libraries(llama PUBLIC OpenMP::OpenMP_C)
        endif()
        message(STATUS "llama.cpp configured successfully - LLM plugin support enabled")
        message(STATUS "  - Version: ${LLAMA_CPP_GIT_TAG}")
        message(STATUS "  - Flash Attention: ${LLAMA_FLASH_ATTN}")
        message(STATUS "  - Continuous Batching: ${LLAMA_CONTINUOUS_BATCHING}")
        add_compile_definitions(THEMIS_ENABLE_LLM=1)
        # Expose the pinned commit hash as a compile-time constant so that
        # LlamaWrapper can compare it against the runtime llama_build_commit()
        # value at startup and warn on mismatch (risk mitigation for API drift).
        add_compile_definitions(THEMIS_LLAMA_CPP_EXPECTED_COMMIT="${LLAMA_CPP_GIT_TAG}")
    else()
        message(FATAL_ERROR "llama.cpp target 'llama' not created after FetchContent")
    endif()
    
    # Voice assistant support (requires Whisper, Piper)
    if(THEMIS_ENABLE_VOICE_ASSISTANT)
        if(THEMIS_ENABLE_WHISPER)
            find_package(whisper QUIET CONFIG)
            if(whisper_FOUND)
                message(STATUS "Whisper.cpp found - enabling Speech-to-Text")
                add_compile_definitions(THEMIS_ENABLE_WHISPER=1)
            else()
                message(FATAL_ERROR "THEMIS_ENABLE_WHISPER=ON but whisper.cpp not found")
            endif()
        endif()
        
        if(THEMIS_ENABLE_PIPER_TTS)
            find_package(piper-phoneme-ids QUIET CONFIG)
            if(piper-phoneme-ids_FOUND)
                message(STATUS "Piper TTS found - enabling Text-to-Speech")
                add_compile_definitions(THEMIS_ENABLE_PIPER_TTS=1)
            else()
                message(FATAL_ERROR "THEMIS_ENABLE_PIPER_TTS=ON but Piper TTS not found")
            endif()
        endif()
    endif()

    # RNNoise: deep-learning noise suppression (optional, standalone – does not
    # require THEMIS_ENABLE_VOICE_ASSISTANT; usable in any audio pipeline).
    if(THEMIS_ENABLE_RNNOISE)
        find_package(rnnoise QUIET CONFIG)
        if(NOT rnnoise_FOUND)
            # Try pkg-config fallback (common on Linux distributions)
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(RNNOISE QUIET rnnoise)
            endif()
        endif()
        if(rnnoise_FOUND OR RNNOISE_FOUND)
            message(STATUS "RNNoise found - enabling deep-learning noise suppression")
            add_compile_definitions(THEMIS_ENABLE_RNNOISE=1)
            if(rnnoise_FOUND)
                target_link_libraries(themis_core PUBLIC rnnoise::rnnoise)
            else()
                target_include_directories(themis_core PUBLIC ${RNNOISE_INCLUDE_DIRS})
                target_link_libraries(themis_core PUBLIC ${RNNOISE_LIBRARIES})
            endif()
        else()
            message(FATAL_ERROR "THEMIS_ENABLE_RNNOISE=ON but RNNoise library not found. "
                "Install via vcpkg (rnnoise) or your system package manager (librnnoise-dev).")
        endif()
    endif()
endif()

# ============================================================================
# BENCHMARK-SPECIFIC DEPENDENCIES
# ============================================================================

if(THEMIS_BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED CONFIG)
    message(STATUS "Google Benchmark found")
    
    # Docker RAID benchmark extras
    if(THEMIS_BUILD_DOCKER_RAID_BENCHMARK)
        find_package(prometheus-cpp QUIET CONFIG)
        if(prometheus-cpp_FOUND)
            message(STATUS "prometheus-cpp found - enabling RAID benchmark metrics")
            add_compile_definitions(THEMIS_HAS_PROMETHEUS=1)
        endif()
    endif()
endif()

# ============================================================================
# CLOUD STORAGE DEPENDENCIES (GAP-008: Backup Automation)
# ============================================================================

# Cloud storage support for backup automation (AWS S3, Azure Blob, Google Cloud Storage)
option(THEMIS_ENABLE_CLOUD_STORAGE "Enable cloud storage backends for backup automation" OFF)

if(THEMIS_ENABLE_CLOUD_STORAGE)
    message(STATUS "Cloud storage support enabled - searching for SDKs...")
    
    # AWS SDK for C++ (S3 support)
    find_package(AWSSDK QUIET CONFIG COMPONENTS s3 transfer)
    if(AWSSDK_FOUND)
        message(STATUS "AWS SDK C++ found - enabling S3 backup support")
        add_compile_definitions(THEMIS_HAS_AWS_SDK=1)
        set(THEMIS_HAS_AWS_SDK ON)
    else()
        message(WARNING "AWS SDK C++ not found - S3 backup support disabled")
        message(STATUS "Install with: vcpkg install aws-sdk-cpp[s3,transfer]")
        set(THEMIS_HAS_AWS_SDK OFF)
    endif()
    
    # Azure Storage SDK for C++
    find_package(azure-storage-blobs-cpp QUIET CONFIG)
    if(azure-storage-blobs-cpp_FOUND)
        message(STATUS "Azure Storage C++ SDK found - enabling Azure Blob backup support")
        add_compile_definitions(THEMIS_HAS_AZURE_STORAGE=1)
        set(THEMIS_HAS_AZURE_STORAGE ON)
    else()
        message(WARNING "Azure Storage C++ SDK not found - Azure Blob backup support disabled")
        message(STATUS "Install with: vcpkg install azure-storage-blobs-cpp")
        set(THEMIS_HAS_AZURE_STORAGE OFF)
    endif()
    
    # Google Cloud C++ SDK (Storage support)
    find_package(google_cloud_cpp_storage QUIET CONFIG)
    if(google_cloud_cpp_storage_FOUND)
        message(STATUS "Google Cloud C++ SDK (Storage) found - enabling GCS backup support")
        add_compile_definitions(THEMIS_HAS_GCS_SDK=1)
        set(THEMIS_HAS_GCS_SDK ON)
    else()
        message(WARNING "Google Cloud C++ SDK (Storage) not found - GCS backup support disabled")
        message(STATUS "Install with: vcpkg install google-cloud-cpp[storage]")
        set(THEMIS_HAS_GCS_SDK OFF)
    endif()
    
    # Summary of cloud storage support
    if(NOT THEMIS_HAS_AWS_SDK AND NOT THEMIS_HAS_AZURE_STORAGE AND NOT THEMIS_HAS_GCS_SDK)
        message(WARNING "No cloud storage SDKs found - cloud backup features will be unavailable")
        message(STATUS "To enable cloud storage, install at least one SDK:")
        message(STATUS "  - AWS S3: vcpkg install aws-sdk-cpp[s3,transfer]")
        message(STATUS "  - Azure Blob: vcpkg install azure-storage-blobs-cpp")
        message(STATUS "  - Google Cloud Storage: vcpkg install google-cloud-cpp[storage]")
        set(THEMIS_ENABLE_CLOUD_STORAGE OFF CACHE BOOL "Disabled due to missing SDKs" FORCE)
    else()
        message(STATUS "Cloud storage SDKs enabled:")
        if(THEMIS_HAS_AWS_SDK)
            message(STATUS "  ✓ AWS S3")
        endif()
        if(THEMIS_HAS_AZURE_STORAGE)
            message(STATUS "  ✓ Azure Blob Storage")
        endif()
        if(THEMIS_HAS_GCS_SDK)
            message(STATUS "  ✓ Google Cloud Storage")
        endif()
    endif()
else()
    message(STATUS "Cloud storage support disabled (THEMIS_ENABLE_CLOUD_STORAGE=OFF)")
    message(STATUS "Enable with: cmake -DTHEMIS_ENABLE_CLOUD_STORAGE=ON")
endif()

# ============================================================================
# SUMMARY
# ============================================================================

message(STATUS "============================================")
message(STATUS "ThemisDB Dependencies Summary")
message(STATUS "============================================")
message(STATUS "Required: OpenSSL, RocksDB, gRPC, Protobuf, GTest")
message(STATUS "Optional: CURL, Arrow, Parquet, mimalloc, OpenTelemetry")
message(STATUS "Protocols: HTTP/2=${THEMIS_ENABLE_HTTP2}, HTTP/3=${THEMIS_ENABLE_HTTP3}")
message(STATUS "Features: LLM=${THEMIS_ENABLE_LLM}, GPU=${THEMIS_ENABLE_GPU}, CUDA=${THEMIS_ENABLE_CUDA}")
message(STATUS "Cloud Storage: Enabled=${THEMIS_ENABLE_CLOUD_STORAGE}")
if(THEMIS_ENABLE_CLOUD_STORAGE)
    message(STATUS "  - AWS S3: ${THEMIS_HAS_AWS_SDK}")
    message(STATUS "  - Azure Blob: ${THEMIS_HAS_AZURE_STORAGE}")
    message(STATUS "  - Google Cloud Storage: ${THEMIS_HAS_GCS_SDK}")
endif()
message(STATUS "============================================")
