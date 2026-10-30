// SPDX-License-Identifier: Apache-2.0
// Copyright 2024 ThemisDB Contributors
//
// @file
// @brief Access Coordinator implementation: orchestrates cache↔storage tier transitions
// @brief Coordinates validated, bounded data transfers across registered tiers.

#include "access_model/access_coordinator.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "utils/logger.h"
#include "access_model/access_model_logging.h"
#include "access_model/access_model_trace.h"

namespace themis {
namespace access_model {

// ============================================================================
// § 1  AccessCoordinatorImpl: Central Tier Orchestrator
// ============================================================================

/** @brief Mutex-protected coordinator implementation with bounded worker admission. */
class AccessCoordinatorImpl : public AccessCoordinator {
 public:
    /** @brief Queued promotion or demotion work and its optional completion promise. */
    struct DemotionEvent {
        std::string key{};
        TierLevel from_tier = TierLevel::UNKNOWN;
        TierLevel to_tier = TierLevel::UNKNOWN;
        std::string reason{};
        std::shared_ptr<std::promise<PromotionResult>> promise{};
        uint64_t size_bytes = 0;
        bool remove_source = false;
    };

    /**
     * @brief Construct AccessCoordinatorImpl with a given background worker thread count.
     * @param[in] thread_pool_size Number of worker threads to spawn on start() (default: 4).
     */
    explicit AccessCoordinatorImpl(size_t thread_pool_size = 4,
                                   size_t max_queue_depth = 10000)
        : thread_pool_size_(thread_pool_size),
          max_queue_depth_(max_queue_depth),
          running_(false),
          initialized_(false),
          policy_set_(false) {
        if (thread_pool_size_ == 0 || max_queue_depth_ == 0) {
            throw std::invalid_argument(
                "AccessCoordinator thread and queue limits must be greater than zero");
        }
        metrics_.counters.max_queue_depth = max_queue_depth_;
    }

    /// Shut down worker threads and release resources if still running.
    ~AccessCoordinatorImpl() {
        if (running_) {
            shutdown();
        }
    }

    // Lifecycle management
    /**
     * @brief Register all tier instances and prepare the coordinator for start().
     * @param[in] tiers Map from TierLevel to the owning AccessTier instance.
     * @return true on success; false if the tier map is empty or initialization fails.
     */
    bool initialize(const std::map<TierLevel, std::shared_ptr<AccessTier>>& tiers)
        override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return false;
        }
        initialized_ = false;
        tiers_.clear();
        key_metrics_.clear();
        tier_metrics_.clear();
        pending_plans_.clear();
        if (tiers.empty()) {
            return false;
        }

        bool has_cache = false;
        bool has_storage = false;
        try {
            for (const auto& [level, tier] : tiers) {
                if (!tier || level == TierLevel::UNKNOWN ||
                    tier->getTierLevel() != level || !tier->isHealthy()) {
                    return false;
                }
                if (classifyTier(level) == TierClassification::CACHE) {
                    has_cache = true;
                } else {
                    has_storage = true;
                }
            }
        } catch (...) {
            return false;
        }
        if (!has_cache || !has_storage) {
            return false;
        }

        tiers_ = tiers;
        initialized_ = true;
        for (const auto& [level, tier] : tiers_) {
            (void)tier;
            tier_metrics_.try_emplace(level);
        }
        THEMIS_INFO("AccessCoordinator initialized with {} tiers", tiers_.size());
        return true;
    }

