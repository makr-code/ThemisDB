/**
 * @file build_info.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB Build Information
 * ==========================
 * Captures compile-time configuration and module availability.
 * This header provides runtime access to CMake build flags and edition settings.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <functional>
#include "themis/edition.h"
#include "themis/export.h"

namespace themis {
namespace build_info {

// ============================================================================
// MODULE BUILD STATUS
// ============================================================================

struct ModuleInfo {
    std::string name;
    bool compiled_in;     // Was this module compiled into the binary?
    bool runtime_enabled; // Is this module enabled via config?
    std::string description;
};

struct BuildConfiguration {
    // Edition information
    std::string edition_name;
    std::string edition_type;
    int gpu_max_vram_gb;
    int sharding_max_nodes;
    
    // Compiler information
    std::string compiler;
    std::string compiler_version;
    std::string build_type;
    std::string build_timestamp;
    
    // Module compilation status
    std::vector<ModuleInfo> modules;
    
    // Feature flags
    std::vector<std::pair<std::string, bool>> compile_flags;
};

// ============================================================================
// BUILD INFORMATION COLLECTION
// ============================================================================
//
// Windows DLL / STL ABI note:
//   The APIs below pass and return STL types (std::string, std::vector,
//   std::optional, std::map) across the DLL boundary.  This is safe only
//   when the consumer and the DLL are compiled with the *same* MSVC toolset
//   version and the same CRT linkage (default: /MD, MultiThreadedDLL).
//   Mixing toolset versions or CRT flavours (/MT vs /MD) will cause crashes
//   or heap corruption at runtime.  Third-party consumers must therefore
//   build against the same Visual Studio version that produced
//   themis_base.dll.  A stable C ABI wrapper is planned for v2.0.
//
/**
 * Get complete build configuration including edition and modules
 */
THEMIS_BASE_API BuildConfiguration getBuildConfiguration();

/**
 * Format build configuration as a human-readable string
 * suitable for logging at server startup
 */
THEMIS_BASE_API std::string formatBuildInfo(const BuildConfiguration& config);

/**
 * Get a compact summary of key build information for version endpoint
 */
THEMIS_BASE_API std::string getVersionSummary();

// ============================================================================
// MODULE STATUS QUERIES
// ============================================================================

/**
 * Check if a specific module was compiled into the binary
 */
THEMIS_BASE_API bool isModuleCompiledIn(const std::string& module_name);

/**
 * Get list of all modules compiled into this binary
 */
THEMIS_BASE_API std::vector<std::string> getCompiledModules();

/**
 * Get list of all supported but not compiled modules
 */
THEMIS_BASE_API std::vector<std::string> getDisabledModules();

// ============================================================================
// BUILD REPRODUCIBILITY (Phase 1 - v1.7.0)
// ============================================================================

/**
 * @brief Reproducibility information for build auditing and CI verification
 *
 * Captures the exact source-code revision, toolchain version, and key
 * dependency hashes so that a binary can be unambiguously traced back to
 * a specific source snapshot.
 */
struct ReproducibilityInfo {
    std::string git_commit;        ///< Full SHA-1 of HEAD at build time
    std::string git_commit_date;   ///< ISO-8601 author date of HEAD
    std::string git_branch;        ///< Branch name (or "(detached)")
    bool        git_dirty = false; ///< True if working tree had uncommitted changes
    std::string build_host;        ///< Hostname of the build machine
    std::string build_user;        ///< OS username that invoked the build
    std::string toolchain;         ///< "compiler-id/version" e.g. "GCC/13.2.0"
    std::map<std::string, std::string> dependencies; ///< dep-name → version/hash
    std::string binary_hash;       ///< SHA-256 of the current executable (filled at runtime)
};

/**
 * @brief Collect reproducibility metadata embedded at compile time.
 *
 * cmake/CMakeLists.txt captures git HEAD, branch, dirty flag, and build-host
 * via `execute_process()` and injects them as compile definitions
 * (THEMIS_GIT_COMMIT, THEMIS_GIT_BRANCH, THEMIS_GIT_DIRTY,
 *  THEMIS_BUILD_HOST, THEMIS_BUILD_USER).  Those definitions are read here.
 */
THEMIS_BASE_API ReproducibilityInfo getReproducibilityInfo();

/**
 * @brief Write a JSON build-manifest to @p output_path.
 *
 * The manifest contains all fields of ReproducibilityInfo plus the full
 * BuildConfiguration.  It is suitable for archiving alongside a release
 * binary and for automated CI reproducibility checks.
 *
 * @param output_path  Destination file path (will be created or overwritten).
 * @return true on success, false on I/O error.
 */
THEMIS_BASE_API bool exportBuildManifest(const std::string& output_path);

/**
 * @brief Verify that the build manifest at @p manifest_path matches the
 *        current binary's embedded build metadata.
 *
 * Compares git_commit, toolchain, and key compile flags.  Useful in CI to
 * assert that a binary was built from the expected commit.
 *
 * @param manifest_path  Path to a previously generated manifest JSON file.
 * @return true if all compared fields match, false otherwise.
 */
THEMIS_BASE_API bool verifyBuildManifest(const std::string& manifest_path);

// ============================================================================
// HSM MODULE STATUS BRIDGE (STUB #95)
// ============================================================================
//
// Allows the server startup code to inject the actual runtime HSM state
// into the build-info module report.  When the bridge is set, the HSM
// PKCS#11 module entry in getBuildConfiguration() reflects the live state
// of the HSM provider (stub vs. hardware-backed) instead of the static
// compile-time default.
//
// The bridge is only consulted in builds WITHOUT THEMIS_ENABLE_HSM_REAL.
// In THEMIS_ENABLE_HSM_REAL builds the real HSM is compiled in and always
// reported as compiled_in=true.
//
// Usage (server startup):
//   themis::build_info::setHsmModuleStatusFn([]() {
//       bool real = !hsm->isStubProvider();
//       std::string desc = real ? "HSM PKCS#11 (hardware-backed)"
//                               : "HSM PKCS#11 (software stub – dev only)";
//       return std::make_pair(real, desc);
//   });
//
// Usage (tests / teardown):
//   themis::build_info::clearHsmModuleStatusFn();  // restore default

/// Callable that returns {is_real_hsm_active, description}.
using HsmModuleStatusFn = std::function<std::pair<bool, std::string>()>;

/// Register a bridge that reports runtime HSM module status.
/// Thread-safe.  Pass an empty function to restore static defaults.
THEMIS_BASE_API void setHsmModuleStatusFn(HsmModuleStatusFn fn);

/// Remove any previously registered HSM module status bridge.
/// Thread-safe.
THEMIS_BASE_API void clearHsmModuleStatusFn();

} // namespace build_info
} // namespace themis
