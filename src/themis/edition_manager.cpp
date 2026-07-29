/**
 * @file edition_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: edition_manager.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 223
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * PR History (last 5): #3646 fix(themis): complete build... (2026-03-12) | #3598 feat(themis): complete Phas... (2026-03-12) | #3429 [WIP] Add full modularizati... (2026-03-12) | #3411 [themis] Add getRegisteredM... (2026-03-12) | #3410 feat(themis): Dynamic featu... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB Edition Manager – Implementation
 * ==========================================
 * Community / Enterprise / Hyperscaler edition feature gating.
 *
 * See include/themis/edition_manager.h for full API documentation.
 */

#include "themis/edition_manager.h"
#include "themis/runtime_license_gate.h"
#include "utils/logger.h"

#include <optional>
#include <sstream>

namespace themis {
namespace edition {

// ============================================================================
// Singleton
// ============================================================================

EditionManager& EditionManager::instance() {
    static EditionManager mgr;
    return mgr;
}

// ============================================================================
// Feature availability
// ============================================================================

bool EditionManager::isFeatureAvailable(std::string_view feature_name) const {
    std::string unused;
    return isFeatureAvailable(feature_name, unused);
}

bool EditionManager::isFeatureAvailable(std::string_view feature_name,
                                        std::string& error_out) const {
    // Step 0: Check dynamic override first (if any).
    {
        std::lock_guard<std::mutex> lock(overrides_mutex_);
        auto it = overrides_.find(std::string(feature_name));
        if (it != overrides_.end() && !it->second) {
            // Override explicitly set to false → always blocked by admin.
            std::ostringstream msg;
            msg << "Feature '" << feature_name
                << "' has been administratively disabled at runtime.";
            error_out = msg.str();
            return false;
        }
        // Override=true means "allow if edition+license also allow" — fall through.
    }

    // Delegate to RuntimeLicenseGate which already implements the two-stage
    // (compile-time + runtime-license) check.
    return license::RuntimeLicenseGate::instance()
               .isFeatureAllowed(feature_name, error_out);
}

// ============================================================================
// Resource-limit checks
// ============================================================================

bool EditionManager::checkNodeLimit(int requested_nodes,
                                    std::string& error_out) const {
    const int ceiling = SHARDING_MAX_NODES;  // compile-time, absolute ceiling

    // Step 1: Compile-time ceiling (Defense in Depth — never bypassed).
    if (ceiling >= 0 && requested_nodes > ceiling) {
        std::ostringstream msg;
        msg << "Requested node count (" << requested_nodes
            << ") exceeds the compile-time ceiling for the " << EDITION_STRING
            << " edition (" << ceiling << " nodes maximum).";
        if (GetEditionType() == EditionType::COMMUNITY) {
            msg << " Upgrade to Enterprise or Hyperscaler for higher limits.";
        }
        error_out = msg.str();
        return false;
    }

    // Step 2: Consult installed shard-limit policy (if any).
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        if (shard_policy_) {
            if (!shard_policy_->canExpand(requested_nodes)) {
                std::ostringstream msg;
                msg << "Requested node count (" << requested_nodes
                    << ") exceeds the active shard-limit policy bound ("
                    << effective_shard_nodes_ << " nodes).";
                error_out = msg.str();
                return false;
            }
            return true;
        }
    }

    // Step 3: No policy installed — use compile-time default.
    if (ceiling < 0) {
        // Unlimited (HYPERSCALER)
        return true;
    }
    if (requested_nodes <= ceiling) {
        return true;
    }
    std::ostringstream msg;
    msg << "Requested node count (" << requested_nodes
        << ") exceeds the limit for the " << EDITION_STRING
        << " edition (" << ceiling << " nodes maximum).";
    if (GetEditionType() == EditionType::COMMUNITY) {
        msg << " Upgrade to Enterprise or Hyperscaler for higher limits.";
    }
    error_out = msg.str();
    return false;
}

bool EditionManager::checkVRAMLimit(int requested_vram_gb,
                                    std::string& error_out) const {
    const int ceiling = GPU_MAX_VRAM_GB;  // compile-time, absolute ceiling

    // Step 1: Compile-time ceiling (Defense in Depth — never bypassed).
    if (ceiling >= 0 && requested_vram_gb > ceiling) {
        std::ostringstream msg;
        msg << "Requested GPU VRAM (" << requested_vram_gb
            << " GB) exceeds the compile-time ceiling for the " << EDITION_STRING
            << " edition (" << ceiling << " GB maximum).";
        if (GetEditionType() == EditionType::COMMUNITY) {
            msg << " Upgrade to Enterprise or Hyperscaler for higher GPU VRAM limits.";
        }
        error_out = msg.str();
        return false;
    }

    // Step 2: Consult installed VRAM policy (if any).
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        if (vram_policy_) {
            const size_t requested_bytes =
                static_cast<size_t>(requested_vram_gb) * 1024ULL * 1024ULL * 1024ULL;
            if (!vram_policy_->canAllocate(requested_bytes)) {
                std::ostringstream msg;
                msg << "Requested GPU VRAM (" << requested_vram_gb
                    << " GB) exceeds the active VRAM policy bound ("
                    << effective_vram_gb_ << " GB).";
                error_out = msg.str();
                return false;
            }
            return true;
        }
    }

