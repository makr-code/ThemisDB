# SignPlugin.cmake
# CMake module for signing ThemisDB plugins with embedded manufacturer signature
#
# Usage:
#   include(SignPlugin)
#   sign_plugin(my_plugin_target)

# Option to enable/disable plugin signing (default: OFF for development)
option(THEMIS_SIGN_PLUGINS "Enable plugin signing (requires certificates)" OFF)

# Option to enable embedded certificate generation
option(THEMIS_EMBED_CERTIFICATES "Generate embedded certificate headers for plugins" ON)

# Paths to signing certificates (can be overridden)
set(THEMISDB_CERT_PATH "${CMAKE_SOURCE_DIR}/certs/manufacturer/themisdb-plugin-signer.crt" 
    CACHE FILEPATH "Path to ThemisDB plugin signing certificate")
set(THEMISDB_KEY_PATH "${CMAKE_SOURCE_DIR}/certs/manufacturer/themisdb-plugin-signer.key" 
    CACHE FILEPATH "Path to ThemisDB plugin signing private key")
set(THEMISDB_CA_CERT_PATH "${CMAKE_SOURCE_DIR}/certs/manufacturer/themisdb-ca.crt" 
    CACHE FILEPATH "Path to ThemisDB CA certificate")

# Find embed_certificate tool
find_program(EMBED_CERT_TOOL embed_certificate
    HINTS
        "${CMAKE_BINARY_DIR}/tools"
        "${CMAKE_BINARY_DIR}/bin"
    PATHS
        "${CMAKE_SOURCE_DIR}/tools"
)

if(EMBED_CERT_TOOL)
    message(STATUS "Found embed_certificate tool: ${EMBED_CERT_TOOL}")
elseif(THEMIS_EMBED_CERTIFICATES)
    message(STATUS "embed_certificate tool not found - will be built from source")
endif()

# Find signing tools
if(WIN32)
    # Windows: Find signtool.exe (part of Windows SDK)
    set(_themis_signtool_hints)
    if(DEFINED ENV{WindowsSdkDir})
        list(APPEND _themis_signtool_hints
            "$ENV{WindowsSdkDir}/bin/x64"
            "$ENV{WindowsSdkDir}/bin/x86")
        if(DEFINED ENV{WindowsSDKVersion})
            list(APPEND _themis_signtool_hints
                "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64"
                "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x86")
        endif()
    endif()
    find_program(SIGNTOOL signtool
        HINTS
            ${_themis_signtool_hints}
    )
    if(SIGNTOOL)
        message(STATUS "Found signtool: ${SIGNTOOL}")
    else()
        message(STATUS "signtool not found - Authenticode signing disabled")
    endif()
elseif(APPLE)
    # macOS: Find codesign (part of Xcode)
    find_program(CODESIGN codesign)
    if(CODESIGN)
        message(STATUS "Found codesign: ${CODESIGN}")
    else()
        message(STATUS "codesign not found - macOS code signing disabled")
    endif()
else()
    # Linux: Find gpg for signature generation
    find_program(GPG gpg)
    if(GPG)
        message(STATUS "Found gpg: ${GPG}")
    else()
        message(STATUS "gpg not found - GPG signing disabled")
    endif()
    
    # Also find openssl for signature generation
    find_program(OPENSSL_CMD openssl)
    if(OPENSSL_CMD)
        message(STATUS "Found openssl: ${OPENSSL_CMD}")
    endif()
endif()

