cmake_minimum_required(VERSION 3.20)
project(themisdb_storage VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── ThemisDB SDK dependency ────────────────────────────────────────────────────
if(DEFINED THEMISDB_SDK_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${THEMISDB_SDK_DIR}")
endif()
find_package(themisdb_sdk QUIET)

# ── Subdirectories ────────────────────────────────────────────────────────────
# Shared blob backend infrastructure (base class, manager, redundancy)
add_subdirectory(shared)

# Multi-level encrypted user storage
add_subdirectory(user_storage_encrypted)

# Azure Blob Storage backend
option(THEMISDB_STORAGE_AZURE "Build Azure Blob Storage backend" OFF)
if(THEMISDB_STORAGE_AZURE)
    add_subdirectory(azure_blob_storage)
endif()

# AWS S3 Blob Storage backend
option(THEMISDB_STORAGE_S3 "Build S3 Blob Storage backend" OFF)
if(THEMISDB_STORAGE_S3)
    add_subdirectory(s3_blob_storage)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
option(THEMISDB_STORAGE_BUILD_TESTS "Build storage tests" OFF)
if(THEMISDB_STORAGE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(user_storage_encrypted/tests)
endif()