    // Step 3: No policy installed — use compile-time default.
    if (ceiling < 0) {
        // Unlimited (HYPERSCALER)
        return true;
    }
    if (requested_vram_gb <= ceiling) {
        return true;
    }
    std::ostringstream msg;
    msg << "Requested GPU VRAM (" << requested_vram_gb
        << " GB) exceeds the limit for the " << EDITION_STRING
        << " edition (" << ceiling << " GB maximum).";
    if (GetEditionType() == EditionType::COMMUNITY) {
        msg << " Upgrade to Enterprise or Hyperscaler for higher GPU VRAM limits.";
    }
    error_out = msg.str();
    return false;
}

// ============================================================================
// Edition information
// ============================================================================

EditionType EditionManager::getEditionType() const noexcept {
    return GetEditionType();
}

std::string_view EditionManager::getEditionName() const noexcept {
    return EDITION_STRING;
}

int EditionManager::getMaxNodes() const noexcept {
    return SHARDING_MAX_NODES;
}

int EditionManager::getMaxVRAMGB() const noexcept {
    return GPU_MAX_VRAM_GB;
}

// ============================================================================
// Feature enumeration
// ============================================================================

std::vector<std::string> EditionManager::getAvailableFeatures() const {
    std::vector<std::string> result;
    for (std::string_view feat : kGatedFeatureNames) {
        if (isFeatureAvailable(feat)) {
            result.emplace_back(feat);
        }
    }
    return result;
}

std::vector<std::string> EditionManager::getUnavailableFeatures() const {
    std::vector<std::string> result;
    for (std::string_view feat : kGatedFeatureNames) {
        if (!isFeatureAvailable(feat)) {
            result.emplace_back(feat);
        }
    }
    return result;
}

// ============================================================================
// Upgrade guidance
// ============================================================================

std::string EditionManager::getUpgradeMessage(std::string_view feature_name) const {
    if (isFeatureAvailable(feature_name)) {
        return {};
    }

    std::ostringstream msg;
    msg << "Feature '" << feature_name << "' is not available in the "
        << EDITION_STRING << " edition.";

    switch (GetEditionType()) {
        case EditionType::COMMUNITY:
            msg << " This feature requires Enterprise or Hyperscaler Edition."
                   " Visit https://themisdb.io/upgrade for licensing options.";
            break;
        case EditionType::ENTERPRISE:
            msg << " Please verify your license is active and has not expired."
                   " Contact support@themisdb.io for assistance.";
            break;
        default:
            msg << " Please contact your license provider.";
            break;
    }

    return msg.str();
}

// ============================================================================
// Dynamic feature-flag overrides
// ============================================================================

void EditionManager::setFeatureOverride(std::string_view feature_name, bool enabled) {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    overrides_[std::string(feature_name)] = enabled;
}

void EditionManager::clearFeatureOverride(std::string_view feature_name) {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    overrides_.erase(std::string(feature_name));
}

void EditionManager::clearAllFeatureOverrides() {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    overrides_.clear();
}

bool EditionManager::hasFeatureOverride(std::string_view feature_name) const {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    return overrides_.find(std::string(feature_name)) != overrides_.end();
}

