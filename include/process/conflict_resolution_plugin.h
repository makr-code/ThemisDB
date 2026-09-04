// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file conflict_resolution_plugin.h
 * @brief Plugin API for custom process model conflict resolution strategies.
 * @version 2.1.0-beta
 *
 * @section purpose Purpose
 * Provides extensible conflict resolution beyond Last-Write-Wins (LWW). Applications
 * can register custom callbacks to resolve concurrent model imports, link collisions,
 * and versioning conflicts with deterministic fallback to frozen LWW tiebreaker.
 *
 * @section architecture Architecture
 *
 * Conflict resolution operates at federation sync boundary:
 * 1. **Conflict Detection:** Version vector mismatch or concurrent write to same model
 * 2. **Plugin Invocation:** Application callback invoked with (local, remote, context)
 * 3. **Resolution Strategy:** Callback returns LocalWins, RemoteWins, Merged, or Unresolved
 * 4. **Fallback:** If callback unavailable or returns Unresolved, use deterministic LWW
 *
 * @section conflict_types Conflict Types
 *
 * | Type | Trigger | Context | Example |
 * |------|---------|---------|---------|
 * | IMPORT_COLLISION | Same model ID imported on different shards | Model versions, timestamps | Import BPMN v1 on Shard1, same BPMN v2 on Shard2 |
 * | LINKING_COLLISION | Link created to same target from different sources | Link instances, timestamps | Link document on Shard1, same document on Shard2 |
 * | CONCURRENT_MODIFICATION | Model modified concurrently on multiple shards | Version clocks, payloads | Shard1 updates model at v100, Shard2 updates at v100 |
 * | VERSION_DIVERGENCE | Replicas have different model states for same ID | Version vectors, snapshots | Shard1 has model v5, Shard2 has model v4 (replication lag) |
 *
 * @section plugin_lifecycle Plugin Lifecycle
 *
 * **Registration (Load-time):**
 * - Plugin implementation must inherit from `IConflictResolutionPlugin`
 * - Register via `ConflictResolutionRegistry::registerPlugin(name, factory)`
 * - Factory creates instance per operation; must be thread-safe
 *
 * **Invocation (Conflict-time):**
 * - Process module detects conflict; looks up plugin in registry
 * - Plugin callback invoked with (local_version, remote_version, conflict_context)
 * - Callback has < 10 ms budget; timeout → Unresolved → LWW fallback
 *
 * **Unregistration (Shutdown):**
 * - Application unregisters plugin via `ConflictResolutionRegistry::unregisterPlugin(name)`
 * - In-flight operations complete with fallback; no new invocations of unregistered plugin
 *
 * @section deterministic_fallback Deterministic Fallback (LWW Tiebreaker)
 *
 * When plugin unavailable or returns Unresolved:
 * - Apply Last-Write-Wins with **shard_id → version_clock** tiebreaker
 * - Order: version_clock (descending) → shard_id (lexicographic ascending)
 * - **Guarantee:** Identical input always produces identical outcome (deterministic)
 *
 * Example:
 * ```
 * Local:  model_id="proc1", term=5, shard_id="shard-a", timestamp=1000
 * Remote: model_id="proc1", term=5, shard_id="shard-b", timestamp=1000
 * Tiebreaker: term equal, timestamp equal → shard_id("shard-a" < "shard-b") → Local wins
 * Outcome: deterministic (same input always produces same result)
 * ```
 *
 * @section 3way_merge 3-Way Merge Semantics
 *
 * For structured merges (BPMN/CMMN models):
 * - **Base version:** Common ancestor model (from replication log)
 * - **Local version:** Model on receiving shard
 * - **Remote version:** Model on sending shard
 * - **Merge algorithm:** Find additions/deletions/modifications relative to base
 * - **Result:** Merged model; validation required before applying
 *
 * Example:
 * ```
 * Base:   nodes=[A, B], edges=[A->B]
 * Local:  nodes=[A, B, C], edges=[A->B, B->C]  (added C)
 * Remote: nodes=[A, B, D], edges=[A->B, A->D]  (added D)
 * Merged: nodes=[A, B, C, D], edges=[A->B, B->C, A->D]  (union)
 * Validation: Ensure no cycles, no dangling edges
 * ```
 *
 * @section callback_interface Callback Interface
 *
 * Plugin must implement:
 * ```cpp
 * struct ConflictResolution {
 *     enum Strategy { LocalWins, RemoteWins, Merged, Unresolved };
 *     Strategy strategy;
 *     std::optional<ProcessModel> merged_model;  // if strategy == Merged
 * };
 *
 * ConflictResolution resolve(
 *     const ModelVersion& local,
 *     const ModelVersion& remote,
 *     const ConflictContext& ctx
 * );
 * ```
 *
 * @section contract_freeze Contract Freeze
 * This plugin API is frozen for ThemisDB v2.1; plugin interface changes require v3.0.
 */

