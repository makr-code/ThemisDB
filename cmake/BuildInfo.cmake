# cmake/BuildInfo.cmake
# ──────────────────────────────────────────────────────────────────────────────
# Generates include/updates/build_info.h from include/updates/build_info.h.in
#
# Injected compile-time constants
# ─────────────────────────────────
#   THEMIS_BUILD_CHANNEL   "official" | "community"  (default: community)
#   THEMIS_BUILD_ID        Short Git SHA (7 hex chars)
#   THEMIS_BUILD_TIMESTAMP UTC ISO-8601 build timestamp
#   THEMIS_BUILD_SIG       Ed25519 signature (Base64), empty for community builds
#   THEMIS_BUILD_PUBKEY    Ed25519 public key (Base64, hard-coded in .h.in)
#
# How the official channel is activated
# ──────────────────────────────────────
# The CI release workflow injects the following cmake cache variable:
#   cmake … -DTHEMIS_BUILD_CHANNEL=official
#            -DTHEMIS_BUILD_SIG=<base64-sig>
# The private Ed25519 key is stored as a GitHub Actions encrypted secret and
# never appears in the source tree.
#
# Usage (from CMakeLists.txt)
#   include(BuildInfo)          # after project() and version setup
# ──────────────────────────────────────────────────────────────────────────────

cmake_minimum_required(VERSION 3.19)