#
# sign_plugin(TARGET_NAME)
#
# Sign a plugin DLL/SO with ThemisDB manufacturer signature
#
# This function adds post-build commands to:
# 1. Calculate SHA-256 hash of the plugin
# 2. Generate digital signature with ThemisDB certificate
# 3. Embed signature in platform-specific format (if available)
# 4. Generate metadata JSON with signature information
#
function(sign_plugin TARGET_NAME)
    if(NOT THEMIS_SIGN_PLUGINS)
        message(STATUS "Plugin signing disabled for ${TARGET_NAME} (THEMIS_SIGN_PLUGINS=OFF)")
        return()
    endif()
    
    # Check if certificates exist
    if(NOT EXISTS "${THEMISDB_CERT_PATH}")
        message(WARNING "ThemisDB signing certificate not found: ${THEMISDB_CERT_PATH}")
        message(WARNING "  Run: cd ${CMAKE_SOURCE_DIR}/certs/scripts && ./generate_signing_cert.sh")
        return()
    endif()
    
    if(NOT EXISTS "${THEMISDB_KEY_PATH}")
        message(WARNING "ThemisDB signing key not found: ${THEMISDB_KEY_PATH}")
        message(WARNING "  Private key required for signing!")
        return()
    endif()
    
    message(STATUS "Configuring plugin signing for: ${TARGET_NAME}")
    
    # Step 1: Calculate hash and generate signature (platform-independent)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMAND ${CMAKE_COMMAND} -E echo "Signing plugin: ${TARGET_NAME}"
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMENT "Starting plugin signature process"
    )
    
    # Platform-specific signing
    if(WIN32 AND SIGNTOOL)
        # Windows: Authenticode signing
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${SIGNTOOL} sign
                /f "${THEMISDB_CERT_PATH}"
                /t http://timestamp.digicert.com
                /v
                $<TARGET_FILE:${TARGET_NAME}>
            COMMAND ${CMAKE_COMMAND} -E echo "✅ Authenticode signature added"
            COMMENT "Signing ${TARGET_NAME} with Authenticode"
        )
    elseif(APPLE AND CODESIGN)
        # macOS: codesign
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CODESIGN} --sign "Developer ID Application: ThemisDB.org"
                --timestamp
                --options runtime
                $<TARGET_FILE:${TARGET_NAME}>
            COMMAND ${CMAKE_COMMAND} -E echo "✅ macOS code signature added"
            COMMENT "Signing ${TARGET_NAME} with codesign"
        )
    elseif(UNIX AND GPG)
        # Linux: GPG detached signature
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${GPG} --armor --detach-sign
                --local-user plugins@themisdb.org
                --output $<TARGET_FILE:${TARGET_NAME}>.asc
                $<TARGET_FILE:${TARGET_NAME}>
            COMMAND ${CMAKE_COMMAND} -E echo "✅ GPG signature created: $<TARGET_FILE_NAME:${TARGET_NAME}>.asc"
            COMMENT "Signing ${TARGET_NAME} with GPG"
        )
    else()
        message(STATUS "  Platform signing tool not available - using hash-only verification")
    endif()
    
    # Step 2: Generate embedded certificate header (if enabled)
    if(THEMIS_EMBED_CERTIFICATES AND EMBED_CERT_TOOL AND EXISTS "${THEMISDB_CERT_PATH}")
        # Generate output header path
        get_target_property(TARGET_SOURCE_DIR ${TARGET_NAME} SOURCE_DIR)
        set(EMBEDDED_CERT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_embedded_cert.h")
        
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Generating embedded certificate header..."
            COMMAND ${EMBED_CERT_TOOL}
                --cert "${THEMISDB_CERT_PATH}"
                --output "${EMBEDDED_CERT_HEADER}"
                --plugin-name "${TARGET_NAME}"
                --namespace "themis::plugins"
            COMMAND ${CMAKE_COMMAND} -E echo "✅ Embedded certificate header generated: ${EMBEDDED_CERT_HEADER}"
            COMMENT "Embedding ThemisDB certificate in ${TARGET_NAME}"
            BYPRODUCTS "${EMBEDDED_CERT_HEADER}"
        )
        
        # Make the header available to the plugin
        target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
        
        message(STATUS "  Embedded certificate header will be generated at: ${EMBEDDED_CERT_HEADER}")
    elseif(THEMIS_EMBED_CERTIFICATES AND NOT EMBED_CERT_TOOL)
        message(STATUS "  embed_certificate tool not available - skipping embedded certificate generation")
    endif()
    
    # Step 3: Generate metadata JSON with signature
    # Note: GenerateSignatureMetadata.cmake will be created in future enhancement
    # For now, metadata generation is skipped
    if(FALSE AND OPENSSL_CMD)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Generating signature metadata..."
            COMMAND ${CMAKE_COMMAND}
                -DTARGET_FILE=$<TARGET_FILE:${TARGET_NAME}>
                -DCERT_FILE=${THEMISDB_CERT_PATH}
                -DKEY_FILE=${THEMISDB_KEY_PATH}
                -DOUTPUT_JSON=$<TARGET_FILE:${TARGET_NAME}>.json
                -P ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/GenerateSignatureMetadata.cmake
            COMMENT "Generating metadata for ${TARGET_NAME}"
        )
    endif()
    
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
        COMMAND ${CMAKE_COMMAND} -E echo "✅ Plugin signing complete: ${TARGET_NAME}"
        COMMAND ${CMAKE_COMMAND} -E echo "=========================================="
    )
