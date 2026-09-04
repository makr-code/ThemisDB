/**
 * @file subagent_config.h
 * @brief Subagent configuration and lifecycle contracts for independent LLM
 *        inference subagents with isolated model, adapter, and policy settings.
 *
 * @note **Header-Only Configuration**: Defines configuration struct and enums.
 *       Implementations use these structures for lifecycle management.
 *
 * ## Purpose
 *
 * ThemisDB's multi-subagent orchestration allows independent LLM inference
 * instances to run in parallel while accessing shared database infrastructure.
 * Each subagent is independently configured with:
 *   - Unique model_id + optional LoRA adapter_id
 *   - Budget constraints (token quota, timeout)
 *   - Policy gates (prompt policy, safety guardrails)
 *   - Resource isolation (VRAM allocation, quota enforcement)
 *   - Observability (correlation ID, audit trails)
 *
 * ## Architecture
 *
 * A subagent is a long-lived orchestration entity that:
 *   1. Loads/unloads an LLM model (via ModelLoader) + LoRA adapter (via MultiLoRAManager)
 *   2. Enforces independent policy gates (via PromptPolicy)
 *   3. Tracks token quota (via TokenQuotaManager)
 *   4. Submits inference requests (via AsyncInferenceEngine)
 *   5. Logs interactions (via LLMInteractionStore + audit trails)
 *
 * Multiple subagents can:
 *   - Read from shared database (WikiIndexStore, SSMStateRocksDBStore)
 *   - Access shared caches (LLMResponseCache, LLMPrefixCache)
 *   - Run in parallel via SharedWorkerPool
 *   - Enforce independent quotas and policies
 *   - Be coordinated by a SubagentCoordinator for fan-out inference
 *
 * ## Non-Breaking Design
 *
 * Subagent infrastructure is **opt-in** and does not modify existing
 * AIOrchestrator, AsyncInferenceEngine, or MultiLoRAManager interfaces.
 * Existing callers are unaffected; subagent orchestration is new surface area.
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <cstdint>
#include <chrono>

namespace themis {
namespace llm {

// ============================================================================
// § 1  Subagent Isolation Levels
// ============================================================================

/**
 * @brief Resource isolation level for a subagent.
 *
 * Determines whether resource conflicts (VRAM, thread pool slots, quota)
 * are strictly enforced or advisory.
 */
enum class SubagentIsolationLevel {
    /// No enforcement (legacy default, not recommended for production).
    NONE = 0,

    /// Advisory only; quota/policy violations are logged but not enforced.
    ADVISORY = 1,

    /// Strict enforcement; quota/policy violations block requests.
    STRICT = 2,

    /// Strict + preemption; resource-starved subagents may preempt lower-priority
    /// requests (experimental, requires fairness monitor).
    STRICT_WITH_PREEMPTION = 3,
};

// ============================================================================
// § 2  Subagent Configuration
// ============================================================================

/**
 * @brief Quantization configuration for a subagent's LoRA adapter.
 */
struct SubagentQuantizationConfig {
    /// Enable quantization (INT8/INT4/FP32).
    bool enabled = false;
    /// Quantization mode: "none", "int8", "int4" (default: "none").
    std::string mode = "none";
    /// Per-channel quantization (vs per-tensor).
    bool per_channel = true;
    /// Group size for INT4 (0 = per-channel, default: 128).
    int group_size = 128;
};

/**
 * @brief Multi-GPU placement strategy for a subagent.
 */
enum class SubagentMultiGPUStrategy {
    NONE = 0,           ///< Single GPU (default)
    ROUND_ROBIN = 1,    ///< Distribute across GPUs
    DATA_PARALLEL = 2,  ///< Replicate on all GPUs
    MODEL_PARALLEL = 3, ///< Split model across GPUs
};

/**
 * @brief GPU placement configuration for a subagent.
 */
struct SubagentGPUConfig {
    /// Enable multi-GPU placement.
    bool enabled = false;
    /// GPU device IDs to use (e.g., {0, 1, 2, 3}).
    std::vector<int> devices;
    /// Multi-GPU strategy.
    SubagentMultiGPUStrategy strategy = SubagentMultiGPUStrategy::ROUND_ROBIN;
    /// Enable GPU P2P (GPUDirect).
    bool enable_peer_transfer = false;
    /// Max VRAM per GPU (MB, default: 24 * 1024 = 24GB).
    size_t max_vram_per_gpu_mb = 24 * 1024;
    /// Enable load balancing.
    bool enable_load_balancing = true;
    /// Load balance threshold (default: 0.8 = 80%).
    float load_balance_threshold = 0.8f;
};

/**
 * @brief Policy configuration for a subagent.
 */
struct SubagentPolicyConfig {
    /// ID of the prompt policy to enforce (references PromptPolicy registry).
    /// Empty string = no policy enforcement.
    std::string prompt_policy_id;

    /// ID of the safety/ethics profile (references ethics YAML registry).
    /// Empty string = no safety enforcement.
    std::string ethics_profile_id;

    /// Maximum inference retries on transient failure.
    int max_retries = 1;

    /// Block if request violates policy? (vs advisory-only logging)
    bool block_on_policy_violation = true;

