# CMake configuration for Themis Admin Tools (.NET 8 WPF Applications)
# 
# This CMakeLists.txt provides integration of .NET admin tools into the main
# CMake build system. The actual building is done by the .NET SDK (dotnet build).

cmake_minimum_required(VERSION 3.20)
project(ThemisAdminTools LANGUAGES NONE)

# ============================================================================
# C++ Tools
# ============================================================================

# Embed Certificate Tool - Convert certificates to C++ const arrays
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/embed_certificate.cpp")
    message(STATUS "Adding embed_certificate tool")
    
    # Create a separate project for C++ tools
    project(ThemisTools LANGUAGES CXX)
    
    add_executable(embed_certificate
        embed_certificate.cpp
    )
    
    # No external dependencies needed - uses only standard library
    target_compile_features(embed_certificate PRIVATE cxx_std_17)
    
    # Install the tool to bin directory
    install(TARGETS embed_certificate
        RUNTIME DESTINATION bin
        COMPONENT tools
    )
    
    message(STATUS "embed_certificate tool configured")
endif()

# PostgreSQL Dump Import CLI
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/import_cli.cpp")
    message(STATUS "Adding themisdb-import CLI tool")
    
    project(ThemisTools LANGUAGES CXX)
    
    add_executable(themisdb-import
        import_cli.cpp
    )
    
    target_compile_features(themisdb-import PRIVATE cxx_std_17)
    
    # nlohmann/json is header-only; find it via vcpkg or system
    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        target_link_libraries(themisdb-import PRIVATE nlohmann_json::nlohmann_json)
    else()
        # Fallback: assume json.hpp is on the include path
        message(STATUS "nlohmann_json package not found; assuming json.hpp is on include path")
    endif()
    
    install(TARGETS themisdb-import
        RUNTIME DESTINATION bin
        COMPONENT tools
    )
    
    message(STATUS "themisdb-import CLI tool configured")
endif()

# LoRA Provenance CLI
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lora_provenance_cli.cpp")
    if(THEMIS_ENABLE_LLM)
        message(STATUS "Adding lora-provenance CLI tool")

        project(ThemisTools LANGUAGES CXX)

        add_executable(lora-provenance
            lora_provenance_cli.cpp
        )

        target_compile_features(lora-provenance PRIVATE cxx_std_17)

        target_include_directories(lora-provenance PRIVATE
            ${CMAKE_SOURCE_DIR}/include
        )

        # Link against the main Themis library so lora_provenance.cpp symbols are available
        if(TARGET themis_core)
            target_link_libraries(lora-provenance PRIVATE themis_core)
        endif()

        find_package(nlohmann_json QUIET)
        if(nlohmann_json_FOUND)
            target_link_libraries(lora-provenance PRIVATE nlohmann_json::nlohmann_json)
        endif()

        find_package(OpenSSL QUIET)
        if(OpenSSL_FOUND)
            target_link_libraries(lora-provenance PRIVATE OpenSSL::Crypto)
        endif()

        install(TARGETS lora-provenance
            RUNTIME DESTINATION bin
            COMPONENT tools
        )

        message(STATUS "lora-provenance CLI tool configured")
    else()
        message(STATUS "Skipping lora-provenance tool (THEMIS_ENABLE_LLM is OFF)")
    endif()
endif()