std::optional<bool> EditionManager::getFeatureOverride(
        std::string_view feature_name) const {
    std::lock_guard<std::mutex> lock(overrides_mutex_);
    auto it = overrides_.find(std::string(feature_name));
    if (it == overrides_.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ============================================================================
// Runtime resource-limit policies
// ============================================================================

bool EditionManager::installVRAMPolicy(std::shared_ptr<gpu::IVRAMPolicy> policy,
                                       int claimed_max_vram_gb)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installVRAMPolicy: null policy rejected.");
        return false;
    }

    // Defense in Depth: reject if the claimed limit exceeds the compile-time ceiling.
    // A ceiling of -1 means Hyperscaler (unlimited) — always accept.
    if (GPU_MAX_VRAM_GB >= 0 && claimed_max_vram_gb > GPU_MAX_VRAM_GB) {
        THEMIS_WARN(
            "EditionManager::installVRAMPolicy: claimed limit {} GB exceeds "
            "compile-time ceiling {} GB for edition '{}'. Policy rejected.",
            claimed_max_vram_gb, GPU_MAX_VRAM_GB, EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    vram_policy_     = std::move(policy);
    effective_vram_gb_ = claimed_max_vram_gb;
    THEMIS_INFO(
        "EditionManager::installVRAMPolicy: VRAM policy installed "
        "(effective limit: {} GB).",
        effective_vram_gb_);
    return true;
}

bool EditionManager::installShardPolicy(
    std::shared_ptr<sharding::IShardLimitPolicy> policy,
    int claimed_max_nodes)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installShardPolicy: null policy rejected.");
        return false;
    }

    // Defense in Depth: reject if the claimed limit exceeds the compile-time ceiling.
    if (SHARDING_MAX_NODES >= 0 && claimed_max_nodes > SHARDING_MAX_NODES) {
        THEMIS_WARN(
            "EditionManager::installShardPolicy: claimed limit {} nodes exceeds "
            "compile-time ceiling {} nodes for edition '{}'. Policy rejected.",
            claimed_max_nodes, SHARDING_MAX_NODES, EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    shard_policy_          = std::move(policy);
    effective_shard_nodes_ = claimed_max_nodes;
    THEMIS_INFO(
        "EditionManager::installShardPolicy: shard-limit policy installed "
        "(effective limit: {} nodes).",
        effective_shard_nodes_);
    return true;
}

void EditionManager::clearVRAMPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    vram_policy_.reset();
    effective_vram_gb_ = -2;
    THEMIS_INFO("EditionManager::clearVRAMPolicy: VRAM policy removed; "
                "reverted to compile-time default.");
}

void EditionManager::clearShardPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    shard_policy_.reset();
    effective_shard_nodes_ = -2;
    THEMIS_INFO("EditionManager::clearShardPolicy: shard-limit policy removed; "
                "reverted to compile-time default.");
}

// ============================================================================
// Group 4: LLM resource policy
// ============================================================================

