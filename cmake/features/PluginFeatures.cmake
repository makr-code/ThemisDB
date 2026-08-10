# ThemisDB Plugin Feature Configuration
# Per-plugin compiler switches for optional plugin builds.
# Include order: after EditionDefaults.cmake (THEMIS_EDITION_SELECTED must be set)

if(NOT THEMIS_FEATURES_CONFIGURED AND NOT THEMIS_EDITION_SELECTED)
    message(FATAL_ERROR "PluginFeatures.cmake must be included after EditionDefaults.cmake")
endif()

message(STATUS "  Plugin Features:")

# ---------------------------------------------------------------------------
# Public optional module externalization plugins (geo / timeseries)
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_GEO "Enable optional external Geo plugin submodule integration" ON)
option(THEMIS_PLUGIN_TIMESERIES "Enable optional external TimeSeries plugin submodule integration" ON)

# Externalization flags: when ON, use external plugin submodules instead of internal sources
option(THEMIS_EXTERNALIZE_GEO_PLUGIN "Externalize Geo module to optional public submodule (plugins/themisdb_geo)" OFF)
option(THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN "Externalize TimeSeries module to optional public submodule (plugins/themisdb_timeseries)" OFF)

if(THEMIS_PLUGIN_GEO)
    add_compile_definitions(THEMIS_PLUGIN_GEO_ENABLED)
    if(THEMIS_EXTERNALIZE_GEO_PLUGIN)
        message(STATUS "    Geo Plugin:                        ON (externalized to submodule)")
    else()
        message(STATUS "    Geo Plugin:                        ON (integrated source)")
    endif()
else()
    message(STATUS "    Geo Plugin:                        OFF")
endif()

if(THEMIS_PLUGIN_TIMESERIES)
    add_compile_definitions(THEMIS_PLUGIN_TIMESERIES_ENABLED)
    if(THEMIS_EXTERNALIZE_TIMESERIES_PLUGIN)
        message(STATUS "    TimeSeries Plugin:                 ON (externalized to submodule)")
    else()
        message(STATUS "    TimeSeries Plugin:                 ON (integrated source)")
    endif()
else()
    message(STATUS "    TimeSeries Plugin:                 OFF")
endif()

# ---------------------------------------------------------------------------
# Core plugin infrastructure (always on; part of themis_core)
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_SYSTEM "Build core plugin infrastructure (plugin manager, registry, hot-reload)" ON)
if(THEMIS_PLUGIN_SYSTEM)
    add_compile_definitions(THEMIS_PLUGIN_SYSTEM_ENABLED)
    message(STATUS "    Plugin System (core):              ON")
else()
    message(STATUS "    Plugin System (core):              OFF")
endif()

# ---------------------------------------------------------------------------
# Ethics AI Plugin (v1.4.0+)
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_ETHICS_AI "Build the Ethics AI plugin (philosophical evaluation framework)" ON)
if(THEMIS_PLUGIN_ETHICS_AI)
    add_compile_definitions(THEMIS_PLUGIN_ETHICS_AI_ENABLED)
    message(STATUS "    Ethics AI Plugin:                  ON")
else()
    message(STATUS "    Ethics AI Plugin:                  OFF")
endif()

# ---------------------------------------------------------------------------
# gRPC RPC Plugin (v1.3.0+)
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_RPC_GRPC "Build the gRPC RPC plugin for remote procedure calls" ON)
if(THEMIS_PLUGIN_RPC_GRPC)
    add_compile_definitions(THEMIS_PLUGIN_RPC_GRPC_ENABLED)
    message(STATUS "    gRPC RPC Plugin:                   ON")
else()
    message(STATUS "    gRPC RPC Plugin:                   OFF")
endif()

# ---------------------------------------------------------------------------
# Image Analysis – ONNX CLIP Plugin
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX "Build the ONNX CLIP image analysis plugin" ON)
if(THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX)
    add_compile_definitions(THEMIS_PLUGIN_IMAGE_ANALYSIS_ONNX_ENABLED)
    message(STATUS "    Image Analysis (ONNX CLIP):        ON")
else()
    message(STATUS "    Image Analysis (ONNX CLIP):        OFF")
endif()

# ---------------------------------------------------------------------------
# User Storage Encrypted Plugin (v1.5.0+)
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED "Build the multi-level encrypted user storage plugin" ON)
if(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED)
    add_compile_definitions(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED_ENABLED)
    message(STATUS "    User Storage Encrypted Plugin:     ON")
else()
    message(STATUS "    User Storage Encrypted Plugin:     OFF")
endif()