get_filename_component(_THEMIS_BUILDINFO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# ── User-overridable cache variables ─────────────────────────────────────────

set(THEMIS_BUILD_CHANNEL "community"
    CACHE STRING
    "Build channel: 'official' (signed CI release) or 'community' (self-compiled).")

set(THEMIS_BUILD_SIG ""
    CACHE STRING
    "Base64-encoded Ed25519 signature over the build manifest. \
Set by the CI sign step. Leave empty for community builds.")

option(THEMIS_REQUIRE_REPRODUCIBLE_BUILD
    "Fail configure when BuildInfo.cmake cannot derive deterministic build metadata."
    OFF)

# ── Git SHA ───────────────────────────────────────────────────────────────────

find_package(Git QUIET)

if(Git_FOUND OR GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${_THEMIS_BUILDINFO_ROOT}"
        OUTPUT_VARIABLE  _THEMIS_GIT_SHA
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Use %ct (Unix epoch integer) instead of %cI (ISO 8601 with local timezone offset).
    # %cI preserves the committer's stored TZ offset (e.g. +05:30), producing a byte string
    # that differs from the SOURCE_DATE_EPOCH path's "Z"-format for the same instant.
    # By capturing the raw epoch we can re-format through string(TIMESTAMP) below, which
    # guarantees a consistent UTC Z-format regardless of committer timezone or git version.
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" log -1 --format=%ct
        WORKING_DIRECTORY "${_THEMIS_BUILDINFO_ROOT}"
        OUTPUT_VARIABLE  _THEMIS_GIT_COMMIT_EPOCH
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(_THEMIS_GIT_SHA "")
    set(_THEMIS_GIT_COMMIT_EPOCH "")
endif()

if("${_THEMIS_GIT_SHA}" STREQUAL "")
    set(THEMIS_BUILD_ID "unknown")
else()
    set(THEMIS_BUILD_ID "${_THEMIS_GIT_SHA}")
endif()

# ── Build timestamp ───────────────────────────────────────────────────────────
#
# SOURCE_DATE_EPOCH is the canonical reproducible-builds input used by Debian,
# F-Droid and the wider reproducible-builds ecosystem. CMake's
# string(TIMESTAMP) honours it automatically when the environment variable is
# present, so we only need to validate it explicitly here.

set(_THEMIS_SOURCE_DATE_EPOCH "$ENV{SOURCE_DATE_EPOCH}")
set(_THEMIS_BUILD_TIMESTAMP_SOURCE "")

if(NOT "${_THEMIS_SOURCE_DATE_EPOCH}" STREQUAL "")
    if(NOT _THEMIS_SOURCE_DATE_EPOCH MATCHES "^[0-9]+$")
        message(FATAL_ERROR
            "[BuildInfo] SOURCE_DATE_EPOCH='${_THEMIS_SOURCE_DATE_EPOCH}' is invalid. "
            "Expected an integer Unix timestamp.")
    endif()

    string(TIMESTAMP THEMIS_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
    set(_THEMIS_BUILD_TIMESTAMP_SOURCE "SOURCE_DATE_EPOCH=${_THEMIS_SOURCE_DATE_EPOCH}")
elseif(NOT "${_THEMIS_GIT_COMMIT_EPOCH}" STREQUAL "")
    # Temporarily expose the git commit epoch as SOURCE_DATE_EPOCH so that
    # string(TIMESTAMP) produces the same UTC Z-format as the explicit
    # SOURCE_DATE_EPOCH path above.  This keeps the signing manifest byte-identical
    # across contributors irrespective of their git version or commit timezone.
    set(ENV{SOURCE_DATE_EPOCH} "${_THEMIS_GIT_COMMIT_EPOCH}")
    string(TIMESTAMP THEMIS_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
    unset(ENV{SOURCE_DATE_EPOCH})
    set(_THEMIS_BUILD_TIMESTAMP_SOURCE "git-commit-date")
elseif(THEMIS_REQUIRE_REPRODUCIBLE_BUILD)
    message(FATAL_ERROR
        "[BuildInfo] Reproducible build required, but neither SOURCE_DATE_EPOCH "
        "nor a Git commit timestamp is available.")
else()
    message(WARNING
        "[BuildInfo] Falling back to configure-time timestamp because neither "
        "SOURCE_DATE_EPOCH nor git commit metadata is available. "
        "This build is not reproducible.")
    string(TIMESTAMP THEMIS_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
    set(_THEMIS_BUILD_TIMESTAMP_SOURCE "configure-time")
endif()

# ── Validate channel / signature combination ─────────────────────────────────

if(NOT "${THEMIS_BUILD_CHANNEL}" STREQUAL "official" AND
   NOT "${THEMIS_BUILD_CHANNEL}" STREQUAL "community")
    message(WARNING
        "[BuildInfo] Unknown THEMIS_BUILD_CHANNEL '${THEMIS_BUILD_CHANNEL}'. "
        "Defaulting to 'community'.")
    set(THEMIS_BUILD_CHANNEL "community")
endif()

if("${THEMIS_BUILD_CHANNEL}" STREQUAL "official")
    if("${THEMIS_BUILD_SIG}" STREQUAL "")
        message(FATAL_ERROR
            "[BuildInfo] THEMIS_BUILD_CHANNEL=official requires THEMIS_BUILD_SIG. "
            "Signing is owner-controlled and handled via private signing tooling.")
    endif()

    set(_THEMIS_BUILD_VERSION_FOR_SIG "${PROJECT_VERSION}")
    if(DEFINED _ver AND NOT "${_ver}" STREQUAL "")
        set(_THEMIS_BUILD_VERSION_FOR_SIG "${_ver}")
    endif()
    set(_THEMIS_BUILD_MANIFEST
        "${THEMIS_BUILD_CHANNEL}|${_THEMIS_BUILD_VERSION_FOR_SIG}|${THEMIS_BUILD_ID}|${THEMIS_BUILD_TIMESTAMP}")

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(NOT Python3_Interpreter_FOUND)
        message(FATAL_ERROR
            "[BuildInfo] Python3 interpreter is required to verify official build signatures.")
    endif()

    execute_process(
        COMMAND "${Python3_EXECUTABLE}"
                "${_THEMIS_BUILDINFO_ROOT}/cmake/verify_ed25519_signature.py"
                --pubkey-b64 "11qYAYKxCrfVS/7TyWQHOg7hcvPapiMlrwIaaPcHURo="
                --signature-b64 "${THEMIS_BUILD_SIG}"
                --message "${_THEMIS_BUILD_MANIFEST}"
        RESULT_VARIABLE _THEMIS_BUILD_SIG_VERIFY_RC
        OUTPUT_VARIABLE _THEMIS_BUILD_SIG_VERIFY_STDOUT
        ERROR_VARIABLE _THEMIS_BUILD_SIG_VERIFY_STDERR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE
    )
    if(NOT _THEMIS_BUILD_SIG_VERIFY_RC EQUAL 0)
        message(FATAL_ERROR
            "[BuildInfo] Official build signature verification failed. "
            "Only owner-issued signatures are accepted.\n"
            "Manifest: ${_THEMIS_BUILD_MANIFEST}\n"
            "Details: ${_THEMIS_BUILD_SIG_VERIFY_STDERR}")
    endif()
endif()

# ── Generate header ───────────────────────────────────────────────────────────

set(_BUILD_INFO_IN  "${_THEMIS_BUILDINFO_ROOT}/include/updates/build_info.h.in")
set(_BUILD_INFO_OUT "${CMAKE_BINARY_DIR}/include/updates/build_info.h")

configure_file("${_BUILD_INFO_IN}" "${_BUILD_INFO_OUT}" @ONLY)

# Ensure the generated header is on the include path for all targets that
# include BuildInfo.cmake.  Callers can also add this directory explicitly.
include_directories("${CMAKE_BINARY_DIR}/include")

message(STATUS
    "[BuildInfo] channel=${THEMIS_BUILD_CHANNEL}  "
    "id=${THEMIS_BUILD_ID}  "
    "ts=${THEMIS_BUILD_TIMESTAMP}  "
    "ts_source=${_THEMIS_BUILD_TIMESTAMP_SOURCE}  "
    "sig=${THEMIS_BUILD_SIG}")
