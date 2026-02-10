# ThemisDB Security Features
# Encryption, authentication, HSM, tracing

# OpenTelemetry tracing (already set by edition, but allow user override if not forced)
if(NOT DEFINED THEMIS_ENABLE_TRACING)
    option(THEMIS_ENABLE_TRACING "Enable OpenTelemetry tracing" OFF)
endif()

# HSM (Hardware Security Module) - Real provider
if(NOT DEFINED THEMIS_ENABLE_HSM_REAL)
    option(THEMIS_ENABLE_HSM_REAL "Enable real HSM provider" OFF)
endif()

# RFC 3161 Timestamp Authority (TSA) - OpenSSL implementation
if(NOT DEFINED THEMIS_USE_OPENSSL_TSA)
    option(THEMIS_USE_OPENSSL_TSA "Enable OpenSSL-based RFC 3161 Timestamp Authority (production)" ON)
endif()

# Blob storage backends
if(NOT DEFINED THEMIS_ENABLE_S3)
    option(THEMIS_ENABLE_S3 "Enable S3 blob storage backend" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_AZURE)
    option(THEMIS_ENABLE_AZURE "Enable Azure Blob Storage backend" OFF)
endif()

if(NOT DEFINED THEMIS_ENABLE_WEBDAV)
    option(THEMIS_ENABLE_WEBDAV "Enable WebDAV blob storage backend" OFF)
endif()

# Display security features
message(STATUS "  Security & Enterprise:")
if(THEMIS_ENABLE_TRACING)
    message(STATUS "    OpenTelemetry tracing: Enabled")
endif()
if(THEMIS_ENABLE_HSM_REAL)
    message(STATUS "    Real HSM provider: Enabled")
endif()
if(THEMIS_USE_OPENSSL_TSA)
    message(STATUS "    RFC 3161 TSA (OpenSSL): Enabled")
else()
    message(WARNING "    RFC 3161 TSA: STUB MODE (development only - NOT for production!)")
endif()
if(THEMIS_ENABLE_S3)
    message(STATUS "    S3 storage: Enabled")
endif()
if(THEMIS_ENABLE_AZURE)
    message(STATUS "    Azure storage: Enabled")
endif()
if(THEMIS_ENABLE_WEBDAV)
    message(STATUS "    WebDAV storage: Enabled")
endif()
