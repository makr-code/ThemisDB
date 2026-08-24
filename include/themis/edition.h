/**
 * @file edition.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Themis Edition Configuration (v1.3.5+)
 * ========================================
 * Compile-time edition selection and feature gating.
 * This header is automatically configured by CMakeLists.txt
 * during the build process based on -DTHEMIS_EDITION setting.
 *
 * Five editions are supported:
 * - MINIMAL:     Lightweight/embedded (0 GB GPU VRAM cap — CPU fallback enforced, 1 node)
 * - COMMUNITY:   Free, open-source (8 GB GPU VRAM, up to 5 nodes)
 * - ENTERPRISE:  Paid subscription (24 GB GPU VRAM, up to 100 nodes)
 * - MILITARY:    Hardened/air-gapped (16 GB GPU VRAM, up to 50 nodes)
 * - HYPERSCALER: OEM/Custom (unlimited VRAM and nodes)
 *
 * GPU availability: all editions compile with THEMIS_ENABLE_GPU=ON and
 * use a CPU fallback path.  The edition VRAM cap (GPU_MAX_VRAM_GB) governs
 * whether real GPU execution is permitted at runtime: a cap of 0 means
 * GPU execution is disabled and every call is transparently redirected to
 * the CPU fallback (MINIMAL edition).
 */

#pragma once

#include <string_view>
#include <cstdint>

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
    MINIMAL,       // Lightweight/embedded edition (CPU fallback, no GPU execution)
    COMMUNITY,     // Free/open-source edition
    ENTERPRISE,    // Paid subscription edition (100 nodes max)
    MILITARY,      // Hardened/air-gapped edition
    HYPERSCALER,   // OEM/custom edition (unlimited)
    UNKNOWN        // Fallback for unrecognized editions
};

// Get edition type from compile-time string
constexpr EditionType GetEditionType() {
    if (EDITION_STRING == "MINIMAL") {
        return EditionType::MINIMAL;
    } else if (EDITION_STRING == "COMMUNITY") {
        return EditionType::COMMUNITY;
    } else if (EDITION_STRING == "ENTERPRISE") {
        return EditionType::ENTERPRISE;
    } else if (EDITION_STRING == "MILITARY") {
        return EditionType::MILITARY;
    } else if (EDITION_STRING == "HYPERSCALER") {
        return EditionType::HYPERSCALER;
    }
    return EditionType::UNKNOWN;
}

// ============================================================================
// HARDWARE CONSTRAINTS (Edition-specific limits)
// ============================================================================

// GPU Memory constraints (VRAM limit in GB)
// MINIMAL:     0 GB  — no GPU; CPU fallback enforced (IoT/embedded)
// COMMUNITY:   8 GB  — consumer GPUs (e.g. RTX 3070/4060 class)
// ENTERPRISE:  24 GB — professional GPUs (e.g. RTX 4090 / A5000 class)
// MILITARY:    16 GB — controlled/hardened GPU deployments
// HYPERSCALER: 0     — unlimited (OEM, custom deployments)
//
// Default (no CMake edition specified) matches COMMUNITY to stay conservative.
#ifndef THEMIS_GPU_MAX_VRAM_GB
#define THEMIS_GPU_MAX_VRAM_GB 8
#endif
constexpr int GPU_MAX_VRAM_GB = THEMIS_GPU_MAX_VRAM_GB;

// Sharding constraints (maximum number of shard nodes)
// MINIMAL:     1   (single-node only)
// COMMUNITY:   5   (small clusters, HA setups, startups)
// ENTERPRISE: 100  (distributed deployment)
// MILITARY:    50  (controlled, air-gapped clusters)
// HYPERSCALER:  0  — unlimited (massive clustering)
//
// Default (no CMake edition specified) matches MINIMAL to stay conservative.
#ifndef THEMIS_SHARDING_MAX_NODES
#define THEMIS_SHARDING_MAX_NODES 1
#endif
constexpr int SHARDING_MAX_NODES = THEMIS_SHARDING_MAX_NODES;

// ============================================================================
// RESOURCE POLICY CEILINGS (compile-time Defense-in-Depth hard limits)
// ============================================================================
// Convention for "unlimited":
//   - Counts (model instances, connections, jobs): -1
//   - Byte sizes and per-second rates:              0
//
// All values start at unlimited so that deployments without explicit tuning
// experience no artificial constraints.  Edition-aware CMake builds may
// override these via the corresponding THEMIS_* defines.
//
// Policy chain (low to high priority):
//   constexpr ceiling  (this block — never overridable)
//   RuntimeLicenseGate (edition tier)
//   Signed plugin      (fine-tuning within ceiling)
//   Operational config (per-deployment tuning)
//
// Exception: Group 6 (WASM sandbox memory) is a Security-Boundary and MUST
// NOT receive a plugin-policy override layer.  It is enforced separately and
// is not part of the edition-policy plugin contract.