# ============================================================================
# themisctl — Unified ThemisDB Management CLI
# ============================================================================

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/themisctl.cpp")
    message(STATUS "Adding themisctl unified management CLI")

    project(ThemisTools LANGUAGES CXX)

    add_executable(themisctl
        themisctl.cpp
    )

    target_compile_features(themisctl PRIVATE cxx_std_17)

    target_include_directories(themisctl PRIVATE ${CMAKE_SOURCE_DIR}/include)

    # cpp-httplib provides the HTTP client (header-only)
    find_package(httplib QUIET)
    if(httplib_FOUND)
        target_link_libraries(themisctl PRIVATE httplib::httplib)
    else()
        # Fallback: assume httplib.h is on the include path (e.g. via vcpkg toolchain)
        message(STATUS "httplib package not found via find_package; assuming httplib.h is on include path")
    endif()

    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        target_link_libraries(themisctl PRIVATE nlohmann_json::nlohmann_json)
    endif()

    # OpenSSL is optional (HTTPS support for themisctl via cpp-httplib)
    find_package(OpenSSL QUIET)
    if(OpenSSL_FOUND)
        target_link_libraries(themisctl PRIVATE OpenSSL::SSL OpenSSL::Crypto)
        target_compile_definitions(themisctl PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    endif()

    # GNU Readline for REPL command history (optional)
    find_package(Readline QUIET)
    if(Readline_FOUND)
        target_link_libraries(themisctl PRIVATE ${READLINE_LIBRARIES})
        target_include_directories(themisctl PRIVATE ${READLINE_INCLUDE_DIR})
        target_compile_definitions(themisctl PRIVATE THEMISCTL_ENABLE_READLINE=1)
        message(STATUS "themisctl: readline found — REPL will use line editing + history")
    else()
        # Fallback: check for the library directly (common on Debian/Ubuntu)
        find_library(READLINE_LIB readline)
        if(READLINE_LIB)
            target_link_libraries(themisctl PRIVATE ${READLINE_LIB})
            target_compile_definitions(themisctl PRIVATE THEMISCTL_ENABLE_READLINE=1)
            message(STATUS "themisctl: readline found (manual) — REPL will use line editing + history")
        else()
            message(STATUS "themisctl: readline not found — REPL will use plain getline()")
        endif()
    endif()

    install(TARGETS themisctl
        RUNTIME DESTINATION bin
        COMPONENT tools
    )

    # ── Shell completion scripts ───────────────────────────────────────────────
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/completion/themisctl.bash")
        install(FILES completion/themisctl.bash
            DESTINATION share/bash-completion/completions
            RENAME themisctl
            COMPONENT tools
        )
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/completion/_themisctl")
        install(FILES completion/_themisctl
            DESTINATION share/zsh/site-functions
            COMPONENT tools
        )
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/completion/themisctl.fish")
        install(FILES completion/themisctl.fish
            DESTINATION share/fish/vendor_completions.d
            COMPONENT tools
        )
    endif()

    message(STATUS "themisctl unified management CLI configured")
endif()

# ============================================================================
# Config Migration Scanner
# ============================================================================

# This block runs only when building tools standalone (i.e. directly via
# cmake -S tools/ ...) rather than as a subdirectory of the main project.
# When included from cmake/CMakeLists.txt, the config_migration_scanner target
# is already defined there (linking against themis_core), so the guard
# prevents a duplicate-target CMake error.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config_migration_scanner.cpp" AND NOT TARGET config_migration_scanner)
    message(STATUS "Adding config_migration_scanner tool (standalone build)")

    project(ThemisTools LANGUAGES CXX)

    add_executable(config_migration_scanner
        config_migration_scanner.cpp
        ../src/config/config_path_resolver.cpp
        ../src/config/config_audit_log.cpp
    )

    target_compile_features(config_migration_scanner PRIVATE cxx_std_20)

    # Use CMAKE_CURRENT_SOURCE_DIR (the tools/ directory) so that relative
    # paths resolve correctly in both standalone and integrated builds.
    target_include_directories(config_migration_scanner PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    # spdlog is required by config_path_resolver
    find_package(spdlog QUIET)
    if(spdlog_FOUND)
        target_link_libraries(config_migration_scanner PRIVATE spdlog::spdlog)
    endif()

    install(TARGETS config_migration_scanner
        RUNTIME DESTINATION bin
        COMPONENT tools
    )

    message(STATUS "config_migration_scanner tool configured")
endif()

# ============================================================================
# .NET Admin Tools
# ============================================================================

# Check if .NET SDK is available
find_program(DOTNET_EXECUTABLE dotnet)

if(DOTNET_EXECUTABLE)
    message(STATUS "Found .NET SDK for Themis Admin Tools: ${DOTNET_EXECUTABLE}")
    
    # Include the AQL Query Builder
    add_subdirectory(Themis.AqlQueryBuilder)
    
    # Optionally, add other .NET admin tools here in the future
    # add_subdirectory(Themis.AuditLogViewer)
    # add_subdirectory(Themis.PIIManager)
    # add_subdirectory(Themis.KeyRotationDashboard)
    # etc.
    
    message(STATUS "Themis Admin Tools (.NET) configured successfully")
else()
    message(WARNING ".NET admin tools require .NET SDK 8.0 or later")
    message(WARNING "Install .NET SDK from: https://dotnet.microsoft.com/download")
    message(WARNING "Skipping .NET admin tools build")
endif()
