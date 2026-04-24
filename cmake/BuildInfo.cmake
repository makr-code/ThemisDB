# cmake/BuildInfo.cmake
# ──────────────────────────────────────────────────────────────────────────────
# Generates include/updates/build_info.h from include/updates/build_info.h.in
#
# Injected compile-time constants
# ─────────────────────────────────
#   THEMIS_BUILD_CHANNEL   "official" | "community"  (default: community)
#   THEMIS_BUILD_ID        Short Git SHA (7 hex chars)
#   THEMIS_BUILD_TIMESTAMP UTC ISO-8601 configure timestamp
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

# ── User-overridable cache variables ─────────────────────────────────────────

set(THEMIS_BUILD_CHANNEL "community"
    CACHE STRING
    "Build channel: 'official' (signed CI release) or 'community' (self-compiled).")

set(THEMIS_BUILD_SIG ""
    CACHE STRING
    "Base64-encoded Ed25519 signature over the build manifest. \
Set by the CI sign step. Leave empty for community builds.")

# ── Git SHA ───────────────────────────────────────────────────────────────────

find_package(Git QUIET)

if(Git_FOUND OR GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE  _THEMIS_GIT_SHA
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(_THEMIS_GIT_SHA "")
endif()

if("${_THEMIS_GIT_SHA}" STREQUAL "")
    set(THEMIS_BUILD_ID "unknown")
else()
    set(THEMIS_BUILD_ID "${_THEMIS_GIT_SHA}")
endif()

# ── Build timestamp ───────────────────────────────────────────────────────────
# Reproducible-build support: when SOURCE_DATE_EPOCH is set (e.g. in CI or
# Debian/RPM packaging) the timestamp is derived from that Unix epoch value so
# that two builds from the same source tree produce bit-identical binaries.
# See: https://reproducible-builds.org/specs/source-date-epoch/

if(DEFINED ENV{SOURCE_DATE_EPOCH})
    # Convert Unix epoch to an ISO-8601 UTC string using cmake -P scripting
    # cmake's string(TIMESTAMP) does not accept an epoch directly; we use a
    # small trick: configure_file with a generated helper or simply format it.
    set(_sde "$ENV{SOURCE_DATE_EPOCH}")
    # Basic sanity: must be a positive integer
    if(_sde MATCHES "^[0-9]+$")
        # Prefer Python 3 (timezone-aware API, no deprecation warnings)
        find_program(_python_exe NAMES python3 python QUIET)
        if(_python_exe)
            execute_process(
                COMMAND "${_python_exe}" -c
                    "import datetime, sys; ts=int(sys.argv[1]); print(datetime.datetime.fromtimestamp(ts, tz=datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ'))"
                    "${_sde}"
                OUTPUT_VARIABLE _sde_formatted
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE _sde_result
                ERROR_QUIET
            )
        endif()

        # Fallback: POSIX date command (Linux/macOS)
        if(NOT _sde_formatted OR NOT _sde_result EQUAL 0)
            find_program(_date_exe NAMES date QUIET)
            if(_date_exe)
                execute_process(
                    COMMAND "${_date_exe}" -u -d "@${_sde}" "+%Y-%m-%dT%H:%M:%SZ"
                    OUTPUT_VARIABLE _sde_formatted
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE _sde_result
                    ERROR_QUIET
                )
            endif()
        endif()

        if(_sde_formatted AND _sde_result EQUAL 0)
            set(THEMIS_BUILD_TIMESTAMP "${_sde_formatted}")
            message(STATUS "[BuildInfo] SOURCE_DATE_EPOCH=${_sde} → timestamp=${THEMIS_BUILD_TIMESTAMP} (reproducible build)")
        else()
            # Last resort: store the raw epoch value; consumers that parse ISO-8601
            # will reject it, but at least the build is deterministic.
            set(THEMIS_BUILD_TIMESTAMP "1970-01-01T00:00:00Z")
            message(STATUS "[BuildInfo] SOURCE_DATE_EPOCH=${_sde} set but could not format – using epoch zero.")
        endif()
    else()
        message(WARNING "[BuildInfo] SOURCE_DATE_EPOCH='${_sde}' is not a valid Unix epoch. Using current time.")
        string(TIMESTAMP THEMIS_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
    endif()
else()
    string(TIMESTAMP THEMIS_BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)
endif()

# ── Validate channel / signature combination ─────────────────────────────────

if("${THEMIS_BUILD_CHANNEL}" STREQUAL "official" AND "${THEMIS_BUILD_SIG}" STREQUAL "")
    message(WARNING
        "[BuildInfo] THEMIS_BUILD_CHANNEL=official but THEMIS_BUILD_SIG is empty. "
        "Verification will always fail at runtime. "
        "Did you forget to run scripts/sign_build.py and pass -DTHEMIS_BUILD_SIG=…?")
endif()

if(NOT "${THEMIS_BUILD_CHANNEL}" STREQUAL "official" AND
   NOT "${THEMIS_BUILD_CHANNEL}" STREQUAL "community")
    message(WARNING
        "[BuildInfo] Unknown THEMIS_BUILD_CHANNEL '${THEMIS_BUILD_CHANNEL}'. "
        "Defaulting to 'community'.")
    set(THEMIS_BUILD_CHANNEL "community")
endif()

# ── Generate header ───────────────────────────────────────────────────────────

set(_BUILD_INFO_IN  "${CMAKE_SOURCE_DIR}/include/updates/build_info.h.in")
set(_BUILD_INFO_OUT "${CMAKE_BINARY_DIR}/include/updates/build_info.h")

configure_file("${_BUILD_INFO_IN}" "${_BUILD_INFO_OUT}" @ONLY)

# Ensure the generated header is on the include path for all targets that
# include BuildInfo.cmake.  Callers can also add this directory explicitly.
include_directories("${CMAKE_BINARY_DIR}/include")

message(STATUS
    "[BuildInfo] channel=${THEMIS_BUILD_CHANNEL}  "
    "id=${THEMIS_BUILD_ID}  "
    "ts=${THEMIS_BUILD_TIMESTAMP}  "
    "sig=${THEMIS_BUILD_SIG}")