# ---------------------------------------------------------------------------
# Data Importer Plugins
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_IMPORTER_POSTGRES  "Enable PostgreSQL importer plugin manifest" ON)
option(THEMIS_PLUGIN_IMPORTER_MYSQL     "Enable MySQL importer plugin manifest"      ON)
option(THEMIS_PLUGIN_IMPORTER_SQLITE    "Enable SQLite importer plugin manifest"     ON)
option(THEMIS_PLUGIN_IMPORTER_MONGODB   "Enable MongoDB importer plugin manifest"    ON)
option(THEMIS_PLUGIN_IMPORTER_KAFKA     "Enable Kafka importer plugin manifest"      ON)
option(THEMIS_PLUGIN_IMPORTER_S3        "Enable S3 importer plugin manifest"         ON)

foreach(_imp IN ITEMS POSTGRES MYSQL SQLITE MONGODB KAFKA S3)
    if(THEMIS_PLUGIN_IMPORTER_${_imp})
        add_compile_definitions(THEMIS_PLUGIN_IMPORTER_${_imp}_ENABLED)
    endif()
endforeach()
message(STATUS "    Importer Plugins (postgres/mysql/sqlite/mongodb/kafka/s3): configured per flag")

# ---------------------------------------------------------------------------
# Blob Storage Plugins
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_BLOB_S3    "Enable AWS S3 blob storage plugin"     ON)
option(THEMIS_PLUGIN_BLOB_AZURE "Enable Azure blob storage plugin"      ON)
option(THEMIS_PLUGIN_BLOB_GCS   "Enable Google Cloud Storage blob storage plugin" ON)

if(NOT DEFINED THEMIS_ENABLE_BLOB_S3)
    set(THEMIS_ENABLE_BLOB_S3 "${THEMIS_PLUGIN_BLOB_S3}" CACHE BOOL
        "Enable AWS S3 blob backend in modular builds" FORCE)
endif()
if(NOT DEFINED THEMIS_ENABLE_BLOB_AZURE)
    set(THEMIS_ENABLE_BLOB_AZURE "${THEMIS_PLUGIN_BLOB_AZURE}" CACHE BOOL
        "Enable Azure blob backend in modular builds" FORCE)
endif()
if(NOT DEFINED THEMIS_ENABLE_BLOB_GCS)
    set(THEMIS_ENABLE_BLOB_GCS "${THEMIS_PLUGIN_BLOB_GCS}" CACHE BOOL
        "Enable GCS blob backend in modular builds" FORCE)
endif()

if(THEMIS_PLUGIN_BLOB_S3)
    add_compile_definitions(THEMIS_PLUGIN_BLOB_S3_ENABLED)
endif()
if(THEMIS_PLUGIN_BLOB_AZURE)
    add_compile_definitions(THEMIS_PLUGIN_BLOB_AZURE_ENABLED)
endif()
if(THEMIS_PLUGIN_BLOB_GCS)
    add_compile_definitions(THEMIS_PLUGIN_BLOB_GCS_ENABLED)
endif()
message(STATUS "    Blob Storage Plugins (s3/azure/gcs): configured per flag")

# ---------------------------------------------------------------------------
# HuggingFace Ingestion Plugin (integrated in src/importers/huggingface_ingestion_plugin.cpp)
# ---------------------------------------------------------------------------
option(THEMIS_PLUGIN_HUGGINGFACE "Build the HuggingFace model ingestion plugin" ON)
if(THEMIS_PLUGIN_HUGGINGFACE)
    add_compile_definitions(THEMIS_PLUGIN_HUGGINGFACE_ENABLED)
    message(STATUS "    HuggingFace Ingestion Plugin:      ON")
else()
    message(STATUS "    HuggingFace Ingestion Plugin:      OFF")
endif()

# ---------------------------------------------------------------------------
# Edition-based overrides
# Minimal/Community editions disable enterprise-only plugins automatically.
# ---------------------------------------------------------------------------
if(THEMIS_EDITION MATCHES "^(MINIMAL|COMMUNITY)$")
    if(THEMIS_PLUGIN_ETHICS_AI AND NOT THEMIS_DEV_ETHICS_AI_OVERRIDE)
        message(STATUS "    [Edition override] Ethics AI plugin requires ENTERPRISE or higher")
        set(THEMIS_PLUGIN_ETHICS_AI OFF CACHE BOOL "Ethics AI plugin disabled by edition" FORCE)
        remove_definitions(-DTHEMIS_PLUGIN_ETHICS_AI_ENABLED)
    elseif(THEMIS_PLUGIN_ETHICS_AI AND THEMIS_DEV_ETHICS_AI_OVERRIDE)
        message(STATUS "    [Dev override] Ethics AI enabled in ${THEMIS_EDITION} edition (THEMIS_DEV_ETHICS_AI_OVERRIDE=ON)")
    endif()
    if(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED)
        message(STATUS "    [Edition override] User Storage Encrypted plugin requires ENTERPRISE or higher")
        set(THEMIS_PLUGIN_USER_STORAGE_ENCRYPTED OFF CACHE BOOL "User Storage Encrypted plugin disabled by edition" FORCE)
        remove_definitions(-DTHEMIS_PLUGIN_USER_STORAGE_ENCRYPTED_ENABLED)
    endif()
endif()
