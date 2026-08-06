// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file federated_span_contract.h
 * @brief OpenTelemetry integration for distributed tracing in federated process module.
 * @version 2.1.0-beta
 *
 * @section purpose Purpose
 * Defines span semantics, correlation ID propagation, and metric attachment for process
 * module operations running across multiple shards. Enables root-cause analysis of
 * federated operations via distributed traces in OpenTelemetry collectors (Jaeger, Zipkin, OTLP).
 *
 * @section span_model Span Model
 *
 * Process module spans form a DAG (directed acyclic graph):
 * ```
 * Root Span (import operation on Shard1)
 *   ├─ Child Span (consensus replication to Shard2)
 *   ├─ Child Span (conflict detection)
 *   ├─ Child Span (plugin callback for resolution)
 *   └─ Child Span (response to client)
 * ```
 *
 * Each span captures:
 * - **Operation:** What is being done (import, link, federation sync, conflict resolution)
 * - **Resource:** Where it's happening (shard ID, node ID, service name)
 * - **Attributes:** Contextual metadata (model ID, principal ID, version clock)
 * - **Events:** Milestones during operation (replication started, conflict detected, resolved)
 * - **Status:** Success/failure/error with diagnostic details
 *
 * @section correlation_propagation Correlation ID Propagation (W3C Trace Context)
 *
 * Correlation IDs are propagated via W3C Trace Context standard:
 * - **traceparent header:** `version-trace_id-parent_id-trace_flags`
 * - **tracestate header:** Vendor extensions (e.g., `themis=shard_id,principal_id`)
 *
 * **Example:**
 * ```
 * Client Request:
 *   traceparent: 00-0af7651916cd43dd8448eb211c80319c-b7ad6b7169203331-01
 *   tracestate: themis=shard-a,user123
 *
 * Server extracts trace_id = 0af7651916cd43dd8448eb211c80319c
 * Server propagates to other shards in RPC calls with same trace_id
 * All operations form single distributed trace for root-cause analysis
 * ```
 *
 * @section span_types Span Types
 *
 * | Type | Operation | Parent | Attributes | Events |
 * |------|-----------|--------|------------|--------|
 * | import | Model import | None (root) | model_id, size_bytes, shard_id | parse_start, parse_end, storage_start, storage_end |
 * | link | Link creation | import or None | model_id, link_type, target_id, shard_id | lock_acquired, persisted |
 * | federation_sync | Cross-shard sync | None (root) | sync_id, participating_shards, consensus_type | consensus_round_1, consensus_round_2, sync_complete |
 * | conflict_resolution | Plugin callback | federation_sync | conflict_type, local_version, remote_version | plugin_start, plugin_end, merge_metadata |
 * | consensus_rpc | RPC to remote shard | federation_sync | remote_shard_id, rpc_type, bytes_sent | request_sent, response_received |
 * | validation | Model validation | import or recovery | model_id, validation_rules | schema_check, cycle_check, constraint_check |
 *
 * @section span_attributes Standard Span Attributes
 *
 * ### All Process Module Spans
 * - `service.name` = "themisdb-process"
 * - `service.version` = version string
 * - `span.kind` = "INTERNAL" | "CLIENT" | "SERVER"
 * - `span.name` = operation name (e.g., "process.import", "process.link")
 * - `themis.module` = "process"
 * - `themis.operation` = operation type
 * - `themis.shard_id` = local shard ID
 * - `themis.principal_id` = principal (user/service) ID
 * - `db.system` = "themisdb"
 *
 * ### Import Span
 * - `process.model_id` = model ID
 * - `process.model_size_bytes` = size of model content
 * - `process.import_format` = "bpmn" | "cmmn" | "epk" | etc.
 * - `process.timestamp_ns` = model creation timestamp
 *
 * ### Link Span
 * - `process.model_id` = source model ID
 * - `process.link_type` = link type (e.g., "document", "instance")
 * - `process.target_id` = target entity ID
 * - `process.link_count` = total links on target after creation
 *
 * ### Federation Sync Span
 * - `process.sync_id` = unique sync operation ID
 * - `process.consensus_type` = "raft" | "paxos" | "gossip"
 * - `process.participating_shards` = CSV of shard IDs
 * - `process.conflict_count` = number of conflicts detected
 * - `process.resolutions_applied` = number of conflicts resolved
 *
 * ### Conflict Resolution Span
 * - `process.conflict_type` = "import_collision" | "linking_collision" | "concurrent_modification"
 * - `process.resolution_strategy` = "local_wins" | "remote_wins" | "merged" | "unresolved"
 * - `process.plugin_name` = conflict resolution plugin name (if used)
 * - `process.resolution_time_us` = resolution callback duration
 * - `process.fallback_to_lww` = whether fallback was applied
 *
 * ### Consensus RPC Span
 * - `process.rpc_type` = "request_vote" | "append_entries" | "prepare" | "accept"
 * - `process.remote_shard_id` = destination shard ID
 * - `process.bytes_sent` = size of RPC request
 * - `process.bytes_received` = size of RPC response
 * - `process.rpc_timeout_ms` = timeout applied to this RPC
 *
 * @section span_events Span Events (Milestones)
 *
 * Events mark significant points within a span's lifetime.
 *
 * ### Import Span Events
 * - `parse_start` (timestamp) – BPMN/CMMN parsing begins
 * - `parse_end` (timestamp, parse_duration_ms) – Parsing completes
 * - `validation_start` (timestamp) – Model validation begins
 * - `validation_end` (timestamp, validation_duration_ms, validation_result)
 * - `storage_start` (timestamp) – Persist to storage begins
 * - `storage_end` (timestamp, storage_duration_ms, model_version)
 * - `replication_start` (timestamp) – Replication to other shards begins (federation only)
 * - `replication_end` (timestamp, replication_duration_ms, replicated_shards)
 *
 * ### Conflict Resolution Span Events
 * - `conflict_detected` (timestamp, conflict_type, local_version, remote_version)
 * - `plugin_start` (timestamp, plugin_name)
 * - `plugin_end` (timestamp, plugin_result, plugin_duration_ms)
 * - `fallback_applied` (timestamp, reason) – Fallback to LWW applied
 * - `result_applied` (timestamp, winning_version)
 *
 * ### Federation Sync Span Events
 * - `round_1_start` (timestamp) – First consensus round begins
 * - `round_N_complete` (timestamp, votes_received, quorum_reached)
 * - `sync_complete` (timestamp, total_duration_ms, models_synced, conflicts_resolved)
 *
 * @section sampling_strategy Sampling Strategy
 *
 * Tracing overhead budget: < 2% on typical federated operations (< 5 µs per span).
 *
 * **Sampling Strategies:**
 * - **All:** 100% sampling (use for debugging, not production)
 * - **Percentile:** Sample N% of operations (e.g., 1% for low-overhead monitoring)
 * - **Slow-only:** Sample operations exceeding threshold (e.g., >100ms latency)
 * - **Adaptive:** Sample based on error rate (more samples if errors detected)
 *
 * **Default:** Slow-only (>100ms) + 1% random sampling for fast operations.
 *
 * @section metric_attachment Metrics Attached to Spans
 *
 * Span attributes can include performance metrics collected during operation:
 * - `process.cpu_time_us` – CPU time consumed
 * - `process.memory_used_bytes` – Peak memory usage
 * - `process.cache_hits` – Cache hits during operation
 * - `process.cache_misses` – Cache misses during operation
 * - `process.lock_wait_us` – Time spent waiting for locks
 * - `process.consensus_rounds` – Number of consensus rounds (federation)
 * - `process.rpc_calls` – Number of RPC calls made
 * - `process.audit_records_written` – Audit trail entries created
 *
 * @section exporter_configuration Exporter Configuration
 *
 * Process module spans can be exported to multiple backends:
 * - **OTLP (gRPC):** `http://otel-collector:4317`
 * - **OTLP (HTTP):** `http://otel-collector:4318/v1/traces`
 * - **Jaeger:** `http://jaeger:14268/api/traces` (OTLP-compatible)
 * - **Zipkin:** `http://zipkin:9411/api/v2/spans`
 *
 * Exporter must support **batch + async flush** (non-blocking; no trace I/O on hot path).
 *
 * @section contract_freeze Contract Freeze
 * This contract is frozen for ThemisDB v2.1; breaking changes require v3.0.
 */

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <map>
#include <chrono>
#include <memory>