    /// Block if request violates quota? (vs advisory-only logging)
    bool block_on_quota_violation = true;
};

/**
 * @brief Budget constraints for a subagent.
 *
 * Subagents enforce independent token quotas, timeouts, and rate limits.
 */
struct SubagentBudgetConfig {
    /// Maximum tokens per 60-second window (0 = unlimited).
    size_t max_tokens_per_minute = 0;

    /// Maximum tokens per single inference request.
    int max_tokens_per_request = 512;

    /// Maximum wall-clock time per inference request (ms).
    int timeout_ms = 30000;

    /// Maximum number of concurrent requests from this subagent.
    int max_concurrent_requests = 8;

    /// Priority level (higher = more urgent; used by scheduler).
    int priority = 0;
};

/**
 * @brief Independent LLM Inferencing Subagent configuration.
 *
 * A subagent represents a single, isolated instance of LLM inference with:
 *   - Independent model_id and optional LoRA adapter_id
 *   - Isolated quota, policy, and resource constraints
 *   - Unique correlation_id for tracing and audit trails
 *   - Shareable access to common database and cache infrastructure
 *
 * ### Usage
 * @code
 *   SubagentConfig config;
 *   config.id = "assistant_1";
 *   config.model_id = "mistral-7b";
 *   config.lora_adapter_id = "customer-support";
 *   config.budget.max_tokens_per_minute = 50000;
 *   config.policy.prompt_policy_id = "no-jailbreak";
 *   config.isolation_level = SubagentIsolationLevel::STRICT;
 *
 *   // Factory creates subagent instance from config:
 *   auto factory = SubagentFactory::create(...);
 *   auto subagent = factory->createSubagent(config);
 * @endcode
 */
struct SubagentConfig {
    /// Unique subagent identifier (e.g., "assistant_1", "analyzer_2").
    std::string id = {};

    /// LLM model identifier (e.g., "mistral-7b", "llama2-70b").
    std::string model_id;

    /// Optional LoRA adapter ID (empty = use base model only).
    std::string lora_adapter_id;

    /// Optional description of subagent's role/purpose.
    std::string description;

    /// Isolation level for resource enforcement.
    SubagentIsolationLevel isolation_level = SubagentIsolationLevel::STRICT;

    /// Budget constraints (tokens, timeout, concurrency).
    SubagentBudgetConfig budget;

    /// Policy enforcement configuration.
    SubagentPolicyConfig policy;

    /// GPU placement and quantization.
    SubagentGPUConfig gpu;
    SubagentQuantizationConfig quantization;

    /// Observability flags.
    bool enable_audit_logging = true;
    bool enable_metrics = true;
    bool enable_tracing = false;

    /// Optional tenant/organization ID for multi-tenant isolation.
    std::string tenant_id;

    /// Optional deployment tags (e.g., ["prod", "us-east-1"]).
    std::vector<std::string> tags;
};

// ============================================================================
// § 3  Subagent Lifecycle State
// ============================================================================

/**
 * @brief State of a subagent instance.
 */
enum class SubagentState {
    /// Created but not yet initialized.
    CREATED = 0,
    /// Model/adapter loading in progress.
    LOADING = 1,
    /// Ready to accept inference requests.
    READY = 2,
    /// Temporarily paused (quota exhausted, policy violation, etc.).
    PAUSED = 3,
    /// Unloading in progress.
    UNLOADING = 4,
    /// Terminated; cannot be reused.
    TERMINATED = 5,
    /// Error state; requires recovery.
    ERROR = 6,
};

/**
 * @brief Human-readable state name.
 */
inline const char* subagentStateToString(SubagentState state) {
    switch (state) {
        case SubagentState::CREATED:       return "CREATED";
        case SubagentState::LOADING:       return "LOADING";
        case SubagentState::READY:         return "READY";
        case SubagentState::PAUSED:        return "PAUSED";
        case SubagentState::UNLOADING:     return "UNLOADING";
        case SubagentState::TERMINATED:    return "TERMINATED";
        case SubagentState::ERROR:         return "ERROR";
        default:                           return "UNKNOWN";
    }
}

// ============================================================================
// § 4  Subagent Resource Metrics
// ============================================================================

/**
 * @brief Runtime statistics for a subagent.
 */
struct SubagentMetrics {
    /// Total inference requests submitted.
    uint64_t total_requests = 0;
    /// Successful inferences.
    uint64_t successful_inferences = 0;
    /// Failed inferences.
    uint64_t failed_inferences = 0;
    /// Requests blocked by policy.
    uint64_t policy_blocks = 0;
    /// Requests blocked by quota.
    uint64_t quota_blocks = 0;
    /// Tokens consumed in current window.
    size_t tokens_consumed = 0;
    /// Current VRAM usage (bytes).
    uint64_t vram_used_bytes = 0;
    /// Peak VRAM usage (bytes).
    uint64_t vram_peak_bytes = 0;
    /// Cumulative tokens processed.
    uint64_t total_tokens_processed = 0;
    /// Last request timestamp (steady_clock).
    std::chrono::steady_clock::time_point last_request_time;
    /// Time subagent loaded.
    std::chrono::steady_clock::time_point load_time;
};

} // namespace llm
} // namespace themis
