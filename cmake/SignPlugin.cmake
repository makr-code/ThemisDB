# SignPlugin.cmake
# CMake module for signing ThemisDB plugins with embedded manufacturer signature
#
# Usage:
#   include(SignPlugin)
#   sign_plugin(my_plugin_target)

# Option to enable/disable plugin signing (default: OFF for development)
option(THEMIS_SIGN_PLUGINS "Enable plugin signing (requires certificates)" OFF)

# Paths to signing certificates (can be overridden)
set(THEMISDB_CERT_PATH "${CMAKE_SOURCE_DIR}/certs/manufacturer/themisdb-plugin-signer.crt" 
    CACHE FILEPATH "Path to ThemisDB plugin signing certificate")
set(THEMISDB_KEY_PATH "${CMAKE_SOURCE_DIR}/certs/manufacturer/themisdb-plugin-signer.key" 
    CACHE FILEPATH "Path to ThemisDB plugin signing private key")
set(THEMISDB_CA_CERT_PATH "${CMAKE_SOURCE_DIR}/certs/manufacturer/themisdb-ca.crt" 
    CACHE FILEPATH "Path to ThemisDB CA certificate")

# Find signing tools
if(WIN32)
    # Windows: Find signtool.exe (part of Windows SDK)
    find_program(SIGNTOOL signtool
        HINTS
            "C:/Program Files (x86)/Windows Kits/10/bin/x64"
            "C:/Program Files (x86)/Windows Kits/10/bin/x86"
            "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/x64"
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
    
    # Step 2: Generate metadata JSON with signature
    if(OPENSSL_CMD)
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