namespace themis::process {

// ============================================================================
// Span Types and Attributes
// ============================================================================

/**
 * @brief Type of process module span.
 */
enum class ProcessSpanType : int32_t {
    /// Import operation span
    IMPORT = 6300,
    /// Link creation span
    LINK = 6301,
    /// Federation sync span
    FEDERATION_SYNC = 6302,
    /// Conflict resolution span
    CONFLICT_RESOLUTION = 6303,
    /// Consensus RPC call span
    CONSENSUS_RPC = 6304,
    /// Model validation span
    VALIDATION = 6305,
    /// Point-in-time recovery span
    RECOVERY = 6306,
};

/**
 * @brief Span status indicating success/failure.
 */
enum class SpanStatus : int32_t {
    /// Operation completed successfully
    OK = 6310,
    /// Operation failed with error
    ERROR = 6311,
    /// Operation cancelled or timed out
    CANCELLED = 6312,
};

// ============================================================================
// Span Context Carrier
// ============================================================================

/**
 * @brief W3C Trace Context for correlation ID propagation.
 *
 * Encodes trace_id, span_id, and sampling decision for propagation across RPC boundaries.
 */
struct TraceContext {
    /// 32-hex W3C trace ID (128-bit)
    std::string trace_id;

    /// 16-hex W3C span ID (64-bit)
    std::string span_id;

