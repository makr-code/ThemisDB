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

enum class SubagentIsolationLevel {
    NONE = 0,

    ADVISORY = 1,

    STRICT = 2,

    STRICT_WITH_PREEMPTION = 3,
};

// ============================================================================
// § 2  Subagent Configuration
// ============================================================================

struct SubagentQuantizationConfig {
    bool enabled = false;
    std::string mode = "none";
    bool per_channel = true;
    int group_size = 128;
};

enum class SubagentMultiGPUStrategy {
    NONE = 0,           ///< Single GPU (default)
    ROUND_ROBIN = 1,    ///< Distribute across GPUs
    DATA_PARALLEL = 2,  ///< Replicate on all GPUs
    MODEL_PARALLEL = 3, ///< Split model across GPUs
};

struct SubagentGPUConfig {
    bool enabled = false;
    std::vector<int> devices;
    SubagentMultiGPUStrategy strategy = SubagentMultiGPUStrategy::ROUND_ROBIN;
    bool enable_peer_transfer = false;
    size_t max_vram_per_gpu_mb = 24 * 1024;
    bool enable_load_balancing = true;
    float load_balance_threshold = 0.8f;
};

struct SubagentPolicyConfig {
    std::string prompt_policy_id;

    std::string ethics_profile_id;

    int max_retries = 1;

    bool block_on_policy_violation = true;

    bool block_on_quota_violation = true;
};

struct SubagentBudgetConfig {
    size_t max_tokens_per_minute = 0;

    int max_tokens_per_request = 512;

    int timeout_ms = 30000;

    int max_concurrent_requests = 8;

    int priority = 0;
};

struct SubagentConfig {
    std::string id = {};

    std::string model_id;

    std::string lora_adapter_id;

    std::string description;

    SubagentIsolationLevel isolation_level = SubagentIsolationLevel::STRICT;

    SubagentBudgetConfig budget;

    SubagentPolicyConfig policy;

    SubagentGPUConfig gpu;
    SubagentQuantizationConfig quantization;

    bool enable_audit_logging = true;
    bool enable_metrics = true;
    bool enable_tracing = false;

    std::string tenant_id;

    std::vector<std::string> tags;
};

// ============================================================================
// § 3  Subagent Lifecycle State
// ============================================================================

enum class SubagentState {
    CREATED = 0,
    LOADING = 1,
    READY = 2,
    PAUSED = 3,
    UNLOADING = 4,
    TERMINATED = 5,
    ERROR = 6,
};

/**
 * @brief Subagent State To String.
 * @param[in] state Input parameter.
 * @return Pointer to the result.
 * @details Implements subagentStateToString without additional internal calls.
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

struct SubagentMetrics {
    uint64_t total_requests = 0;
    uint64_t successful_inferences = 0;
    uint64_t failed_inferences = 0;
    uint64_t policy_blocks = 0;
    uint64_t quota_blocks = 0;
    size_t tokens_consumed = 0;
    uint64_t vram_used_bytes = 0;
    uint64_t vram_peak_bytes = 0;
    uint64_t total_tokens_processed = 0;
    std::chrono::steady_clock::time_point last_request_time;
    std::chrono::steady_clock::time_point load_time;
};

} // namespace llm
} // namespace themis
