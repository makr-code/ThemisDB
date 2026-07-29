cmake_minimum_required(VERSION 3.20)
project(themisdb_importer VERSION 1.7.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── ThemisDB SDK dependency ────────────────────────────────────────────────────
if(DEFINED THEMISDB_SDK_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${THEMISDB_SDK_DIR}")
endif()
find_package(themisdb_sdk QUIET)

# ── Shared importer infrastructure ────────────────────────────────────────────
add_subdirectory(shared)

# ── Individual importer plugins ───────────────────────────────────────────────
option(THEMISDB_IMPORTER_MYSQL  "Build MySQL importer"  ON)
option(THEMISDB_IMPORTER_MONGO  "Build MongoDB importer" ON)
option(THEMISDB_IMPORTER_KAFKA  "Build Kafka importer"  ON)
option(THEMISDB_IMPORTER_S3     "Build S3 importer"     ON)

if(THEMISDB_IMPORTER_MYSQL)
    add_subdirectory(mysql)
endif()
if(THEMISDB_IMPORTER_MONGO)
    add_subdirectory(mongo)
endif()
if(THEMISDB_IMPORTER_KAFKA)
    add_subdirectory(kafka)
endif()
if(THEMISDB_IMPORTER_S3)
    add_subdirectory(s3)
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
option(THEMISDB_IMPORTER_BUILD_TESTS "Build importer tests" OFF)
if(THEMISDB_IMPORTER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