    /// Trace flags (1 byte): bit 0 = sampled flag
    uint8_t trace_flags = 0x01;  // Default: sampled

    /// Vendor extensions (e.g., "themis=shard_id,principal_id")
    std::string tracestate;

    /**
     * @brief Serialize to W3C Trace Context headers.
     * @return Map of header_name -> header_value
     */
    std::map<std::string, std::string> toHeaders() const {
        return {
            {"traceparent", "00-" + trace_id + "-" + span_id + "-" +
                           (trace_flags & 0x01 ? "01" : "00")},
            {"tracestate", tracestate}
        };
    }

    /**
     * @brief Parse from W3C Trace Context headers.
     * @param headers Map of header_name -> header_value
     * @return Parsed TraceContext, or std::nullopt if invalid
     */
    static std::optional<TraceContext> fromHeaders(
        const std::map<std::string, std::string>& headers
    );

    /**
     * @brief Check if this context is valid (non-empty trace_id and span_id).
     * @return true if valid
     */
    bool isValid() const noexcept {
        return trace_id.size() == 32 && span_id.size() == 16;
    }
};

// ============================================================================
// Span Attributes
// ============================================================================

/**
 * @brief Attributes attached to a span.
 *
 * Key-value pairs describing the span's context and operation details.
 */
struct SpanAttributes {
    /// Standard: service name
    std::string service_name = "themisdb-process";

    /// Standard: service version
    std::string service_version;

    /// Standard: span kind ("INTERNAL", "CLIENT", "SERVER")
    std::string span_kind = "INTERNAL";

    /// Standard: operation name (e.g., "process.import")
    std::string span_name;

    /// Process module: local shard ID
    std::string shard_id;

    /// Process module: principal (user/service) ID
    std::string principal_id;

    /// Process module: operation type (import, link, federation_sync, etc.)
    std::string operation_type;

    /// Generic attributes (additional key-value pairs)
    std::map<std::string, std::string> generic_attributes;

    /**
     * @brief Add a generic attribute.
     * @param key Attribute key
     * @param value Attribute value
     */
    void setAttribute(const std::string& key, const std::string& value) {
        generic_attributes[key] = value;
    }
};

// ============================================================================
// Span Events
// ============================================================================

/**
 * @brief Event recorded within a span (milestone or significant point).
 */
struct SpanEvent {
    /// Event name (e.g., "parse_end", "conflict_detected")
    std::string name;

    /// Timestamp of event (UTC, nanoseconds since epoch)
    int64_t timestamp_ns = 0;

    /// Event-specific attributes (key-value pairs)
    std::map<std::string, std::string> attributes;

    /**
     * @brief Create event with current timestamp.
     * @param event_name Event name
     * @return SpanEvent with current timestamp
     */
    static SpanEvent now(const std::string& event_name) {
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        return {event_name, ns, {}};
    }
};

// ============================================================================
// Span Interface
// ============================================================================

/**
 * @brief Lightweight span representing a unit of work in federated operation.
 *
 * Spans are created by process module and managed by OpenTelemetry tracer.
 * Application code should not create spans directly (use process module APIs).
 */
class ISpan {
public:
    virtual ~ISpan() = default;

    /**
     * @brief Get trace context (for RPC propagation).
     * @return TraceContext containing trace_id and span_id
     */
    virtual TraceContext traceContext() const noexcept = 0;

    /**
     * @brief Add an attribute to this span.
     * @param key Attribute name
     * @param value Attribute value
     */
    virtual void setAttribute(const std::string& key, const std::string& value) = 0;

    /**
     * @brief Add an event (milestone) to this span.
     * @param event SpanEvent to record
     */
    virtual void addEvent(const SpanEvent& event) = 0;

    /**
     * @brief Set span status (success, error, cancelled).
     * @param status SpanStatus
     * @param description Optional error description
     */
    virtual void setStatus(SpanStatus status, const std::string& description = "") = 0;

    /**
     * @brief End this span (mark as complete).
     *
     * After calling end(), span is immutable. No further modifications are allowed.
     */
    virtual void end() = 0;
};

// ============================================================================
// Tracer Interface (Process-Specific)
// ============================================================================

/**
 * @brief Process module tracer (extends observability::OpenTelemetryTracer).
 *
 * Provides factory methods for creating process-specific spans.
 */
class IProcessTracer {
public:
    virtual ~IProcessTracer() = default;