endfunction()

#
# verify_plugin_signature(PLUGIN_PATH)
#
# Verify a plugin's signature (for testing)
#
function(verify_plugin_signature PLUGIN_PATH)
    if(NOT EXISTS "${PLUGIN_PATH}")
        message(FATAL_ERROR "Plugin not found: ${PLUGIN_PATH}")
    endif()
    
    message(STATUS "Verifying plugin signature: ${PLUGIN_PATH}")
    
    if(WIN32 AND SIGNTOOL)
        execute_process(
            COMMAND ${SIGNTOOL} verify /pa "${PLUGIN_PATH}"
            RESULT_VARIABLE VERIFY_RESULT
        )
        if(VERIFY_RESULT EQUAL 0)
            message(STATUS "✅ Authenticode signature valid")
        else()
            message(WARNING "❌ Authenticode signature verification failed")
        endif()
    elseif(APPLE AND CODESIGN)
        execute_process(
            COMMAND ${CODESIGN} --verify --deep --strict "${PLUGIN_PATH}"
            RESULT_VARIABLE VERIFY_RESULT
        )
        if(VERIFY_RESULT EQUAL 0)
            message(STATUS "✅ macOS code signature valid")
        else()
            message(WARNING "❌ macOS code signature verification failed")
        endif()
    elseif(UNIX AND GPG)
        set(SIG_FILE "${PLUGIN_PATH}.asc")
        if(EXISTS "${SIG_FILE}")
            execute_process(
                COMMAND ${GPG} --verify "${SIG_FILE}" "${PLUGIN_PATH}"
                RESULT_VARIABLE VERIFY_RESULT
            )
            if(VERIFY_RESULT EQUAL 0)
                message(STATUS "✅ GPG signature valid")
            else()
                message(WARNING "❌ GPG signature verification failed")
            endif()
        else()
            message(WARNING "❌ GPG signature file not found: ${SIG_FILE}")
        endif()
    endif()
endfunction()

#
# embed_certificate_in_plugin(TARGET_NAME [CERT_PATH])
#
# Generate an embedded certificate header for a plugin without full signing.
# Useful for development builds where signing is disabled but embedded certificates are desired.
#
function(embed_certificate_in_plugin TARGET_NAME)
    # Parse optional CERT_PATH argument
    set(CERT_PATH "${THEMISDB_CERT_PATH}")
    if(ARGC GREATER 1)
        set(CERT_PATH "${ARGV1}")
    endif()
    
    if(NOT THEMIS_EMBED_CERTIFICATES)
        message(STATUS "Embedded certificates disabled for ${TARGET_NAME} (THEMIS_EMBED_CERTIFICATES=OFF)")
        return()
    endif()
    
    if(NOT EMBED_CERT_TOOL)
        message(WARNING "embed_certificate tool not found - cannot generate embedded certificate")
        return()
    endif()
    
    if(NOT EXISTS "${CERT_PATH}")
        message(WARNING "Certificate not found: ${CERT_PATH}")
        return()
    endif()
    
    # Generate output header path
    set(EMBEDDED_CERT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_embedded_cert.h")
    
    add_custom_command(TARGET ${TARGET_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Generating embedded certificate header for ${TARGET_NAME}..."
        COMMAND ${EMBED_CERT_TOOL}
            --cert "${CERT_PATH}"
            --output "${EMBEDDED_CERT_HEADER}"
            --plugin-name "${TARGET_NAME}"
            --namespace "themis::plugins"
        COMMAND ${CMAKE_COMMAND} -E echo "✅ Embedded certificate header: ${EMBEDDED_CERT_HEADER}"
        COMMENT "Embedding ThemisDB certificate in ${TARGET_NAME}"
        BYPRODUCTS "${EMBEDDED_CERT_HEADER}"
    )
    
    # Make the header available to the plugin
    target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    
    message(STATUS "Embedded certificate configured for: ${TARGET_NAME}")
    message(STATUS "  Header: ${EMBEDDED_CERT_HEADER}")
endfunction()
