/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            build_info.h                                       ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     176                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * ThemisDB Build Information
 * ==========================
 * Captures compile-time configuration and module availability.
 * This header provides runtime access to CMake build flags and edition settings.
 */

#ifndef THEMIS_BUILD_INFO_H
#define THEMIS_BUILD_INFO_H

#include <string>
#include <vector>
#include <map>
#include <utility>
#include "themis/edition.h"

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

/**
 * Get complete build configuration including edition and modules
 */
BuildConfiguration getBuildConfiguration();

/**
 * Format build configuration as a human-readable string
 * suitable for logging at server startup
 */
std::string formatBuildInfo(const BuildConfiguration& config);

/**
 * Get a compact summary of key build information for version endpoint
 */
std::string getVersionSummary();

// ============================================================================
// MODULE STATUS QUERIES
// ============================================================================

/**
 * Check if a specific module was compiled into the binary
 */
bool isModuleCompiledIn(const std::string& module_name);

/**
 * Get list of all modules compiled into this binary
 */
std::vector<std::string> getCompiledModules();

/**
 * Get list of all supported but not compiled modules
 */
std::vector<std::string> getDisabledModules();

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
ReproducibilityInfo getReproducibilityInfo();

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
bool exportBuildManifest(const std::string& output_path);

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
bool verifyBuildManifest(const std::string& manifest_path);

} // namespace build_info
} // namespace themis

#endif // THEMIS_BUILD_INFO_H