// --- Group 4: LLM resource limits ---
// LLM_MAX_CONTEXT_TOKENS:    maximum prompt+completion tokens per inference.
//   0 = unlimited.
#ifndef THEMIS_LLM_MAX_CONTEXT_TOKENS
#define THEMIS_LLM_MAX_CONTEXT_TOKENS 0
#endif
constexpr int64_t LLM_MAX_CONTEXT_TOKENS = THEMIS_LLM_MAX_CONTEXT_TOKENS;

// LLM_MAX_MODEL_INSTANCES:   maximum concurrently loaded model instances.
//   -1 = unlimited.
#ifndef THEMIS_LLM_MAX_MODEL_INSTANCES
#define THEMIS_LLM_MAX_MODEL_INSTANCES -1
#endif
constexpr int32_t LLM_MAX_MODEL_INSTANCES = THEMIS_LLM_MAX_MODEL_INSTANCES;

// LLM_MAX_VRAM_PER_MODEL_MB: maximum VRAM per model instance in MiB.
//   0 = unlimited.
#ifndef THEMIS_LLM_MAX_VRAM_PER_MODEL_MB
#define THEMIS_LLM_MAX_VRAM_PER_MODEL_MB 0
#endif
constexpr int64_t LLM_MAX_VRAM_PER_MODEL_MB = THEMIS_LLM_MAX_VRAM_PER_MODEL_MB;

// --- Group 1+3: Tenant quota limits ---
// TENANT_MAX_STORAGE_BYTES:  maximum storage per tenant in bytes.  0 = unlimited.
#ifndef THEMIS_TENANT_MAX_STORAGE_BYTES
#define THEMIS_TENANT_MAX_STORAGE_BYTES 0ULL
#endif
constexpr uint64_t TENANT_MAX_STORAGE_BYTES = THEMIS_TENANT_MAX_STORAGE_BYTES;

// TENANT_MAX_DOCUMENTS:      maximum document count per tenant.  0 = unlimited.
#ifndef THEMIS_TENANT_MAX_DOCUMENTS
#define THEMIS_TENANT_MAX_DOCUMENTS 0ULL
#endif
constexpr uint64_t TENANT_MAX_DOCUMENTS = THEMIS_TENANT_MAX_DOCUMENTS;

// TENANT_MAX_COLLECTIONS:    maximum collection count per tenant.  0 = unlimited.
#ifndef THEMIS_TENANT_MAX_COLLECTIONS
#define THEMIS_TENANT_MAX_COLLECTIONS 0U
#endif
constexpr uint32_t TENANT_MAX_COLLECTIONS = THEMIS_TENANT_MAX_COLLECTIONS;

// TENANT_MAX_CONCURRENT_QUERIES: maximum concurrent queries per tenant.  0 = unlimited.
#ifndef THEMIS_TENANT_MAX_CONCURRENT_QUERIES
#define THEMIS_TENANT_MAX_CONCURRENT_QUERIES 0U
#endif
constexpr uint32_t TENANT_MAX_CONCURRENT_QUERIES = THEMIS_TENANT_MAX_CONCURRENT_QUERIES;

// TENANT_MAX_REQUESTS_PER_SECOND: maximum request rate per tenant.  0 = unlimited.
#ifndef THEMIS_TENANT_MAX_REQUESTS_PER_SECOND
#define THEMIS_TENANT_MAX_REQUESTS_PER_SECOND 0U
#endif
constexpr uint32_t TENANT_MAX_REQUESTS_PER_SECOND = THEMIS_TENANT_MAX_REQUESTS_PER_SECOND;

// --- Group 2: Query limit ceilings ---
// QUERY_MAX_GRAPHQL_DEPTH:     maximum GraphQL/AQL nesting depth.  0 = unlimited.
#ifndef THEMIS_QUERY_MAX_GRAPHQL_DEPTH
#define THEMIS_QUERY_MAX_GRAPHQL_DEPTH 0U
#endif
constexpr uint32_t QUERY_MAX_GRAPHQL_DEPTH = THEMIS_QUERY_MAX_GRAPHQL_DEPTH;

// QUERY_MAX_GRAPHQL_COMPLEXITY: maximum GraphQL complexity score.  0 = unlimited.
#ifndef THEMIS_QUERY_MAX_GRAPHQL_COMPLEXITY
#define THEMIS_QUERY_MAX_GRAPHQL_COMPLEXITY 0U
#endif
constexpr uint32_t QUERY_MAX_GRAPHQL_COMPLEXITY = THEMIS_QUERY_MAX_GRAPHQL_COMPLEXITY;

// QUERY_MAX_PAYLOAD_BYTES:      maximum request payload in bytes.  0 = unlimited.
#ifndef THEMIS_QUERY_MAX_PAYLOAD_BYTES
#define THEMIS_QUERY_MAX_PAYLOAD_BYTES 0ULL
#endif
constexpr uint64_t QUERY_MAX_PAYLOAD_BYTES = THEMIS_QUERY_MAX_PAYLOAD_BYTES;

