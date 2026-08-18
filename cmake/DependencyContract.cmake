# ThemisDB canonical dependency contract
#
# Rule: normal dev + release builds are strict-by-default. Only the explicit
# diagnostic bootstrap path may allow runtime fallback behavior.
#
# Core packages are required for a valid ThemisDB build. Feature packages are
# optional and may be disabled when the selected edition or runtime feature set
# does not require them.

set(THEMIS_REQUIRED_CORE_SUBMODULES
    vcpkg
    ffmpeg
    llama.cpp
    whisper.cpp
    stable-diffusion.cpp
)

set(THEMIS_REQUIRED_CORE_PACKAGES
    fmt
    spdlog
    nlohmann_json
    ZLIB
    RocksDB
)

set(THEMIS_OPTIONAL_FEATURE_PACKAGES
    OpenSSL
    Boost
    simdjson
    TBB
    Protobuf
    gRPC
    CURL
    prometheus-cpp
    benchmark
    GTest
    msgpack
    Kerberos
)

if(THEMIS_DIAGNOSTIC_MODE)
    message(STATUS "ThemisDB dependency contract: DIAGNOSTIC mode enabled. Local fallbacks are allowed only for bootstrap troubleshooting.")
else()
    message(STATUS "ThemisDB dependency contract: STRICT mode enabled. Core dependencies must resolve without silent fallback.")
endif()

# Diagnostic mode is the only way to tolerate missing core dependencies during
# early diagnosis. Regular developer and release presets must keep this OFF.
if(DEFINED THEMIS_ALLOW_MISSING_ROCKSDB AND THEMIS_ALLOW_MISSING_ROCKSDB AND NOT THEMIS_DIAGNOSTIC_MODE)
    message(FATAL_ERROR
        "THEMIS_ALLOW_MISSING_ROCKSDB=ON is only allowed in diagnostic/bootstrap mode. "
        "Enable THEMIS_DIAGNOSTIC_MODE=ON for troubleshooting; otherwise configure must fail."
    )
endif()
