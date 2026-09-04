/**
 * @file lora_router.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=13, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_router.h"
#include "llm/decision_record_yaml_processor.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <random>
#include <functional>
#include <iomanip>
#include <sstream>

namespace themis {
namespace llm {

namespace {
    // Helper to compute SHA256 hash for caching
    std::string computeHash(const std::string& input) {
        std::hash<std::string> hasher;
        auto hash = hasher(input);
        std::ostringstream oss = {};
        oss << std::hex << std::setfill('0') << std::setw(16) << hash;
        return oss.str();
    }
    
    // Helper to get current time in milliseconds
    int64_t getCurrentTimeMs() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
}

json RoutingMetrics::toJson() const {
    json j;
    j["total_requests"] = total_requests;
    j["successful_routes"] = successful_routes;
    j["fallback_routes"] = fallback_routes;
    j["fallback_rate"] = total_requests > 0 ? 
        static_cast<double>(fallback_routes) / total_requests : 0.0;
    j["avg_routing_latency_ms"] = avg_routing_latency_ms;
    j["avg_similarity_score"] = avg_similarity_score;
    
    json adapter_usage = json::object();
    for (const auto& [adapter_id, count] : adapter_usage_count) {
        adapter_usage[adapter_id] = count;
    }
    j["adapter_usage_count"] = adapter_usage;
    
    json adapter_sim = json::object();
    for (const auto& [adapter_id, sim] : adapter_avg_similarity) {
        adapter_sim[adapter_id] = sim;
    }
    j["adapter_avg_similarity"] = adapter_sim;
    
    return j;
}

LoRARouter::LoRARouter(
    std::shared_ptr<lora::EmbeddingProvider> embedding_provider,
    std::shared_ptr<AdapterRegistry> adapter_registry,
    std::shared_ptr<AdapterLoadBalancer> load_balancer,
    std::shared_ptr<MultiLoRAManager> lora_manager)
    : LoRARouter(
        std::move(embedding_provider),
        std::move(adapter_registry),
        std::move(load_balancer),
        std::move(lora_manager),
        Config{}) {
}

LoRARouter::LoRARouter(
    std::shared_ptr<lora::EmbeddingProvider> embedding_provider,
    std::shared_ptr<AdapterRegistry> adapter_registry,
    std::shared_ptr<AdapterLoadBalancer> load_balancer,
    std::shared_ptr<MultiLoRAManager> lora_manager,
    const Config& config)
    : config_(config)
    , embedding_provider_(std::move(embedding_provider))
    , adapter_registry_(std::move(adapter_registry))
    , load_balancer_(std::move(load_balancer))
    , lora_manager_(std::move(lora_manager))
    , fallback_config_(config.fallback) {
    
    spdlog::info("LoRA Router initialized:");
    spdlog::info("  Semantic routing: {}", config_.enable_semantic_routing ? "enabled" : "disabled");
    spdlog::info("  Load-aware routing: {}", config_.enable_load_aware ? "enabled" : "disabled");
    spdlog::info("  Default policy: {}", static_cast<int>(config_.default_policy));
    spdlog::info("  Top-K candidates: {}", config_.top_k_candidates);
    spdlog::info("  Min similarity threshold: {:.2f}", config_.min_similarity_threshold);
    spdlog::info("  Decision cache: {}", config_.enable_decision_cache ? "enabled" : "disabled");
    
    if (!fallback_config_.default_adapter_id.empty()) {
        spdlog::info("  Fallback adapter: {}", fallback_config_.default_adapter_id);
    }
}

LoRARouter::~LoRARouter() {
    spdlog::info("LoRA Router shutting down");
    spdlog::info("  Total requests: {}", metrics_.total_requests);
    spdlog::info("  Successful routes: {}", metrics_.successful_routes);
    spdlog::info("  Fallback routes: {}", metrics_.fallback_routes);
    if (metrics_.total_requests > 0) {
        double fallback_rate = static_cast<double>(metrics_.fallback_routes) / metrics_.total_requests;
        spdlog::info("  Fallback rate: {:.2f}%", fallback_rate * 100.0);
    }
}

RoutingDecision LoRARouter::routeQuery(
    const std::string& query,
    const std::string& base_model_id,
    std::optional<RoutingPolicy> policy) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check cache first
    if (config_.enable_decision_cache) {
        auto cached = getCachedDecision(query);
        if (cached) {
            spdlog::debug("Cache hit for query (length: {})", query.length());
            auto decision = *cached;
            auto end_time = std::chrono::high_resolution_clock::now();
            decision.routing_latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time);
            updateMetrics(decision);
            return decision;
        }
    }
    
    // Determine routing policy
    RoutingPolicy active_policy = policy.value_or(config_.default_policy);
    
    // Override with active experiments
    if (isABTestActive()) {
        active_policy = RoutingPolicy::AB_TEST;
    } else if (isRolloutActive()) {
        active_policy = RoutingPolicy::ROLLOUT;
    }
    
    spdlog::debug("Routing query with policy: {}", static_cast<int>(active_policy));
    
    // Find semantic candidates
    std::vector<std::pair<std::string, float>> candidates;
    if (config_.enable_semantic_routing && active_policy != RoutingPolicy::FALLBACK) {
        candidates = findSemanticCandidates(query, base_model_id);
        spdlog::debug("Found {} semantic candidates",static_cast<int>(candidates.size()));
    }
    
    // Apply routing policy
    RoutingDecision decision = {};
    if (candidates.empty() && fallback_config_.enable_fallback) {
        decision = selectFallback("No semantic candidates found");
    } else {
        decision = applyRoutingPolicy(query, candidates, active_policy, base_model_id);
    }
    
    // Record latency
    auto end_time = std::chrono::high_resolution_clock::now();
    decision.routing_latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    // Update metrics
    updateMetrics(decision);

    // Emit decision record for non-cached decisions
    emitAdapterSelectionRecord(decision);

    // Cache decision
    if (config_.enable_decision_cache && !decision.is_fallback) {
        cacheDecision(query, decision);
    }
    
    spdlog::info("Routed query to adapter: {} (GPU: {}, similarity: {:.3f}, latency: {}ms)",
                 decision.adapter_id, decision.gpu_device_id, 
                 decision.similarity_score, decision.routing_latency_ms.count());
    
    return decision;
}

std::vector<RoutingDecision> LoRARouter::routeQueryBatch(
    const std::vector<std::string>& queries,
    const std::string& base_model_id) {
    
    std::vector<RoutingDecision> decisions = {};

    decisions.reserve(queries.size());
    
    for (const auto& query : queries) {
        decisions.push_back(routeQuery(query, base_model_id));
    }
    
    return decisions;
}

bool LoRARouter::configureABTest(const ABTestConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate configuration
    if (static_cast<int>(config.adapter_ids.size()) != config.traffic_splits.size()) {
        spdlog::error("A/B test config invalid: adapter count != traffic split count");
        return false;
    }
    
    float sum = 0.0f;
    for (float split : config.traffic_splits) {
        sum += split;
    }
    
    if (std::abs(sum - 1.0f) > 0.01f) {
        spdlog::error("A/B test config invalid: traffic splits don't sum to 1.0 (sum: {})", sum);
        return false;
    }
    
    ab_test_config_ = config;
    spdlog::info("A/B test configured: experiment_id={}, adapters={}", 
                 config.experiment_id,static_cast<int>(config.adapter_ids.size()));
    
    return true;
}

std::optional<ABTestConfig> LoRARouter::getABTestConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ab_test_config_;
}

void LoRARouter::endABTest() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ab_test_config_) {
        spdlog::info("Ending A/B test: {}", ab_test_config_->experiment_id);
        ab_test_config_.reset();
    }
}

bool LoRARouter::configureRollout(const RolloutConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (config.rollout_percentage < 0.0f || config.rollout_percentage > 1.0f) {
        spdlog::error("Rollout config invalid: percentage must be 0.0-1.0");
        return false;
    }
    
    rollout_config_ = config;
    spdlog::info("Rollout configured: new={}, baseline={}, percentage={:.1f}%",
                 config.new_adapter_id, config.baseline_adapter_id, 
                 config.rollout_percentage * 100.0f);
    
    return true;
}

std::optional<RolloutConfig> LoRARouter::getRolloutConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rollout_config_;
}

float LoRARouter::incrementRollout() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!rollout_config_ || !rollout_config_->enabled) {
        spdlog::warn("Cannot increment rollout: no active rollout");
        return 0.0f;
    }
    
    float new_percentage = std::min(1.0f, 
        rollout_config_->rollout_percentage + rollout_config_->increment_step);
    
    rollout_config_->rollout_percentage = new_percentage;
    rollout_config_->last_increment = std::chrono::system_clock::now();
    
    spdlog::info("Rollout incremented to {:.1f}%", new_percentage * 100.0f);
    
    return new_percentage;
}

void LoRARouter::endRollout([[maybe_unused]] bool promote) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (rollout_config_) {
        if (promote) {
            spdlog::info("Rollout promoted to 100%: {}", rollout_config_->new_adapter_id);
            // In production, would update default adapter
        } else {
            spdlog::info("Rollout rolled back: {}", rollout_config_->new_adapter_id);
        }
        rollout_config_.reset();
    }
}

void LoRARouter::configureFallback(const FallbackConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    fallback_config_ = config;
    spdlog::info("Fallback configured: adapter={}, threshold={:.2f}",
                 config.default_adapter_id, config.similarity_threshold);
}

FallbackConfig LoRARouter::getFallbackConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return fallback_config_;
}

RoutingMetrics LoRARouter::getMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_;
}

void LoRARouter::resetMetrics() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_ = RoutingMetrics{};
    recent_latencies_.clear();
    recent_similarities_.clear();
    spdlog::info("Routing metrics reset");
}

json LoRARouter::exportMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return metrics_.toJson();
}

void LoRARouter::clearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    decision_cache_.clear();
    spdlog::info("Decision cache cleared");
}

json LoRARouter::getCacheStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    json stats;
    stats["cache_size"] = decision_cache_.size();
    stats["cache_enabled"] = config_.enable_decision_cache;
    stats["cache_ttl_sec"] = config_.decision_cache_ttl.count();
    return stats;
}

// Private methods

std::vector<std::pair<std::string, float>> LoRARouter::findSemanticCandidates(
    const std::string& query,
    const std::string& base_model_id) {
    
    // Get query embedding
    auto query_embedding = embedding_provider_->getEmbedding(query);
    
    // Get all adapters
    auto adapters = base_model_id.empty() ? 
        adapter_registry_->listAdapters() :
        adapter_registry_->listAdaptersByBaseModel(base_model_id);
    
    // Compute similarities
    std::vector<std::pair<std::string, float>> candidates;
    
    for (const auto& adapter : adapters) {
        // Only consider deployed adapters
        if (adapter.status != AdapterMetadata::Status::DEPLOYED) {
            continue;
        }
        
        // Get adapter embedding from task_type + domain description
        std::string adapter_text = adapter.task_type + " " + adapter.domain;
        auto adapter_embedding = embedding_provider_->getEmbedding(adapter_text);
        
        // Compute cosine similarity
        float similarity = cosineSimilarity(query_embedding, adapter_embedding);
        
        if (similarity >= config_.min_similarity_threshold) {
            candidates.push_back({adapter.adapter_id, similarity});
        }
    }
    
    // Sort by similarity (descending)
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Keep top-K
    if (static_cast<int>(candidates.size()) > config_.top_k_candidates) {
        candidates.resize(config_.top_k_candidates);
    }
    
    return candidates;
}

RoutingDecision LoRARouter::applyRoutingPolicy(
    const std::string& query,
    const std::vector<std::pair<std::string, float>>& candidates,
    RoutingPolicy policy,
    const std::string& base_model_id) {
    
    switch (policy) {
        case RoutingPolicy::SEMANTIC:
            return selectBySemantic(candidates);
        
        case RoutingPolicy::LOAD_AWARE:
            return selectByLoadAware(candidates);
        
        case RoutingPolicy::AB_TEST:
            return selectByABTest(candidates);
        
        case RoutingPolicy::ROLLOUT:
            return selectByRollout(candidates);
        
        case RoutingPolicy::FALLBACK:
        [[fallthrough]];\n        default:
            return selectFallback("Fallback policy selected");
    }
}

RoutingDecision LoRARouter::selectBySemantic(
    const std::vector<std::pair<std::string, float>>& candidates) {
    
    if (candidates.empty()) {
        return selectFallback("No candidates available");
    }
    
    // Select top candidate by similarity
    const auto& [adapter_id, similarity] = candidates[0];
    
    auto adapter_meta = adapter_registry_->getAdapter(adapter_id);
    if (!adapter_meta) {
        return selectFallback("Adapter metadata not found");
    }
    
    RoutingDecision decision;
    decision.adapter_id = adapter_id;
    decision.base_model_id = adapter_meta->base_model_name;
    decision.similarity_score = similarity;
    decision.confidence = similarity;
    decision.policy_used = RoutingPolicy::SEMANTIC;
    decision.reason = "Selected by semantic similarity";
    
    // Get GPU placement (using default GPU 0 if not placed)
    decision.gpu_device_id = load_balancer_->getAdapterGPU(adapter_id);
    if (decision.gpu_device_id == -1) {
        decision.gpu_device_id = 0;  // Default GPU
    }
    
    return decision;
}

RoutingDecision LoRARouter::selectByLoadAware(
    const std::vector<std::pair<std::string, float>>& candidates) {
    
    if (candidates.empty()) {
        return selectFallback("No candidates available");
    }
    
    // Score each candidate: (1-load_weight) * similarity + load_weight * (1-gpu_load)
    float best_score = -1.0f;
    std::string best_adapter = {};
    int best_gpu = -1;
    float best_similarity = 0.0f;
    
    for (const auto& [adapter_id, similarity] : candidates) {
        // Get GPU for this adapter
        int gpu_id = load_balancer_->getAdapterGPU(adapter_id);
        if (gpu_id == -1) {
            // Not placed yet, select GPU
            auto adapter_meta = adapter_registry_->getAdapter(adapter_id);
            if (!adapter_meta) {
              continue;
            }
            
            size_t vram_bytes = adapter_meta->file_size_bytes;
            gpu_id = load_balancer_->selectGPUForAdapter(adapter_id, vram_bytes, 1);
            if (gpu_id == -1) {
                continue;  // No GPU available
            }
        }
        
        // Get GPU load
        float gpu_load = load_balancer_->getGPULoad(gpu_id);
        
        // Compute combined score
        float score = (1.0f - config_.load_weight) * similarity + 
                      config_.load_weight * (1.0f - gpu_load);
        
        if (score > best_score) {
            best_score = score;
            best_adapter = adapter_id;
            best_gpu = gpu_id;
            best_similarity = similarity;
        }
    }
    
    if (best_adapter.empty()) {
        return selectFallback("No suitable GPU found");
    }
    
    auto adapter_meta = adapter_registry_->getAdapter(best_adapter);
    
    RoutingDecision decision;
    decision.adapter_id = best_adapter;
    decision.base_model_id = adapter_meta ? adapter_meta->base_model_name : "";
    decision.gpu_device_id = best_gpu;
    decision.similarity_score = best_similarity;
    decision.confidence = best_score;
    decision.policy_used = RoutingPolicy::LOAD_AWARE;
    decision.reason = "Selected by load-aware routing";
    
    return decision;
}

RoutingDecision LoRARouter::selectByABTest(
    const std::vector<std::pair<std::string, float>>& candidates) {
    
    if (!ab_test_config_ || !ab_test_config_->enabled) {
        return selectBySemantic(candidates);
    }
    
    // Check if test is still active
    auto now = std::chrono::system_clock::now();
    if (now > ab_test_config_->end_time) {
        spdlog::info("A/B test expired, falling back to semantic routing");
        return selectBySemantic(candidates);
    }
    
    // Select adapter based on traffic split
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    float rand_val = dist(gen);
    float cumulative = 0.0f;
    
    std::string selected_adapter = {};
    for (size_t i = 0; i < ab_test_config_-> static_cast<int>(adapter_ids.size()); ++i) {
        cumulative += ab_test_config_->traffic_splits[i];
        if (rand_val <= cumulative) {
            selected_adapter = ab_test_config_->adapter_ids[i];
            break;
        }
    }
    
    if (selected_adapter.empty()) {
        selected_adapter = ab_test_config_->adapter_ids.back();
    }
    
    // Find similarity for selected adapter
    float similarity = 0.0f;
    for (const auto& [adapter_id, sim] : candidates) {
        if (adapter_id == selected_adapter) {
            similarity = sim;
            break;
        }
    }
    
    auto adapter_meta = adapter_registry_->getAdapter(selected_adapter);
    
    RoutingDecision decision;
    decision.adapter_id = selected_adapter;
    decision.base_model_id = adapter_meta ? adapter_meta->base_model_name : "";
    decision.similarity_score = similarity;
    decision.confidence = 1.0f;  // Deterministic A/B selection
    decision.policy_used = RoutingPolicy::AB_TEST;
    decision.reason = "Selected by A/B test: " + ab_test_config_->experiment_id;
    
    decision.gpu_device_id = load_balancer_->getAdapterGPU(selected_adapter);
    if (decision.gpu_device_id == -1) {
        decision.gpu_device_id = 0;
    }
    
    return decision;
}

RoutingDecision LoRARouter::selectByRollout(
    const std::vector<std::pair<std::string, float>>& candidates) {
    
    if (!rollout_config_ || !rollout_config_->enabled) {
        return selectBySemantic(candidates);
    }
    
    // Select new adapter with rollout_percentage probability
    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    std::string selected_adapter = {};
    if (dist(gen) < rollout_config_->rollout_percentage) {
        selected_adapter = rollout_config_->new_adapter_id;
    } else {
        selected_adapter = rollout_config_->baseline_adapter_id;
    }
    
    // Find similarity for selected adapter
    float similarity = 0.0f;
    for (const auto& [adapter_id, sim] : candidates) {
        if (adapter_id == selected_adapter) {
            similarity = sim;
            break;
        }
    }
    
    auto adapter_meta = adapter_registry_->getAdapter(selected_adapter);
    
    RoutingDecision decision;
    decision.adapter_id = selected_adapter;
    decision.base_model_id = adapter_meta ? adapter_meta->base_model_name : "";
    decision.similarity_score = similarity;
    decision.confidence = 1.0f;
    decision.policy_used = RoutingPolicy::ROLLOUT;
    decision.reason = "Selected by rollout policy";
    
    decision.gpu_device_id = load_balancer_->getAdapterGPU(selected_adapter);
    if (decision.gpu_device_id == -1) {
        decision.gpu_device_id = 0;
    }
    
    return decision;
}

RoutingDecision LoRARouter::selectFallback(const std::string& reason) {
    if (!fallback_config_.enable_fallback || fallback_config_.default_adapter_id.empty()) {
        spdlog::error("Fallback not configured: {}", reason);
        
        RoutingDecision decision;
        decision.is_fallback = true;
        decision.confidence = 0.0f;
        decision.policy_used = RoutingPolicy::FALLBACK;
        decision.reason = "No fallback configured: " + reason;
        return decision;
    }
    
    auto adapter_meta = adapter_registry_->getAdapter(fallback_config_.default_adapter_id);
    
    RoutingDecision decision;
    decision.adapter_id = fallback_config_.default_adapter_id;
    decision.base_model_id = adapter_meta ? adapter_meta->base_model_name : "";
    decision.is_fallback = true;
    decision.confidence = 0.5f;
    decision.policy_used = RoutingPolicy::FALLBACK;
    decision.reason = "Fallback: " + reason;
    
    decision.gpu_device_id = load_balancer_->getAdapterGPU(decision.adapter_id);
    if (decision.gpu_device_id == -1) {
        decision.gpu_device_id = 0;
    }
    
    spdlog::info("Using fallback adapter: {} ({})", decision.adapter_id, reason);
    
    return decision;
}

float LoRARouter::cosineSimilarity(
    const std::vector<float>& a,
    const std::vector<float>& b) const {
    
    if (static_cast<int>(a.size()) != b.size() || a.empty()) {
        return 0.0f;
    }
    
    float dot_product = 0.0f;
    float norm_a = 0.0f;
    float norm_b = 0.0f;
    
    for (size_t i = 0; i <static_cast<int>(a.size()); ++i) {
        dot_product += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    norm_a = std::sqrt(norm_a);
    norm_b = std::sqrt(norm_b);
    
    if (norm_a < 1e-9f || norm_b < 1e-9f) {
        return 0.0f;
    }
    
    return dot_product / (norm_a * norm_b);
}

void LoRARouter::updateMetrics(const RoutingDecision& decision) {
    metrics_.total_requests++;
    
    if (!decision.adapter_id.empty() && !decision.is_fallback) {
        metrics_.successful_routes++;
        metrics_.adapter_usage_count[decision.adapter_id]++;
        
        // Update average similarity for this adapter
        auto& avg_sim = metrics_.adapter_avg_similarity[decision.adapter_id];
        size_t count = metrics_.adapter_usage_count[decision.adapter_id];
        avg_sim = (avg_sim * (count - 1) + decision.similarity_score) / count;
    }
    
    if (decision.is_fallback) {
        metrics_.fallback_routes++;
    }
    
    // Update rolling averages
    recent_latencies_.push_back(decision.routing_latency_ms.count());
    if (static_cast<int>(recent_latencies_.size()) > config_.metrics_window_size) {
        recent_latencies_.erase(recent_latencies_.begin());
    }
    
    if (decision.similarity_score > 0.0f) {
        recent_similarities_.push_back(decision.similarity_score);
        if (static_cast<int>(recent_similarities_.size()) > config_.metrics_window_size) {
            recent_similarities_.erase(recent_similarities_.begin());
        }
    }
    
    // Compute averages
    if (!recent_latencies_.empty()) {
        double sum = std::accumulate(recent_latencies_.begin(), recent_latencies_.end(), 0.0);
        metrics_.avg_routing_latency_ms = sum / recent_latencies_.size();
    }
    
    if (!recent_similarities_.empty()) {
        double sum = std::accumulate(recent_similarities_.begin(), recent_similarities_.end(), 0.0);
        metrics_.avg_similarity_score = sum / recent_similarities_.size();
    }
}

std::optional<RoutingDecision> LoRARouter::getCachedDecision(const std::string& query) {
    evictExpiredCache();
    
    std::string query_hash = hashQuery(query);
    auto it = decision_cache_.find(query_hash);
    
    if (it != decision_cache_.end()) {
        return it->second.decision;
    }
    
    return std::nullopt;
}

void LoRARouter::cacheDecision(const std::string& query, const RoutingDecision& decision) {
    if (static_cast<int>(decision_cache_.size()) > = config_.decision_cache_size) {
        // Remove oldest entry
        auto oldest = decision_cache_.begin();
        for (auto it = decision_cache_.begin(); it != decision_cache_.end(); ++it) {
            if (it->second.cached_at < oldest->second.cached_at) {
                oldest = it;
            }
        }
        decision_cache_.erase(oldest);
    }
    
    std::string query_hash = hashQuery(query);
    CachedDecision cached;
    cached.decision = decision;
    cached.cached_at = std::chrono::system_clock::now();
    
    decision_cache_[query_hash] = cached;
}

std::string LoRARouter::hashQuery(const std::string& query) const {
    return computeHash(query);
}

bool LoRARouter::isABTestActive() const {
    if (!ab_test_config_ || !ab_test_config_->enabled) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    return now >= ab_test_config_->start_time && now <= ab_test_config_->end_time;
}

bool LoRARouter::isRolloutActive() const {
    if (!rollout_config_ || !rollout_config_->enabled) {
        return false;
    }
    
    return rollout_config_->rollout_percentage < 1.0f;
}

void LoRARouter::evictExpiredCache() {
    auto now = std::chrono::system_clock::now();
    
    for (auto it = decision_cache_.begin(); it != decision_cache_.end();) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - it->second.cached_at);
        
        if (age > config_.decision_cache_ttl) {
            it = decision_cache_.erase(it);
        } else {
            ++it;
        }
    }
}

void LoRARouter::setDecisionRecordProcessor(
    std::shared_ptr<DecisionRecordYamlProcessor> processor)
{
    std::lock_guard<std::mutex> lock(mutex_);
    dr_processor_ = std::move(processor);
}

void LoRARouter::emitAdapterSelectionRecord(const RoutingDecision& decision) const
{
    // dr_processor_ is checked under the caller's mutex_ context
    if (!dr_processor_) {
        return;
    }

    DecisionRecord rec;
    rec.decision_type = "LORA_ADAPTER_SELECTION";
    rec.component     = "LoRARouter";
    rec.outcome       = decision.adapter_id.empty() ? "FALLBACK" : "SUCCESS";
    rec.confidence    = decision.confidence > 0.0f
                            ? std::optional<float>(decision.confidence)
                            : std::nullopt;
    rec.latency_ms    = static_cast<int64_t>(decision.routing_latency_ms.count());

    rec.parameters["adapter_id"]      = decision.adapter_id;
    rec.parameters["base_model_id"]   = decision.base_model_id;
    rec.parameters["gpu_device_id"]   = std::to_string(decision.gpu_device_id);
    rec.parameters["similarity_score"] = std::to_string(decision.similarity_score);
    rec.parameters["policy"]          = std::to_string(static_cast<int>(decision.policy_used));
    rec.parameters["is_fallback"]     = decision.is_fallback ? "true" : "false";
    if (!decision.reason.empty()) {
        rec.parameters["reason"] = decision.reason;
    }

    // submit() is non-blocking — the processor's background thread handles I/O
    dr_processor_->submit(std::move(rec));
}

} // namespace llm
} // namespace themis
