/*
 * ThemisDB | File: plugins_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen plugins module contract semantics for the active v1.x major line.
 */

/**
 * @file plugins_api_contract.h
 * @brief Frozen plugin lifecycle, security, and monitoring contract for v1.x.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Defines the normative contract for the plugins module covering plugin
 * lifecycle management (load/unload/hot-plug), manifest and signature
 * validation, capability negotiation, health monitoring, and OCI/RPC
 * integration surfaces.
 *
 * ## §API Contracts
 *
 * Key behavioural invariants:
 *   1. A plugin MUST NOT be started if its manifest fails schema validation or
 *      its signature cannot be verified; result: kManifestInvalid or
 *      kSignatureVerifyFailed.
 *   2. Capability escalation after plugin load is forbidden; capability set is
 *      frozen at registration time (kCapabilityDenied on attempted escalation).
 *   3. Hot-plug transitions are atomic: partial loads that fail mid-way result
 *      in a full rollback, leaving the registry unchanged.
 *   4. Health-monitor failures are never silently swallowed; they produce
 *      kHealthCheckFailed and trigger the configured remediation policy.
 *   5. Registry conflicts (duplicate plugin ID) are rejected with
 *      kRegistryConflict; callers must unload the existing plugin first.
 *
 * ## §Error Taxonomy
 *
 * | Code  | Constant                | Meaning                                          |
 * |-------|-------------------------|--------------------------------------------------|
 * | 0     | kSuccess                | Operation completed without error                |
 * | 8200  | kPluginNotFound         | Plugin ID not present in registry                |
 * | 8201  | kManifestInvalid        | Plugin manifest fails schema or semantic checks  |
 * | 8202  | kSignatureVerifyFailed  | Cryptographic signature check failed             |
 * | 8203  | kLifecycleTransition    | Invalid lifecycle state transition attempted     |
 * | 8204  | kCapabilityDenied       | Requested capability not granted to plugin       |
 * | 8205  | kRegistryConflict       | Duplicate plugin ID detected on registration     |
 * | 8206  | kHealthCheckFailed      | Plugin health check returned unhealthy status    |
 * | 8207  | kInternalError          | Unclassified internal error; always deny         |
 *
 * ## §Threading Guarantees
 *
 * - PluginRegistry reads are lock-free after startup registration phase.
 * - Hot-plug operations acquire an exclusive registry write-lock; concurrent
 *   hot-plug requests are serialized.
 * - Health-monitor callbacks are delivered on a dedicated thread; they MUST NOT
 *   re-enter the registry.
 *
 * ## §Contract Freeze
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/plugins/ROADMAP.md — Phase 1 item
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace themis {
namespace plugins {

// ============================================================================
// § 1  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the plugins module.
 *
 * All plugin operations return or throw with one of these codes.
 * Values are in the reserved range [8200, 8299].
 */
enum class PluginsError : int32_t {
    kSuccess               = 0,
    kPluginNotFound        = 8200, ///< Plugin ID not present in registry.
    kManifestInvalid       = 8201, ///< Manifest fails schema or semantic checks.
    kSignatureVerifyFailed = 8202, ///< Cryptographic signature check failed.
    kLifecycleTransition   = 8203, ///< Invalid lifecycle state transition.
    kCapabilityDenied      = 8204, ///< Requested capability not granted.
    kRegistryConflict      = 8205, ///< Duplicate plugin ID on registration.
    kHealthCheckFailed     = 8206, ///< Health check returned unhealthy status.
    kInternalError         = 8207, ///< Unclassified internal error.
};

// ============================================================================
// § 2  Plugin identifier constraints
// ============================================================================

/// Maximum plugin identifier length in bytes.
inline constexpr std::size_t kMaxPluginIdBytes = 256;

/// Maximum plugin version string length in bytes.
inline constexpr std::size_t kMaxPluginVersionBytes = 64;

/// Maximum number of capability tokens per plugin manifest.
inline constexpr std::size_t kMaxCapabilitiesPerPlugin = 32;

/// Default health-check interval for monitored plugins.
inline constexpr std::chrono::seconds kDefaultHealthCheckInterval{30};

// ============================================================================
// § 3  Lifecycle states
// ============================================================================

/**
 * @brief Canonical lifecycle states for a managed plugin.
 *
 * Valid transitions: Unloaded → Loading → Active → Unloading → Unloaded.
 * Any deviation produces kLifecycleTransition.
 */
enum class PluginState : int32_t {
    Unloaded  = 0, ///< Plugin not present in memory.
    Loading   = 1, ///< Plugin load in progress (transient).
    Active    = 2, ///< Plugin loaded and accepting requests.
    Degraded  = 3, ///< Plugin loaded but health check failed.
    Unloading = 4, ///< Plugin unload in progress (transient).
};

// ============================================================================
// § 4  Supporting struct — plugin registration descriptor
// ============================================================================

/**
 * @brief Descriptor submitted when registering a new plugin.
 *
 * All fields except @p capabilities must be non-empty.
 */
struct PluginRegistrationDescriptor {
    std::string plugin_id;      ///< Unique plugin identifier (max kMaxPluginIdBytes).
    std::string version;        ///< Semantic version string (max kMaxPluginVersionBytes).
    std::string manifest_path;  ///< Path to the validated manifest file.
    std::string signature;      ///< Base64-encoded detached signature over manifest.
    bool        hot_plug_eligible{false}; ///< Whether the plugin supports hot-plug.
};

// ============================================================================
// § 5  Fail-closed contract
// ============================================================================

/**
 * @brief Returns true when the given error mandates fail-closed denial.
 */
[[nodiscard]] inline constexpr bool isPluginsFailClosed(PluginsError e) noexcept {
    return e == PluginsError::kSignatureVerifyFailed
        || e == PluginsError::kInternalError
        || e == PluginsError::kHealthCheckFailed;
}

} // namespace plugins
} // namespace themis