// QUERY_MAX_RESULT_ROWS:        maximum rows returned per query.  0 = unlimited.
#ifndef THEMIS_QUERY_MAX_RESULT_ROWS
#define THEMIS_QUERY_MAX_RESULT_ROWS 0ULL
#endif
constexpr uint64_t QUERY_MAX_RESULT_ROWS = THEMIS_QUERY_MAX_RESULT_ROWS;

// --- Group 2+3: Connection policy ceilings ---
// CONNECTION_MAX_HTTP2_STREAMS:    max concurrent HTTP/2 streams per connection.  0 = unlimited.
#ifndef THEMIS_CONNECTION_MAX_HTTP2_STREAMS
#define THEMIS_CONNECTION_MAX_HTTP2_STREAMS 0U
#endif
constexpr uint32_t CONNECTION_MAX_HTTP2_STREAMS = THEMIS_CONNECTION_MAX_HTTP2_STREAMS;

// CONNECTION_MAX_SSE_CONNECTIONS:  max total SSE connections.  0 = unlimited.
#ifndef THEMIS_CONNECTION_MAX_SSE_CONNECTIONS
#define THEMIS_CONNECTION_MAX_SSE_CONNECTIONS 0U
#endif
constexpr uint32_t CONNECTION_MAX_SSE_CONNECTIONS = THEMIS_CONNECTION_MAX_SSE_CONNECTIONS;

// CONNECTION_MAX_TOTAL:            max total simultaneous server connections.  0 = unlimited.
#ifndef THEMIS_CONNECTION_MAX_TOTAL
#define THEMIS_CONNECTION_MAX_TOTAL 0U
#endif
constexpr uint32_t CONNECTION_MAX_TOTAL = THEMIS_CONNECTION_MAX_TOTAL;

// CONNECTION_MAX_SSE_EVENTS_PER_SEC: max SSE events emitted per second.  0 = unlimited.
#ifndef THEMIS_CONNECTION_MAX_SSE_EVENTS_PER_SEC
#define THEMIS_CONNECTION_MAX_SSE_EVENTS_PER_SEC 0U
#endif
constexpr uint32_t CONNECTION_MAX_SSE_EVENTS_PER_SEC = THEMIS_CONNECTION_MAX_SSE_EVENTS_PER_SEC;

// --- Group 5: Storage operations ceilings ---
// STORAGE_MAX_BACKGROUND_JOBS:       max concurrent background storage jobs.  -1 = unlimited.
#ifndef THEMIS_STORAGE_MAX_BACKGROUND_JOBS
#define THEMIS_STORAGE_MAX_BACKGROUND_JOBS -1
#endif
constexpr int32_t STORAGE_MAX_BACKGROUND_JOBS = THEMIS_STORAGE_MAX_BACKGROUND_JOBS;

// STORAGE_MAX_COMPACTION_BYTES_PER_SEC: max compaction I/O rate in bytes/s.  0 = unlimited.
#ifndef THEMIS_STORAGE_MAX_COMPACTION_BYTES_PER_SEC
#define THEMIS_STORAGE_MAX_COMPACTION_BYTES_PER_SEC 0ULL
#endif
constexpr uint64_t STORAGE_MAX_COMPACTION_BYTES_PER_SEC = THEMIS_STORAGE_MAX_COMPACTION_BYTES_PER_SEC;

// STORAGE_MAX_CONCURRENT_SNAPSHOTS:  max snapshots running in parallel.  -1 = unlimited.
#ifndef THEMIS_STORAGE_MAX_CONCURRENT_SNAPSHOTS
#define THEMIS_STORAGE_MAX_CONCURRENT_SNAPSHOTS -1
#endif
constexpr int32_t STORAGE_MAX_CONCURRENT_SNAPSHOTS = THEMIS_STORAGE_MAX_CONCURRENT_SNAPSHOTS;

// --- Group 3: Global rate-limit ceiling ---
// RATE_LIMIT_MAX_GLOBAL_RPS: max total requests per second across all tenants.  0 = unlimited.
#ifndef THEMIS_RATE_LIMIT_MAX_GLOBAL_RPS
#define THEMIS_RATE_LIMIT_MAX_GLOBAL_RPS 0ULL
#endif
constexpr uint64_t RATE_LIMIT_MAX_GLOBAL_RPS = THEMIS_RATE_LIMIT_MAX_GLOBAL_RPS;

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
// GATED FEATURE NAMES (single source of truth)
// ============================================================================
// All Enterprise/Hyperscaler-only feature names that are subject to runtime
// license gating.  Any consumer that needs to enumerate or iterate over gated
// features should reference this array rather than duplicating the list.
//
// Note: std::array<std::string_view, N> is not constexpr-initializable until
// C++20 in all major compilers, so we use a plain C array.

static constexpr std::string_view kGatedFeatureNames[] = {
    "enterprise_plugins",
    "multi_master",
    "field_encryption",
    "rbac",
    "hsm",
};

static constexpr std::size_t kGatedFeatureCount =
    sizeof(kGatedFeatureNames) / sizeof(kGatedFeatureNames[0]);

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