#include <cstdint>
#include <string>
#include <memory>
#include <functional>
#include <optional>
#include <map>
#include <chrono>

namespace themis::process {

// ============================================================================
// Conflict Types and Resolution Strategies
// ============================================================================

/**
 * @brief Type of conflict detected during federation sync or replication.
 */
enum class ConflictType : int32_t {
    /// Same model ID imported on different shards with different content
    IMPORT_COLLISION = 6200,
    /// Link created to same target from different sources concurrently
    LINKING_COLLISION = 6201,
    /// Model modified concurrently on multiple shards
    CONCURRENT_MODIFICATION = 6202,
    /// Replicas have diverged; version vector mismatch
    VERSION_DIVERGENCE = 6203,
};

/**
 * @brief Resolution strategy determined by conflict resolution plugin.
 */
enum class ConflictResolutionStrategy : int32_t {
    /// Local version wins; remote version discarded
    LOCAL_WINS = 6210,
    /// Remote version wins; local version replaced
    REMOTE_WINS = 6211,
    /// Merged version from 3-way merge (plugin-generated)
    MERGED = 6212,
    /// Conflict unresolved; fallback to deterministic LWW tiebreaker
    UNRESOLVED = 6213,
};

/**
 * @brief Serialized representation of a process model version (opaque to plugin).
 *
 * Contains metadata required by plugin to make conflict resolution decision.
 */
struct ModelVersion {
    /// Unique model identifier
    std::string model_id;

    /// Version number (from process module version clock)
    uint64_t version = 0;

    /// Term from consensus replication log
    uint64_t term = 0;

    /// Shard ID where this version originated
    std::string shard_id;

    /// Timestamp of model creation/modification (UTC, nanoseconds since epoch)
    int64_t timestamp_ns = 0;

    /// Serialized model content (BPMN/CMMN XML or JSON)
    std::string model_content = {};

    /// Cryptographic hash of model_content (for integrity)
    std::string content_hash;

    /// Conflict-resolution operation counter (for audit trail)
    uint32_t conflict_count = 0;
};

/**
 * @brief Context information for conflict resolution callback.
 *
 * Provides additional information about the conflict and the operation context.
 */
struct ConflictContext {
    /// Type of conflict detected
    ConflictType conflict_type = ConflictType::CONCURRENT_MODIFICATION;

    /// Principal (user/service) ID that triggered the operation
    std::string principal_id;

    /// Source shard ID (where conflict resolution is running)
    std::string local_shard_id;

    /// Remote shard ID (where conflicting model originated)
    std::string remote_shard_id;

    /// Timestamp of conflict detection (UTC, nanoseconds since epoch)
    int64_t detection_timestamp_ns = 0;

    /// Optional: base version (common ancestor for 3-way merge)
    std::optional<ModelVersion> base_version;

    /// Correlation ID for tracing this conflict resolution
    std::string trace_id;

    /// Audit trail: operation history leading to this conflict
    std::string audit_log_snippet;
};

/**
 * @brief Result of conflict resolution from plugin callback.
 *
 * Plugin implementation returns this to indicate chosen strategy and merged model (if any).
 */
struct ConflictResolutionResult {
    /// Strategy chosen by plugin
    ConflictResolutionStrategy strategy = ConflictResolutionStrategy::UNRESOLVED;

    /// Merged model (only populated if strategy == MERGED)
    std::optional<ModelVersion> merged_model;

    /// Reason for chosen strategy (for audit trail and diagnostics)
    std::string reason;

    /// Merge metadata (if strategy == MERGED): algorithm used, conflicts resolved
    std::string merge_metadata;

    /// Duration of resolution in microseconds (for performance tracking)
    uint64_t resolution_duration_us = 0;

    /// true if result is deterministic (same input always produces same result)
    bool is_deterministic = true;
};

/**
 * @brief Built-in conflict resolution strategy: Last-Write-Wins (LWW).
 *
 * Applies LWW using monotonic version clocks with deterministic shard_id tiebreaker.
 * This is the default and fallback strategy.
 */
struct LWWResolutionStrategy {
    /**
     * @brief Resolve conflict using Last-Write-Wins.
     * @param local Local model version
     * @param remote Remote model version
     * @return Result with LOCAL_WINS or REMOTE_WINS
     */
    static ConflictResolutionResult resolve(
        const ModelVersion& local,
        const ModelVersion& remote
    ) noexcept;
};

/**
 * @brief Built-in conflict resolution strategy: 3-Way Merge.
 *
 * Performs structural merge of BPMN/CMMN models using base, local, and remote versions.
 * Requires base version to be present in ConflictContext.
 */
struct ThreeWayMergeStrategy {
    /**
     * @brief Resolve conflict using 3-way merge.
     * @param local Local model version
     * @param remote Remote model version
     * @param base Base model version (common ancestor)
     * @return Result with MERGED strategy and merged_model, or UNRESOLVED if merge impossible
     */
    static ConflictResolutionResult resolve(
        const ModelVersion& local,
        const ModelVersion& remote,
        const ModelVersion& base
    ) noexcept;
};

// ============================================================================
// Plugin Interface
// ============================================================================

/**
 * @brief Abstract interface for conflict resolution plugin implementations.
 *
 * Applications must inherit from this and implement `resolve()`.
 */
class IConflictResolutionPlugin {
public:
    virtual ~IConflictResolutionPlugin() = default;