    /**
     * @brief Create import span.
     *
     * @param model_id Model being imported
     * @param shard_id Local shard ID
     * @param principal_id Principal performing import
     * @param parent_context Optional parent trace context
     * @return Span for import operation
     */
    virtual std::unique_ptr<ISpan> createImportSpan(
        const std::string& model_id,
        const std::string& shard_id,
        const std::string& principal_id,
        const std::optional<TraceContext>& parent_context = std::nullopt
    ) = 0;

    /**
     * @brief Create link span.
     *
     * @param model_id Source model ID
     * @param target_id Target entity ID
     * @param shard_id Local shard ID
     * @param principal_id Principal creating link
     * @param parent_context Optional parent trace context
     * @return Span for link creation
     */
    virtual std::unique_ptr<ISpan> createLinkSpan(
        const std::string& model_id,
        const std::string& target_id,
        const std::string& shard_id,
        const std::string& principal_id,
        const std::optional<TraceContext>& parent_context = std::nullopt
    ) = 0;

    /**
     * @brief Create federation sync span.
     *
     * @param sync_id Unique sync operation ID
     * @param participating_shards CSV of shard IDs participating in sync
     * @param principal_id Principal triggering sync
     * @param consensus_type Consensus protocol ("raft", "paxos", "gossip")
     * @return Span for federation sync
     */
    virtual std::unique_ptr<ISpan> createFederationSyncSpan(
        const std::string& sync_id,
        const std::string& participating_shards,
        const std::string& principal_id,
        const std::string& consensus_type
    ) = 0;

    /**
     * @brief Create conflict resolution span.
     *
     * @param conflict_type Type of conflict
     * @param local_version Local version being resolved
     * @param remote_version Remote version being resolved
     * @param plugin_name Conflict resolution plugin name (if used)
     * @param parent_context Parent trace context (from federation sync)
     * @return Span for conflict resolution
     */
    virtual std::unique_ptr<ISpan> createConflictResolutionSpan(
        const std::string& conflict_type,
        const std::string& local_version,
        const std::string& remote_version,
        const std::string& plugin_name,
        const TraceContext& parent_context
    ) = 0;

    /**
     * @brief Create consensus RPC span.
     *
     * @param rpc_type RPC type ("request_vote", "append_entries", "prepare", "accept")
     * @param remote_shard_id Destination shard ID
     * @param parent_context Parent trace context (from federation sync)
     * @return Span for RPC call
     */
    virtual std::unique_ptr<ISpan> createConsensusRpcSpan(
        const std::string& rpc_type,
        const std::string& remote_shard_id,
        const TraceContext& parent_context
    ) = 0;

    /**
     * @brief Create recovery span.
     *
     * @param model_id Model being recovered
     * @param target_timestamp_ns Target timestamp for recovery
     * @param shard_id Local shard ID
     * @param principal_id Principal performing recovery
     * @return Span for point-in-time recovery
     */
    virtual std::unique_ptr<ISpan> createRecoverySpan(
        const std::string& model_id,
        int64_t target_timestamp_ns,
        const std::string& shard_id,
        const std::string& principal_id
    ) = 0;

    /**
     * @brief Get singleton instance of process tracer.
     * @return Reference to global process tracer
     */
    static IProcessTracer& instance();
};

// ============================================================================
// Tracing Configuration
// ============================================================================

/**
 * @brief Configuration for process module tracing.
 */
struct TracingConfig {
    /// true to enable tracing; false to disable
    bool enabled = true;

    /// Sampling strategy: "all" | "percentile" | "slow_only" | "adaptive"
    std::string sampling_strategy = "slow_only";

    /// Percentile for percentile sampling (1-100)
    uint32_t sampling_percentile = 1;

    /// Threshold for slow_only sampling (ms)
    uint32_t slow_operation_threshold_ms = 100;

    /// Exporter endpoint (OTLP, Jaeger, or Zipkin)
    std::string exporter_endpoint = "http://localhost:4317";

    /// Exporter type: "otlp" | "jaeger" | "zipkin"
    std::string exporter_type = "otlp";

    /// Batch size for span export
    uint32_t batch_size = 512;

    /// Timeout for span export (ms)
    uint32_t export_timeout_ms = 5000;

    /// true to export asynchronously (non-blocking)
    bool async_export = true;

    /// Service name embedded in spans
    std::string service_name = "themisdb-process";

    /// Service version embedded in spans
    std::string service_version;

    /// Correlation ID for this tracer instance
    std::string trace_id;
};

} // namespace themis::process
