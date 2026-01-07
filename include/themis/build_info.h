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

} // namespace build_info
} // namespace themis

#endif // THEMIS_BUILD_INFO_H