    /**
     * @brief Resolve a conflict between two model versions.
     *
     * Called when process module detects a conflict during federation sync or replication.
     * Must complete within < 10 ms; timeout triggers fallback to LWW.
     *
     * @param local Local model version
     * @param remote Remote model version
     * @param ctx Conflict context (includes base version if available)
     * @return Conflict resolution result with chosen strategy
     */
    virtual ConflictResolutionResult resolve(
        const ModelVersion& local,
        const ModelVersion& remote,
        const ConflictContext& ctx
    ) = 0;

    /**
     * @brief Get human-readable name of this plugin.
     * @return Plugin name (for logging and debugging)
     */
    virtual std::string name() const noexcept = 0;

    /**
     * @brief Validate that a merged model is consistent after merge.
     *
     * Optional; called by process module after merge to ensure result is valid.
     *
     * @param merged Merged model to validate
     * @return true if model is valid (passes schema, no cycles, etc.)
     */
    virtual bool validateMerge(const ModelVersion& merged) const noexcept {
        return true;  // Default: assume valid
    }
};

// ============================================================================
// Plugin Registry
// ============================================================================

/**
 * @brief Factory function type for creating plugin instances.
 *
 * Called each time a conflict needs resolution; must be thread-safe and fast.
 */
using ConflictResolutionPluginFactory = std::function<
    std::unique_ptr<IConflictResolutionPlugin>()
>;

/**
 * @brief Global registry for conflict resolution plugins.
 *
 * Thread-safe singleton; applications register plugins at startup, process module
 * looks up plugins at conflict-time.
 */
class ConflictResolutionRegistry {
public:
    /**
     * @brief Get singleton instance.
     * @return Reference to global registry
     */
    static ConflictResolutionRegistry& instance();

    /**
     * @brief Register a conflict resolution plugin.
     *
     * @param name Unique plugin name (e.g., "custom_merge_v1")
     * @param factory Factory function to create instances
     * @return true if registered successfully; false if name already registered
     */
    bool registerPlugin(
        const std::string& name,
        ConflictResolutionPluginFactory factory
    );

    /**
     * @brief Unregister a conflict resolution plugin.
     *
     * In-flight operations continue with fallback; no new invocations after unregister.
     *
     * @param name Plugin name to unregister
     * @return true if unregistered; false if plugin not found
     */
    bool unregisterPlugin(const std::string& name);

    /**
     * @brief Get a plugin by name.
     *
     * @param name Plugin name
     * @return Newly created plugin instance, or nullptr if not registered
     */
    std::unique_ptr<IConflictResolutionPlugin> getPlugin(const std::string& name);

    /**
     * @brief Check if a plugin is registered.
     *
     * @param name Plugin name
     * @return true if plugin is registered
     */
    bool hasPlugin(const std::string& name) const;

    /**
     * @brief Get list of all registered plugin names.
     *
     * @return Vector of plugin names
     */
    std::vector<std::string> listPlugins() const;

private:
    ConflictResolutionRegistry() = default;
    ~ConflictResolutionRegistry() = default;

    std::map<std::string, ConflictResolutionPluginFactory> plugins_;
    mutable std::mutex mutex_;
};

// ============================================================================
// Conflict Resolution Configuration
// ============================================================================

/**
 * @brief Configuration for conflict resolution in federated deployments.
 */
struct ConflictResolutionConfig {
    /// Name of plugin to use for conflict resolution (empty = use LWW fallback)
    std::string plugin_name;

    /// Timeout for plugin callback in milliseconds
    uint32_t callback_timeout_ms = 10;

    /// Strategy if callback times out: "lww_fallback" or "reject_operation"
    std::string timeout_strategy = "lww_fallback";

    /// true to record all conflict resolution decisions in audit trail
    bool audit_all_conflicts = true;

    /// Maximum number of conflicts to record in audit trail (prevents unbounded growth)
    uint32_t audit_buffer_size = 10000;

    /// Correlation ID for tracing conflict resolution operations
    std::string trace_id;
};

} // namespace themis::process