    void start() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
          return;
        }
        if (!initialized_) {
            throw std::logic_error(
                "AccessCoordinator must be initialized before start()");
        }

        running_ = true;
        worker_threads_.clear();

        for (size_t i = 0; i < thread_pool_size_; ++i) {
            worker_threads_.emplace_back([this] { workerMain(); });
        }

        THEMIS_INFO("AccessCoordinator started with {} worker threads",
                       thread_pool_size_);
        
        // Emit structured lifecycle log
        CoordinatorLifecycleLog lifecycle_log{
            .event_type = "START",
            .details = "worker_thread_count=" + std::to_string(thread_pool_size_),
            .correlation_id = generateCorrelationId("startup"),
            .thread_id = std::this_thread::get_id(),
            .timestamp = std::chrono::system_clock::now(),
        };
        accessModelLogger().logCoordinatorLifecycle(lifecycle_log);
    }

    void shutdown() override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ && pending_tasks_.empty()) {
              return;
            }
            running_ = false;
        }

        // Wake all threads
        condition_.notify_all();

        // Wait for threads to finish
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        worker_threads_.clear();
        THEMIS_INFO("AccessCoordinator shut down");
        
        // Emit structured lifecycle log
        CoordinatorLifecycleLog lifecycle_log{
            .event_type = "SHUTDOWN",
            .details = "queued_tasks_drained",
            .correlation_id = generateCorrelationId("shutdown"),
            .thread_id = std::this_thread::get_id(),
            .timestamp = std::chrono::system_clock::now(),
        };
        accessModelLogger().logCoordinatorLifecycle(lifecycle_log);
    }

    bool isRunning() const override { return running_; }

    // Event listening (from cache/storage)
    void onEviction(const EvictionEvent& event) override {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto event_start = std::chrono::steady_clock::now();
        const auto event_time = std::chrono::system_clock::now();
        touchKeyMetricsLocked(event.key);
        touchTierMetricsLocked(event.tier);
        auto& key_metrics = key_metrics_[event.key];
        auto& source_metrics = tier_metrics_[event.tier];
        key_metrics.recordEviction();
        source_metrics.recordEviction();

        // Fast-path: when no policy is configured yet, treat the event as a
        // lightweight observation and avoid the expensive transition/logging
        // work that is irrelevant to the current benchmark and hot path.
        if (!policy_set_) {
            metrics_.counters.cache_evictions_observed++;
            recordEventSampleLocked(event_start);
            return;
        }

        std::string correlation_id = generateCorrelationId("evict");
        // Update counters
        metrics_.counters.cache_evictions_observed++;

        // Record event
        AccessTransitionEvent transition{
            .key = std::string(event.key),
            .from_tier = event.tier,
            .to_tier = TierLevel::UNKNOWN,
            .reason = "cache_eviction",
            .timestamp = event_time,
            .latency_ms = std::chrono::milliseconds(0),
            .correlation_id = correlation_id,
            .success = true,
            .error_message = "",
        };

        // Apply age policy: decide if this should trigger storage demotion
        std::string decision = "UNKNOWN";
        if (policy_set_) {
            bool should_demote = false;
            TierLevel target_tier = TierLevel::UNKNOWN;

            // High access count → keep hot (don't demote)
            if (event.access_count >= policy_.l1_promotion_threshold) {
                should_demote = false;
                decision = "RETAIN";
            }
            // Low access count + old → demote to storage
            else if (event.last_access_age_secs.count() >
                     (policy_.hot_zero_access_days * 86400)) {
                should_demote = true;
                target_tier = TierLevel::STORAGE_WARM;
                decision = "DEMOTE";
            } else {
                decision = "DEFER";
            }

            if (should_demote && tiers_.count(target_tier)) {
                DemotionEvent task{
                    .key = std::string(event.key),
                    .from_tier = event.tier,
                    .to_tier = target_tier,
                    .reason = "cache_eviction_demotion",
                    .size_bytes = event.evicted_size_bytes,
                    .remove_source = true,
                };
                if (enqueueLocked(std::move(task))) {
                    metrics_.counters.demotions_initiated++;
                    key_metrics.demotion_count++;
                    source_metrics.demotion_count++;
                    tier_metrics_[target_tier].demotion_count++;
                } else {
                    transition.success = false;
                    transition.error_message = "Coordinator task queue is full or stopped";
                    metrics_.counters.demotions_failed++;
                }

                transition.to_tier = target_tier;
            } else if (should_demote) {
                transition.to_tier = target_tier;
                transition.success = false;
                transition.error_message = "Demotion destination tier is not registered";
                metrics_.counters.demotions_failed++;
            }
        }

        // Track metrics
        const auto elapsed = std::chrono::steady_clock::now() - event_start;
        const auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        metrics_.recordPolicyDecisionLatency(static_cast<uint64_t>(latency_us));
        metrics_.recordEventProcessingLatency(static_cast<uint64_t>(latency_us));
        transition.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        recordTransitionLocked(transition);

        THEMIS_DEBUG(
            "Cache eviction observed: key={}, tier={}, access_count={}, "
            "correlation_id={}",
            event.key, tierLevelName(event.tier), event.access_count,
            correlation_id);
        
        // Emit structured eviction log
        EvictionEventLog eviction_log{
            .key = std::string(event.key),
            .from_tier = event.tier,
            .eviction_reason = event.reason,
            .size_bytes = event.evicted_size_bytes,
            .access_count = event.access_count,
            .last_access_age = event.last_access_age_secs,
            .decision = decision,
            .correlation_id = correlation_id,
            .thread_id = std::this_thread::get_id(),
            .timestamp = event_time,
        };
        accessModelLogger().logEvictionEvent(eviction_log);
    }

    void onHotAccess(const AccessEvent& event) override {
        std::lock_guard<std::mutex> lock(mutex_);

        const auto event_start = std::chrono::steady_clock::now();
        const auto event_time = std::chrono::system_clock::now();
        touchKeyMetricsLocked(event.key);
        touchTierMetricsLocked(event.current_tier);
        auto& key_metrics = key_metrics_[event.key];
        auto& current_tier_metrics = tier_metrics_[event.current_tier];
        key_metrics.recordAccess(0);
        key_metrics.access_count = event.access_count;
        current_tier_metrics.recordAccess(0);

        // Fast-path: the coordinator may be observed before policy initialization
        // in benchmarking or bootstrap scenarios; no policy means no decision to
        // produce, so keep the path extremely cheap.
        if (!policy_set_) {
            metrics_.counters.storage_hot_accesses_observed++;
            recordEventSampleLocked(event_start);
            return;
        }

        std::string correlation_id = generateCorrelationId("hot-access");
        // Update counters
        metrics_.counters.storage_hot_accesses_observed++;

        // Record event
        AccessTransitionEvent transition{
            .key = std::string(event.key),
            .from_tier = event.current_tier,
            .to_tier = TierLevel::UNKNOWN,
            .reason = "storage_hot_access",
            .timestamp = event_time,
            .latency_ms = std::chrono::milliseconds(0),
            .correlation_id = correlation_id,
            .success = true,
            .error_message = "",
        };

        // Apply age policy: check if should promote to cache
        std::string decision = "UNKNOWN";
        std::optional<TierLevel> target_tier_opt;
        
        if (policy_set_ && event.access_count >= policy_.storage_promotion_threshold) {
            // Promotion candidate
            TierLevel target_tier = TierLevel::L3_SEMANTIC;

            // If cold tier, promote to warm first
            if (event.current_tier == TierLevel::STORAGE_COLD &&
                tiers_.count(TierLevel::STORAGE_WARM)) {
                target_tier = TierLevel::STORAGE_WARM;
            }

            if (tiers_.count(target_tier)) {
                decision = "PROMOTE";
                target_tier_opt = target_tier;

                DemotionEvent task{
                    .key = std::string(event.key),
                    .from_tier = event.current_tier,
                    .to_tier = target_tier,
                    .reason = "storage_hot_access_promotion",
                };
                if (enqueueLocked(std::move(task))) {
                    metrics_.counters.promotions_initiated++;
                    key_metrics.promotion_count++;
                    current_tier_metrics.promotion_count++;
                    tier_metrics_[target_tier].promotion_count++;
                } else {
                    transition.success = false;
                    transition.error_message = "Coordinator task queue is full or stopped";
                    metrics_.counters.promotions_failed++;
                }

                transition.to_tier = target_tier;
            } else {
                transition.to_tier = target_tier;
                transition.success = false;
                transition.error_message = "Promotion destination tier is not registered";
                metrics_.counters.promotions_failed++;
            }
        } else if (policy_set_) {
            decision = "REJECT";
        }

        // Track metrics
        const auto elapsed = std::chrono::steady_clock::now() - event_start;
        const auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
        metrics_.recordPolicyDecisionLatency(static_cast<uint64_t>(latency_us));
        metrics_.recordEventProcessingLatency(static_cast<uint64_t>(latency_us));
        transition.latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
        recordTransitionLocked(transition);

        THEMIS_DEBUG(
            "Hot access detected: key={}, tier={}, access_count={}, "
            "correlation_id={}",
            event.key, tierLevelName(event.current_tier), event.access_count,
            correlation_id);
        
        // Emit structured promotion decision log
        PromotionDecisionLog decision_log{
            .key = std::string(event.key),
            .current_tier = event.current_tier,
            .target_tier = target_tier_opt,
            .decision = decision,
            .access_count = event.access_count,
            .age_secs = event.access_window,
            .threshold_name = "storage_promotion_threshold",
            .threshold_value = policy_.storage_promotion_threshold,
            .actual_value = event.access_count,
            .reason = "hot_access_detected_on_storage_tier",
            .correlation_id = correlation_id,
            .thread_id = std::this_thread::get_id(),
            .timestamp = event_time,
        };
        accessModelLogger().logPromotionDecision(decision_log);
    }

    // Async promotion/demotion
    std::future<PromotionResult> promoteAsync(
        const std::string& key, TierLevel from_tier, TierLevel to_tier,
        uint64_t size_bytes) override {
        auto promise = std::make_shared<std::promise<PromotionResult>>();
        auto future = promise->get_future();

        {
            std::lock_guard<std::mutex> lock(mutex_);

            if (!running_ || !tiers_.count(from_tier) || !tiers_.count(to_tier) ||
                from_tier == to_tier || key.empty() ||
                pending_tasks_.size() >= max_queue_depth_) {
                const std::string error = !running_
                    ? "Coordinator not running"
                    : "Invalid promotion request or task queue is full";
                promise->set_value(PromotionResult{
                    .success = false,
                    .error_message = error,
                    .size_bytes = 0,
                    .from_tier = from_tier,
                    .to_tier = to_tier,
                    .total_latency_ms = std::chrono::milliseconds(0),
                    .correlation_id = generateCorrelationId("promo-failed"),
                    .completed_at = std::chrono::system_clock::now(),
                });
                metrics_.counters.promotions_failed++;
                return future;
            }

            std::string correlation_id = generateCorrelationId("promote");

            DemotionEvent task{
                .key = key,
                .from_tier = from_tier,
                .to_tier = to_tier,
                .reason = "explicit_promotion",
                .promise = promise,
                .size_bytes = size_bytes,
            };

            pending_tasks_.push(std::move(task));
            metrics_.counters.pending_queue_depth = pending_tasks_.size();
            metrics_.counters.promotions_initiated++;
            touchKeyMetricsLocked(key);
            touchTierMetricsLocked(from_tier);
            touchTierMetricsLocked(to_tier);
            ++key_metrics_[key].promotion_count;
            ++tier_metrics_[from_tier].promotion_count;
            ++tier_metrics_[to_tier].promotion_count;

            THEMIS_DEBUG(
                "Promotion queued: key={}, from={}, to={}, correlation_id={}",
                key, tierLevelName(from_tier), tierLevelName(to_tier),
                correlation_id);
        }

        condition_.notify_one();
        return future;
    }

    std::optional<DemotionPlan> planDemotion(
        const std::string& key, TierLevel from_tier, TierLevel to_tier,
        uint64_t data_size_bytes) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (key.empty() || from_tier == to_tier ||
            !tiers_.count(from_tier) || !tiers_.count(to_tier) ||
            pending_plans_.size() >= max_queue_depth_) {
            return std::nullopt;
        }

        std::string plan_id = generatePlanId();

        DemotionPlan plan{
            .plan_id = plan_id,
            .key = key,
            .from_tier = from_tier,
            .to_tier = to_tier,
            .reason = "coordinator_plan",
            .data_size_bytes = static_cast<std::size_t>(
                std::min<uint64_t>(data_size_bytes,
                                   std::numeric_limits<std::size_t>::max())),
            .created_at = std::chrono::system_clock::now(),
        };

        pending_plans_[plan_id] = plan;
        ++metrics_.counters.demotions_initiated;
        touchKeyMetricsLocked(key);
        touchTierMetricsLocked(from_tier);
        touchTierMetricsLocked(to_tier);
        ++key_metrics_[key].demotion_count;
        ++tier_metrics_[from_tier].demotion_count;
        ++tier_metrics_[to_tier].demotion_count;

        THEMIS_DEBUG("Demotion plan created: plan_id={}, key={}, from={}, to={}",
                       plan_id, key, tierLevelName(from_tier), tierLevelName(to_tier));

        return plan;
    }

    /**
     * @brief Execute Demotion.
     * @param[in] plan_id Identifier of the plan.
     * @return Return value.
     */
    std::optional<DemotionResult> executeDemotion(const std::string& plan_id)
        override {
        DemotionPlan plan;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = pending_plans_.find(plan_id);
            if (it == pending_plans_.end()) {
                THEMIS_WARN("Execute demotion: plan not found for plan_id {}", plan_id);
                return std::nullopt;
            }
            plan = it->second;
            pending_plans_.erase(it);
        }

        const auto start_time = std::chrono::steady_clock::now();
        std::string correlation_id = generateCorrelationId("demotion");
        const auto transfer = transferKey(
            plan.key, plan.from_tier, plan.to_tier, plan.data_size_bytes, true,
            correlation_id);
        DemotionResult result{
            .success = transfer.success,
            .error_message = transfer.error_message,
            .size_bytes = transfer.size_bytes,
            .from_tier = plan.from_tier,
            .to_tier = plan.to_tier,
            .total_latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time),
            .correlation_id = correlation_id,
            .completed_at = std::chrono::system_clock::now(),
        };

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (result.success) {
                metrics_.counters.demotions_succeeded++;
            } else {
                metrics_.counters.demotions_failed++;
            }
            recordTransitionLocked(AccessTransitionEvent{
                .key = plan.key,
                .from_tier = plan.from_tier,
                .to_tier = plan.to_tier,
                .reason = plan.reason,
                .timestamp = result.completed_at,
                .latency_ms = result.total_latency_ms,
                .correlation_id = correlation_id,
                .success = result.success,
                .error_message = result.error_message,
            });
            const auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            metrics_.recordTierDemotionLatency(static_cast<uint64_t>(latency_us));
        }

        THEMIS_INFO(
            "Demotion executed: correlation_id={}, key={}, latency_ms={}",
            correlation_id, plan.key, result.total_latency_ms.count());

        return result;
    }

    void setAgePolicy(const AgeBasedPolicy& policy) override {
        if (!policy.isValid()) {
            throw std::invalid_argument("AgeBasedPolicy thresholds are invalid");
        }
        std::lock_guard<std::mutex> lock(mutex_);
        policy_ = policy;
        policy_set_ = true;
        THEMIS_INFO("AgeBasedPolicy set: hot_to_warm_days={}",
                       policy_.hot_to_warm_days);
    }

    void setPromotionThresholds(uint64_t cache_threshold,
                                uint64_t storage_threshold) override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto candidate = policy_;
        candidate.l1_promotion_threshold = cache_threshold;
        candidate.storage_promotion_threshold = storage_threshold;
        if (!candidate.isValid()) {
            throw std::invalid_argument("Promotion thresholds make the policy invalid");
        }
        policy_.l1_promotion_threshold = cache_threshold;
        policy_.storage_promotion_threshold = storage_threshold;
    }

    AccessMetrics getKeyMetrics(const std::string& key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = key_metrics_.find(key);
        return it == key_metrics_.end() ? AccessMetrics{} : it->second;
    }

    AccessMetrics getTierMetrics(TierLevel tier) override {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = tier_metrics_.find(tier);
        return it == tier_metrics_.end() ? AccessMetrics{} : it->second;
    }

    AccessModelMetrics getAccessModelMetrics() override {
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_;
    }

    /**
     * @brief Return up to @p limit of the most recent tier transition events.
     * @param[in] limit Maximum number of events to return (default: 100).
     * @return Vector of AccessTransitionEvent records, ordered newest-first.
     */
    std::vector<AccessTransitionEvent> getRecentTransitions(size_t limit = 100)
        override {

        std::lock_guard<std::mutex> lock(mutex_);

        const auto count = std::min(limit, recent_transitions_.size());
        return std::vector<AccessTransitionEvent>(
            recent_transitions_.rbegin(), recent_transitions_.rbegin() + count);
    }

 private:
    static constexpr std::size_t kMaxRetainedKeyMetrics = 10000;
    static constexpr std::size_t kMaxRetainedTransitions = 1000;

    void touchKeyMetricsLocked(const std::string& key) {
        const auto existing = key_metrics_.find(key);
        if (existing != key_metrics_.end()) {
            existing->second.last_access_time = std::chrono::system_clock::now();
            return;
        }
        if (key_metrics_.size() >= kMaxRetainedKeyMetrics) {
            const auto oldest = std::min_element(
                key_metrics_.begin(), key_metrics_.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.second.last_access_time < rhs.second.last_access_time;
                });
            if (oldest != key_metrics_.end()) {
                key_metrics_.erase(oldest);
            }
        }
        key_metrics_.try_emplace(key);
    }

    void touchTierMetricsLocked(TierLevel tier) {
        tier_metrics_.try_emplace(tier);
    }

    void recordTransitionLocked(AccessTransitionEvent event) {
        recent_transitions_.emplace_back(std::move(event));
        if (recent_transitions_.size() > kMaxRetainedTransitions) {
            recent_transitions_.erase(recent_transitions_.begin());
        }
    }

    void recordEventSampleLocked(std::chrono::steady_clock::time_point started) {
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - started)
                .count();
        const auto sample = static_cast<uint64_t>(std::max<int64_t>(elapsed, 0));
        metrics_.recordEventProcessingLatency(sample);
    }

    bool enqueueLocked(DemotionEvent task) {
        if (!running_ || pending_tasks_.size() >= max_queue_depth_) {
            return false;
        }
        pending_tasks_.push(std::move(task));
        metrics_.counters.pending_queue_depth = pending_tasks_.size();
        condition_.notify_one();
        return true;
    }

    PromotionResult transferKey(const std::string& key, TierLevel from_tier,
                                TierLevel to_tier, uint64_t requested_size,
                                bool remove_source,
                                const std::string& correlation_id) {
        const auto started = std::chrono::steady_clock::now();
        auto make_result = [&](bool success, std::string error,
                               std::size_t size_bytes) {
            return PromotionResult{
                .success = success,
                .error_message = std::move(error),
                .size_bytes = size_bytes,
                .from_tier = from_tier,
                .to_tier = to_tier,
                .total_latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - started),
                .correlation_id = correlation_id,
                .completed_at = std::chrono::system_clock::now(),
            };
        };

        if (key.empty() || from_tier == to_tier) {
            return make_result(false, "Transfer key and distinct tiers are required", 0);
        }

        std::shared_ptr<AccessTier> source;
        std::shared_ptr<AccessTier> destination;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            const auto source_it = tiers_.find(from_tier);
            const auto destination_it = tiers_.find(to_tier);
            if (source_it == tiers_.end() || destination_it == tiers_.end()) {
                return make_result(false, "Source or destination tier is not registered", 0);
            }
            source = source_it->second;
            destination = destination_it->second;
        }

        try {
            if (!source->isHealthy() || !destination->isHealthy()) {
                return make_result(false, "Source or destination tier is unhealthy", 0);
            }

            TierAccessOptions options;
            options.correlation_id = correlation_id;
            options.target_tier = to_tier;
            const auto fetched = source->get(key, options);
            if (!fetched.success) {
                const auto error = fetched.error_message.empty()
                    ? "Source tier did not return the key"
                    : fetched.error_message;
                return make_result(false, error, 0);
            }

            const auto fallback_size = static_cast<std::size_t>(
                std::min<uint64_t>(requested_size,
                                   std::numeric_limits<std::size_t>::max()));
            const auto size_bytes =
                fetched.size_bytes == 0 ? fallback_size : fetched.size_bytes;
            const auto placed = destination->put(key, fetched.value, options);
            if (!placed.success) {
                const auto error = placed.error_message.empty()
                    ? "Destination tier rejected the value"
                    : placed.error_message;
                return make_result(false, error, size_bytes);
            }
            if (placed.placed_in_tier != TierLevel::UNKNOWN &&
                placed.placed_in_tier != to_tier) {
                return make_result(false, "Destination reported placement in the wrong tier",
                                   size_bytes);
            }

            if (remove_source && !source->invalidate(key)) {
                return make_result(
                    false,
                    "Destination write succeeded but source invalidation failed; both copies may remain",
                    size_bytes);
            }
            return make_result(true, "", size_bytes);
        } catch (const std::exception& error) {
            return make_result(false, error.what(), 0);
        } catch (...) {
            return make_result(false, "Tier adapter threw an unknown exception", 0);
        }
    }

    /**
     * @brief Worker Main.
     * @details Calls: generateCorrelationId(), trace_guard(), THEMIS_DEBUG(), lock(), wait(), empty(), front(), pop().
     */
    void workerMain() {
        // Set up trace context for this worker thread
        auto worker_id = generateCorrelationId("worker");
        TraceContext ctx{worker_id};
        TraceContextManager::ScopedContext trace_guard(ctx);
        
        THEMIS_DEBUG("Worker thread started: correlation_id={}", worker_id);

        while (true) {
            std::unique_lock<std::mutex> lock(mutex_);

            // Wait for work or shutdown signal
            condition_.wait(lock,
                           [this] { return !pending_tasks_.empty() || !running_; });

            if (!running_ && pending_tasks_.empty()) {
              break;
            }

            if (pending_tasks_.empty()) {
              continue;
            }

            DemotionEvent task = pending_tasks_.front();
            pending_tasks_.pop();
            metrics_.counters.pending_queue_depth = pending_tasks_.size();

            lock.unlock();

            // Process task (trace context automatically propagated)
            processPromotionTask(task);

        }
        
        THEMIS_DEBUG("Worker thread exiting: correlation_id={}", worker_id);
    }

    /**
     * @brief Process Promotion Task.
     * @param[in] task Input parameter.
     * @details Calls: std::chrono::system_clock::now(), generateCorrelationId(), lock(), set_value(), emplace_back(), THEMIS_DEBUG(), tierLevelName(), count().
     */
    void processPromotionTask(const DemotionEvent& task) {
        const auto processing_started = std::chrono::steady_clock::now();
        std::string correlation_id = generateCorrelationId("promo-task");
        auto result = transferKey(task.key, task.from_tier, task.to_tier,
                                  task.size_bytes, task.remove_source,
                                  correlation_id);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (task.remove_source) {
                if (result.success) {
                    metrics_.counters.demotions_succeeded++;
                } else {
                    metrics_.counters.demotions_failed++;
                }
            } else if (result.success) {
                metrics_.counters.promotions_succeeded++;
            } else {
                metrics_.counters.promotions_failed++;
            }
            const auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - processing_started).count();
            const auto sample = static_cast<uint64_t>(std::max<int64_t>(latency_us, 0));
            metrics_.recordEventProcessingLatency(sample);
            if (task.remove_source) {
                metrics_.recordTierDemotionLatency(sample);
            } else {
                metrics_.recordTierPromotionLatency(sample);
            }
            recordTransitionLocked(AccessTransitionEvent{
                .key = task.key,
                .from_tier = task.from_tier,
                .to_tier = task.to_tier,
                .reason = task.reason,
                .timestamp = result.completed_at,
                .latency_ms = result.total_latency_ms,
                .correlation_id = correlation_id,
                .success = result.success,
                .error_message = result.error_message,
            });
        }

        if (task.promise) {
            task.promise->set_value(result);
        }

        THEMIS_DEBUG(
            "Promotion task processed: key={}, from_tier={}, to_tier={}",
            task.key, tierLevelName(task.from_tier), tierLevelName(task.to_tier));
        
        // Emit structured tier transition log
        TierTransitionLog transition_log{
            .key = task.key,
            .from_tier = task.from_tier,
            .to_tier = task.to_tier,
            .reason = task.reason,
            .latency_ms = static_cast<uint64_t>(result.total_latency_ms.count()),
            .correlation_id = correlation_id,
            .thread_id = std::this_thread::get_id(),
            .timestamp = result.completed_at,
            .status = result.success ? "SUCCESS" : "FAILED",
        };
        accessModelLogger().logTierTransition(transition_log);
    }

    /**
     * @brief Generate Correlation Id.
     * @param[in] prefix Input parameter.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    std::string generateCorrelationId(const std::string& prefix) {
        static std::atomic<uint64_t> counter{0};
        return prefix + "-" + std::to_string(counter++);
    }

    /**
     * @brief Generate Plan Id.
     * @return Return value.
     * @details Calls: std::to_string().
     */
    std::string generatePlanId() {
        static std::atomic<uint64_t> counter{0};
        return "plan-" + std::to_string(counter++);
    }

    size_t thread_pool_size_;
    size_t max_queue_depth_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_;
    bool initialized_;

    std::mutex mutex_;
    std::condition_variable condition_;

    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers_;
    AgeBasedPolicy policy_;
    bool policy_set_;

    std::queue<DemotionEvent> pending_tasks_;
    std::unordered_map<std::string, DemotionPlan> pending_plans_;

    AccessModelMetrics metrics_;
    std::unordered_map<std::string, AccessMetrics> key_metrics_;
    std::map<TierLevel, AccessMetrics> tier_metrics_;
    std::vector<AccessTransitionEvent> recent_transitions_;
};

// ============================================================================
// § 2  Factory Function
// ============================================================================

/**
 * @brief Create Access Coordinator.
 * @param[in] thread_pool_size Input parameter.
 * @return Return value.
 * @details Implements createAccessCoordinator without additional internal calls.
 */
std::shared_ptr<AccessCoordinator> createAccessCoordinator(
    size_t thread_pool_size) {
    return std::make_shared<AccessCoordinatorImpl>(thread_pool_size);
}

std::shared_ptr<AccessCoordinator> createAccessCoordinator(
    size_t thread_pool_size, size_t max_queue_depth) {
    return std::make_shared<AccessCoordinatorImpl>(thread_pool_size,
                                                   max_queue_depth);
}

}  // namespace access_model
}  // namespace themis