bool EditionManager::installLLMResourcePolicy(
    std::shared_ptr<llm::ILLMResourcePolicy> policy,
    int64_t claimed_max_context_tokens,
    int32_t claimed_max_model_instances,
    int64_t claimed_max_vram_per_model_mb)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installLLMResourcePolicy: null policy rejected.");
        return false;
    }

    // Defense in Depth: 0 = unlimited ceiling → always accepted.
    if (LLM_MAX_CONTEXT_TOKENS > 0 &&
        claimed_max_context_tokens > LLM_MAX_CONTEXT_TOKENS) {
        THEMIS_WARN(
            "EditionManager::installLLMResourcePolicy: claimed context tokens {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_context_tokens, LLM_MAX_CONTEXT_TOKENS, EDITION_STRING);
        return false;
    }
    if (LLM_MAX_MODEL_INSTANCES >= 0 &&
        claimed_max_model_instances > LLM_MAX_MODEL_INSTANCES) {
        THEMIS_WARN(
            "EditionManager::installLLMResourcePolicy: claimed model instances {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_model_instances, LLM_MAX_MODEL_INSTANCES, EDITION_STRING);
        return false;
    }
    if (LLM_MAX_VRAM_PER_MODEL_MB > 0 &&
        claimed_max_vram_per_model_mb > LLM_MAX_VRAM_PER_MODEL_MB) {
        THEMIS_WARN(
            "EditionManager::installLLMResourcePolicy: claimed VRAM per model {} MiB "
            "exceeds compile-time ceiling {} MiB for edition '{}'. Policy rejected.",
            claimed_max_vram_per_model_mb, LLM_MAX_VRAM_PER_MODEL_MB, EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    llm_resource_policy_ = std::move(policy);
    THEMIS_INFO(
        "EditionManager::installLLMResourcePolicy: LLM resource policy installed "
        "(context_tokens={}, model_instances={}, vram_per_model_mb={}).",
        claimed_max_context_tokens, claimed_max_model_instances,
        claimed_max_vram_per_model_mb);
    return true;
}

void EditionManager::clearLLMResourcePolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    llm_resource_policy_.reset();
    THEMIS_INFO("EditionManager::clearLLMResourcePolicy: LLM resource policy removed; "
                "reverted to compile-time default.");
}

// ============================================================================
// Group 1+3: Tenant quota policy
// ============================================================================

bool EditionManager::installTenantQuotaPolicy(
    std::shared_ptr<tenant::ITenantQuotaPolicy> policy,
    uint64_t claimed_max_storage_bytes,
    uint64_t claimed_max_documents,
    uint32_t claimed_max_collections,
    uint32_t claimed_max_concurrent_queries,
    uint32_t claimed_max_rps)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installTenantQuotaPolicy: null policy rejected.");
        return false;
    }

    // Defense in Depth: 0 = unlimited ceiling → always accepted.
    if (TENANT_MAX_STORAGE_BYTES > 0 &&
        claimed_max_storage_bytes > TENANT_MAX_STORAGE_BYTES) {
        THEMIS_WARN(
            "EditionManager::installTenantQuotaPolicy: claimed storage {} bytes "
            "exceeds compile-time ceiling for edition '{}'. Policy rejected.",
            claimed_max_storage_bytes, EDITION_STRING);
        return false;
    }
    if (TENANT_MAX_DOCUMENTS > 0 && claimed_max_documents > TENANT_MAX_DOCUMENTS) {
        THEMIS_WARN(
            "EditionManager::installTenantQuotaPolicy: claimed documents {} "
            "exceeds compile-time ceiling for edition '{}'. Policy rejected.",
            claimed_max_documents, EDITION_STRING);
        return false;
    }
    if (TENANT_MAX_COLLECTIONS > 0 &&
        claimed_max_collections > TENANT_MAX_COLLECTIONS) {
        THEMIS_WARN(
            "EditionManager::installTenantQuotaPolicy: claimed collections {} "
            "exceeds compile-time ceiling for edition '{}'. Policy rejected.",
            claimed_max_collections, EDITION_STRING);
        return false;
    }
    if (TENANT_MAX_CONCURRENT_QUERIES > 0 &&
        claimed_max_concurrent_queries > TENANT_MAX_CONCURRENT_QUERIES) {
        THEMIS_WARN(
            "EditionManager::installTenantQuotaPolicy: claimed concurrent queries {} "
            "exceeds compile-time ceiling for edition '{}'. Policy rejected.",
            claimed_max_concurrent_queries, EDITION_STRING);
        return false;
    }
    if (TENANT_MAX_REQUESTS_PER_SECOND > 0 &&
        claimed_max_rps > TENANT_MAX_REQUESTS_PER_SECOND) {
        THEMIS_WARN(
            "EditionManager::installTenantQuotaPolicy: claimed RPS {} "
            "exceeds compile-time ceiling for edition '{}'. Policy rejected.",
            claimed_max_rps, EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    tenant_quota_policy_ = std::move(policy);
    THEMIS_INFO(
        "EditionManager::installTenantQuotaPolicy: tenant quota policy installed "
        "(storage_bytes={}, documents={}, collections={}, queries={}, rps={}).",
        claimed_max_storage_bytes, claimed_max_documents, claimed_max_collections,
        claimed_max_concurrent_queries, claimed_max_rps);
    return true;
}

void EditionManager::clearTenantQuotaPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    tenant_quota_policy_.reset();
    THEMIS_INFO("EditionManager::clearTenantQuotaPolicy: tenant quota policy removed; "
                "reverted to compile-time default.");
}

// ============================================================================
// Group 2: Query limit policy
// ============================================================================

bool EditionManager::installQueryLimitPolicy(
    std::shared_ptr<query::IQueryLimitPolicy> policy,
    uint32_t claimed_max_depth,
    uint32_t claimed_max_complexity,
    uint64_t claimed_max_payload_bytes,
    uint64_t claimed_max_result_rows)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installQueryLimitPolicy: null policy rejected.");
        return false;
    }

    if (QUERY_MAX_GRAPHQL_DEPTH > 0 && claimed_max_depth > QUERY_MAX_GRAPHQL_DEPTH) {
        THEMIS_WARN(
            "EditionManager::installQueryLimitPolicy: claimed depth {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_depth, QUERY_MAX_GRAPHQL_DEPTH, EDITION_STRING);
        return false;
    }
    if (QUERY_MAX_GRAPHQL_COMPLEXITY > 0 &&
        claimed_max_complexity > QUERY_MAX_GRAPHQL_COMPLEXITY) {
        THEMIS_WARN(
            "EditionManager::installQueryLimitPolicy: claimed complexity {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_complexity, QUERY_MAX_GRAPHQL_COMPLEXITY, EDITION_STRING);
        return false;
    }
    if (QUERY_MAX_PAYLOAD_BYTES > 0 &&
        claimed_max_payload_bytes > QUERY_MAX_PAYLOAD_BYTES) {
        THEMIS_WARN(
            "EditionManager::installQueryLimitPolicy: claimed payload {} bytes "
            "exceeds compile-time ceiling for edition '{}'. Policy rejected.",
            claimed_max_payload_bytes, EDITION_STRING);
        return false;
    }
    if (QUERY_MAX_RESULT_ROWS > 0 && claimed_max_result_rows > QUERY_MAX_RESULT_ROWS) {
        THEMIS_WARN(
            "EditionManager::installQueryLimitPolicy: claimed result rows {} "
            "exceeds compile-time ceiling for edition '{}'. Policy rejected.",
            claimed_max_result_rows, EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    query_limit_policy_ = std::move(policy);
    THEMIS_INFO(
        "EditionManager::installQueryLimitPolicy: query limit policy installed "
        "(depth={}, complexity={}, payload_bytes={}, result_rows={}).",
        claimed_max_depth, claimed_max_complexity,
        claimed_max_payload_bytes, claimed_max_result_rows);
    return true;
}

void EditionManager::clearQueryLimitPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    query_limit_policy_.reset();
    THEMIS_INFO("EditionManager::clearQueryLimitPolicy: query limit policy removed; "
                "reverted to compile-time default.");
}

// ============================================================================
// Group 2+3: Connection policy
// ============================================================================

bool EditionManager::installConnectionPolicy(
    std::shared_ptr<network::IConnectionPolicy> policy,
    uint32_t claimed_max_http2_streams,
    uint32_t claimed_max_sse_connections,
    uint32_t claimed_max_total_connections,
    uint32_t claimed_max_sse_events_per_sec)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installConnectionPolicy: null policy rejected.");
        return false;
    }

    if (CONNECTION_MAX_HTTP2_STREAMS > 0 &&
        claimed_max_http2_streams > CONNECTION_MAX_HTTP2_STREAMS) {
        THEMIS_WARN(
            "EditionManager::installConnectionPolicy: claimed HTTP/2 streams {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_http2_streams, CONNECTION_MAX_HTTP2_STREAMS, EDITION_STRING);
        return false;
    }
    if (CONNECTION_MAX_SSE_CONNECTIONS > 0 &&
        claimed_max_sse_connections > CONNECTION_MAX_SSE_CONNECTIONS) {
        THEMIS_WARN(
            "EditionManager::installConnectionPolicy: claimed SSE connections {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_sse_connections, CONNECTION_MAX_SSE_CONNECTIONS, EDITION_STRING);
        return false;
    }
    if (CONNECTION_MAX_TOTAL > 0 &&
        claimed_max_total_connections > CONNECTION_MAX_TOTAL) {
        THEMIS_WARN(
            "EditionManager::installConnectionPolicy: claimed total connections {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_total_connections, CONNECTION_MAX_TOTAL, EDITION_STRING);
        return false;
    }
    if (CONNECTION_MAX_SSE_EVENTS_PER_SEC > 0 &&
        claimed_max_sse_events_per_sec > CONNECTION_MAX_SSE_EVENTS_PER_SEC) {
        THEMIS_WARN(
            "EditionManager::installConnectionPolicy: claimed SSE events/sec {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_sse_events_per_sec, CONNECTION_MAX_SSE_EVENTS_PER_SEC,
            EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    connection_policy_ = std::move(policy);
    THEMIS_INFO(
        "EditionManager::installConnectionPolicy: connection policy installed "
        "(http2_streams={}, sse_connections={}, total_connections={}, sse_events/s={}).",
        claimed_max_http2_streams, claimed_max_sse_connections,
        claimed_max_total_connections, claimed_max_sse_events_per_sec);
    return true;
}

void EditionManager::clearConnectionPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    connection_policy_.reset();
    THEMIS_INFO("EditionManager::clearConnectionPolicy: connection policy removed; "
                "reverted to compile-time default.");
}

// ============================================================================
// Group 5: Storage operations policy
// ============================================================================

bool EditionManager::installStorageOpsPolicy(
    std::shared_ptr<storage::IStorageOpsPolicy> policy,
    int32_t  claimed_max_background_jobs,
    uint64_t claimed_max_compaction_bytes_per_sec,
    int32_t  claimed_max_concurrent_snapshots)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installStorageOpsPolicy: null policy rejected.");
        return false;
    }

    // -1 = unlimited ceiling for counts; 0 = unlimited for rates.
    if (STORAGE_MAX_BACKGROUND_JOBS >= 0 &&
        claimed_max_background_jobs > STORAGE_MAX_BACKGROUND_JOBS) {
        THEMIS_WARN(
            "EditionManager::installStorageOpsPolicy: claimed background jobs {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_background_jobs, STORAGE_MAX_BACKGROUND_JOBS, EDITION_STRING);
        return false;
    }
    if (STORAGE_MAX_COMPACTION_BYTES_PER_SEC > 0 &&
        claimed_max_compaction_bytes_per_sec > STORAGE_MAX_COMPACTION_BYTES_PER_SEC) {
        THEMIS_WARN(
            "EditionManager::installStorageOpsPolicy: claimed compaction rate {} B/s "
            "exceeds compile-time ceiling for edition '{}'. Policy rejected.",
            claimed_max_compaction_bytes_per_sec, EDITION_STRING);
        return false;
    }
    if (STORAGE_MAX_CONCURRENT_SNAPSHOTS >= 0 &&
        claimed_max_concurrent_snapshots > STORAGE_MAX_CONCURRENT_SNAPSHOTS) {
        THEMIS_WARN(
            "EditionManager::installStorageOpsPolicy: claimed snapshots {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_concurrent_snapshots, STORAGE_MAX_CONCURRENT_SNAPSHOTS,
            EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    storage_ops_policy_ = std::move(policy);
    THEMIS_INFO(
        "EditionManager::installStorageOpsPolicy: storage ops policy installed "
        "(background_jobs={}, compaction_bytes/s={}, snapshots={}).",
        claimed_max_background_jobs, claimed_max_compaction_bytes_per_sec,
        claimed_max_concurrent_snapshots);
    return true;
}

void EditionManager::clearStorageOpsPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    storage_ops_policy_.reset();
    THEMIS_INFO("EditionManager::clearStorageOpsPolicy: storage ops policy removed; "
                "reverted to compile-time default.");
}

// ============================================================================
// Group 3: Global rate-limit policy
// ============================================================================

bool EditionManager::installRateLimitPolicy(
    std::shared_ptr<ratelimit::IRateLimitPolicy> policy,
    uint64_t claimed_max_global_rps)
{
    if (!policy) {
        THEMIS_WARN("EditionManager::installRateLimitPolicy: null policy rejected.");
        return false;
    }

    if (RATE_LIMIT_MAX_GLOBAL_RPS > 0 &&
        claimed_max_global_rps > RATE_LIMIT_MAX_GLOBAL_RPS) {
        THEMIS_WARN(
            "EditionManager::installRateLimitPolicy: claimed global RPS {} "
            "exceeds compile-time ceiling {} for edition '{}'. Policy rejected.",
            claimed_max_global_rps, RATE_LIMIT_MAX_GLOBAL_RPS, EDITION_STRING);
        return false;
    }

    std::lock_guard<std::mutex> lock(policy_mutex_);
    rate_limit_policy_ = std::move(policy);
    THEMIS_INFO(
        "EditionManager::installRateLimitPolicy: global rate-limit policy installed "
        "(max_global_rps={}).",
        claimed_max_global_rps);
    return true;
}

void EditionManager::clearRateLimitPolicy() {
    std::lock_guard<std::mutex> lock(policy_mutex_);
    rate_limit_policy_.reset();
    THEMIS_INFO("EditionManager::clearRateLimitPolicy: global rate-limit policy removed; "
                "reverted to compile-time default.");
}

} // namespace edition
} // namespace themis
