/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            edition.h                                          ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-02-21 14:17:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     220                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * Themis Edition Configuration (v1.3.5+)
 * ========================================
 * Compile-time edition selection and feature gating.
 * This header is automatically configured by CMakeLists.txt
 * during the build process based on -DTHEMIS_EDITION setting.
 *
 * Three editions are supported:
 * - COMMUNITY: Free, open-source (24GB GPU VRAM, single-node only)
 * - ENTERPRISE: Paid subscription (256GB GPU VRAM, up to 100 nodes)
 * - HYPERSCALER: OEM/Custom (unlimited VRAM and nodes)
 */

#ifndef THEMIS_EDITION_H
#define THEMIS_EDITION_H

#include <string_view>

namespace themis {
namespace edition {

// ============================================================================
// COMPILE-TIME EDITION IDENTIFIER
// ============================================================================
// These values are set by CMakeLists.txt at build time and embedded into
// the executable as compile-time constants.

// Provide default values if not defined by CMake
#ifndef THEMIS_EDITION_STRING
#define THEMIS_EDITION_STRING "COMMUNITY"
#endif

constexpr std::string_view EDITION_STRING = THEMIS_EDITION_STRING;

// Edition type enumeration
enum class EditionType {
    COMMUNITY,     // Free/open-source edition
    ENTERPRISE,    // Paid subscription edition (100 nodes max)
    HYPERSCALER,   // OEM/custom edition (unlimited)
    UNKNOWN        // Fallback for unrecognized editions
};

// Get edition type from compile-time string
constexpr EditionType GetEditionType() {
    if (EDITION_STRING == "COMMUNITY") {
        return EditionType::COMMUNITY;
    } else if (EDITION_STRING == "ENTERPRISE") {
        return EditionType::ENTERPRISE;
    } else if (EDITION_STRING == "HYPERSCALER") {
        return EditionType::HYPERSCALER;
    }
    return EditionType::UNKNOWN;
}

// ============================================================================
// HARDWARE CONSTRAINTS (Edition-specific limits)
// ============================================================================

// GPU Memory constraints (VRAM limit in GB)
// COMMUNITY: 24 GB   (consumer-grade GPU like RTX 4090)
// ENTERPRISE: 256 GB (data center GPU like A100/H100)
// HYPERSCALER: Unlimited (custom, OEM deployments)
#ifndef THEMIS_GPU_MAX_VRAM_GB
#define THEMIS_GPU_MAX_VRAM_GB 24
#endif
constexpr int GPU_MAX_VRAM_GB = THEMIS_GPU_MAX_VRAM_GB;

// Sharding constraints (maximum number of shard nodes)
// COMMUNITY: 5      (small clusters, HA setups, startups)
// ENTERPRISE: 100   (distributed deployment)
// HYPERSCALER: Unlimited (massive clustering)
#ifndef THEMIS_SHARDING_MAX_NODES
#define THEMIS_SHARDING_MAX_NODES 1
#endif
constexpr int SHARDING_MAX_NODES = THEMIS_SHARDING_MAX_NODES;

// ============================================================================
// FEATURE FLAGS (Compile-time feature availability)
// ============================================================================
// These are automatically set to ON/OFF based on THEMIS_EDITION.
// At runtime, code should check these constants for feature access.

// Enterprise Plugin System: Allows loading custom plugins and extensions
// COMMUNITY: Disabled (plugin loading will fail with helpful message)
// ENTERPRISE: Enabled
// HYPERSCALER: Enabled
#ifdef THEMIS_ENABLE_ENTERPRISE_PLUGINS
constexpr bool FEATURE_ENTERPRISE_PLUGINS = THEMIS_ENABLE_ENTERPRISE_PLUGINS;
#else
constexpr bool FEATURE_ENTERPRISE_PLUGINS = false;
#endif

// Multi-Master Replication: Active-active replication across nodes
// COMMUNITY: Disabled (single-node only)
// ENTERPRISE: Enabled
// HYPERSCALER: Enabled
#ifdef THEMIS_ENABLE_MULTI_MASTER
constexpr bool FEATURE_MULTI_MASTER = THEMIS_ENABLE_MULTI_MASTER;
#else
constexpr bool FEATURE_MULTI_MASTER = false;
#endif

// Field-Level Encryption: Encrypt specific columns at rest
// COMMUNITY: Disabled (basic TLS only)
// ENTERPRISE: Enabled
// HYPERSCALER: Enabled
#ifdef THEMIS_ENABLE_FIELD_ENCRYPTION
constexpr bool FEATURE_FIELD_ENCRYPTION = THEMIS_ENABLE_FIELD_ENCRYPTION;
#else
constexpr bool FEATURE_FIELD_ENCRYPTION = false;
#endif

// Role-Based Access Control (RBAC): Fine-grained permission management
// COMMUNITY: Disabled (basic auth only)
// ENTERPRISE: Enabled
// HYPERSCALER: Enabled
#ifdef THEMIS_ENABLE_RBAC
constexpr bool FEATURE_RBAC = THEMIS_ENABLE_RBAC;
#else
constexpr bool FEATURE_RBAC = false;
#endif

// Hardware Security Module (HSM) Integration: PKCS#11 support
// COMMUNITY: Disabled (software key storage only)
// ENTERPRISE: Enabled
// HYPERSCALER: Enabled
#ifdef THEMIS_ENABLE_HSM
constexpr bool FEATURE_HSM = THEMIS_ENABLE_HSM;
#else
constexpr bool FEATURE_HSM = false;
#endif

// ============================================================================
// UTILITY FUNCTIONS FOR RUNTIME CHECKS
// ============================================================================

// Check if this build is the specified edition at compile-time
template<EditionType T>
constexpr bool IsEdition() {
    return GetEditionType() == T;
}

// Check if feature is enabled for this edition
constexpr bool IsFeatureEnabled(std::string_view feature_name) {
    if (feature_name == "enterprise_plugins") {
        return FEATURE_ENTERPRISE_PLUGINS;
    } else if (feature_name == "multi_master") {
        return FEATURE_MULTI_MASTER;
    } else if (feature_name == "field_encryption") {
        return FEATURE_FIELD_ENCRYPTION;
    } else if (feature_name == "rbac") {
        return FEATURE_RBAC;
    } else if (feature_name == "hsm") {
        return FEATURE_HSM;
    }
    return false;
}

// ============================================================================
// RUNTIME EDITION INFORMATION
// ============================================================================

// Helper struct for runtime edition information
struct EditionInfo {
    EditionType type;
    std::string_view name;
    int gpu_max_vram_gb;
    int sharding_max_nodes;
    bool supports_plugins;
    bool supports_multi_master;
    bool supports_field_encryption;
    bool supports_rbac;
    bool supports_hsm;

    static constexpr EditionInfo Get() {
        const auto edition = GetEditionType();
        return EditionInfo{
            .type = edition,
            .name = EDITION_STRING,
            .gpu_max_vram_gb = GPU_MAX_VRAM_GB,
            .sharding_max_nodes = SHARDING_MAX_NODES,
            .supports_plugins = FEATURE_ENTERPRISE_PLUGINS,
            .supports_multi_master = FEATURE_MULTI_MASTER,
            .supports_field_encryption = FEATURE_FIELD_ENCRYPTION,
            .supports_rbac = FEATURE_RBAC,
            .supports_hsm = FEATURE_HSM
        };
    }
};

} // namespace edition
} // namespace themis

#endif // THEMIS_EDITION_H
