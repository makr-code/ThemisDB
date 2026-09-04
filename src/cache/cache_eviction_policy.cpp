/**
 * @file cache_eviction_policy.cpp
 * @brief Eviction policy implementations with move semantics
 * @version 0.1.0
 * @note Gap Fix: CWE-672, CWE-457
 */

#include "cache/cache_eviction_policy.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <limits>
#include <utility>
#include <stdexcept>

namespace themis {
namespace cache {

// =============================================================================
// LRUEvictionPolicy Implementation
// =============================================================================

LRUEvictionPolicy::LRUEvictionPolicy(LRUEvictionPolicy&& other) noexcept
    : is_moved_from_(false) {
    other.is_moved_from_ = true;
}

LRUEvictionPolicy& LRUEvictionPolicy::operator=(LRUEvictionPolicy&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    is_moved_from_ = false;
    other.is_moved_from_ = true;
    return *this;
}

void LRUEvictionPolicy::record_hit(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record hit on moved-from policy");
    }
    // Update access tracking
}

void LRUEvictionPolicy::record_miss(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record miss on moved-from policy");
    }
}

void LRUEvictionPolicy::record_insert(const std::string& key, size_t size) {
    (void)key;
    (void)size;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record insert on moved-from policy");
    }
}

void LRUEvictionPolicy::record_delete(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record delete on moved-from policy");
    }
}

CacheEvictionPolicy::EvictionDecision LRUEvictionPolicy::choose_victim(
    const std::vector<CacheKeyDescriptor>& candidates) {
    
    if (is_moved_from_) {
        throw std::logic_error("Cannot choose victim on moved-from policy");
    }

    if (candidates.empty()) {
        return {false, "", "No candidates available"};
    }

    // Find least recently used (minimum timestamp)
    auto lru = std::min_element(candidates.begin(), candidates.end(),
                                [](const CacheKeyDescriptor& a, const CacheKeyDescriptor& b) {
                                    return a.last_access_ns < b.last_access_ns;
                                });

    if (lru != candidates.end()) {
        return {true, lru->key, "LRU victim: " + lru->key};
    }

    return {false, "", "Failed to find victim"};
}

std::unique_ptr<CacheEvictionPolicy> LRUEvictionPolicy::clone() const {
    return std::make_unique<LRUEvictionPolicy>();
}

// =============================================================================
// LFUEvictionPolicy Implementation
// =============================================================================

LFUEvictionPolicy::LFUEvictionPolicy([[maybe_unused]] double aging_factor)
    : aging_factor_(aging_factor), is_moved_from_(false) {
    
    if (aging_factor < 0.0 || aging_factor > 1.0) {
        throw std::invalid_argument("aging_factor must be in [0.0, 1.0]");
    }
}

LFUEvictionPolicy::LFUEvictionPolicy(LFUEvictionPolicy&& other) noexcept
    : aging_factor_(other.aging_factor_),
      is_moved_from_(false) {
    
    other.is_moved_from_ = true;
}

LFUEvictionPolicy& LFUEvictionPolicy::operator=(LFUEvictionPolicy&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    aging_factor_ = other.aging_factor_;
    is_moved_from_ = false;

    other.is_moved_from_ = true;

    return *this;
}

void LFUEvictionPolicy::record_hit(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record hit on moved-from policy");
    }
}

void LFUEvictionPolicy::record_miss(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record miss on moved-from policy");
    }
}

void LFUEvictionPolicy::record_insert(const std::string& key, size_t size) {
    (void)key;
    (void)size;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record insert on moved-from policy");
    }
}

void LFUEvictionPolicy::record_delete(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record delete on moved-from policy");
    }
}

CacheEvictionPolicy::EvictionDecision LFUEvictionPolicy::choose_victim(
    const std::vector<CacheKeyDescriptor>& candidates) {
    
    if (is_moved_from_) {
        throw std::logic_error("Cannot choose victim on moved-from policy");
    }

    if (candidates.empty()) {
        return {false, "", "No candidates available"};
    }

    // Find least frequently used (minimum access_count)
    auto lfu = std::min_element(candidates.begin(), candidates.end(),
                                [](const CacheKeyDescriptor& a, const CacheKeyDescriptor& b) {
                                    return a.access_count < b.access_count;
                                });

    if (lfu != candidates.end()) {
        return {true, lfu->key, "LFU victim: " + lfu->key};
    }

    return {false, "", "Failed to find victim"};
}

std::unique_ptr<CacheEvictionPolicy> LFUEvictionPolicy::clone() const {
    return std::make_unique<LFUEvictionPolicy>(aging_factor_);
}

// =============================================================================
// FIFOEvictionPolicy Implementation
// =============================================================================

FIFOEvictionPolicy::FIFOEvictionPolicy(FIFOEvictionPolicy&& other) noexcept
    : is_moved_from_(false) {
    other.is_moved_from_ = true;
}

FIFOEvictionPolicy& FIFOEvictionPolicy::operator=(FIFOEvictionPolicy&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    is_moved_from_ = false;
    other.is_moved_from_ = true;
    return *this;
}

void FIFOEvictionPolicy::record_hit(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record hit on moved-from policy");
    }
}

void FIFOEvictionPolicy::record_miss(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record miss on moved-from policy");
    }
}

void FIFOEvictionPolicy::record_insert(const std::string& key, size_t size) {
    (void)key;
    (void)size;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record insert on moved-from policy");
    }
}

void FIFOEvictionPolicy::record_delete(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record delete on moved-from policy");
    }
}

CacheEvictionPolicy::EvictionDecision FIFOEvictionPolicy::choose_victim(
    const std::vector<CacheKeyDescriptor>& candidates) {
    
    if (is_moved_from_) {
        throw std::logic_error("Cannot choose victim on moved-from policy");
    }

    if (candidates.empty()) {
        return {false, "", "No candidates available"};
    }

    // Find oldest by creation time
    auto fifo = std::min_element(candidates.begin(), candidates.end(),
                                 [](const CacheKeyDescriptor& a, const CacheKeyDescriptor& b) {
                                     return a.creation_time_ns < b.creation_time_ns;
                                 });

    if (fifo != candidates.end()) {
        return {true, fifo->key, "FIFO victim: " + fifo->key};
    }

    return {false, "", "Failed to find victim"};
}

std::unique_ptr<CacheEvictionPolicy> FIFOEvictionPolicy::clone() const {
    return std::make_unique<FIFOEvictionPolicy>();
}

// =============================================================================
// ARCEvictionPolicy Implementation
// =============================================================================

ARCEvictionPolicy::ARCEvictionPolicy(ARCEvictionPolicy&& other) noexcept
    : arc_p(other.arc_p),
      is_moved_from_(false) {
    
    other.is_moved_from_ = true;
}

ARCEvictionPolicy& ARCEvictionPolicy::operator=(ARCEvictionPolicy&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    arc_p = other.arc_p;
    is_moved_from_ = false;

    other.is_moved_from_ = true;

    return *this;
}

void ARCEvictionPolicy::record_hit(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record hit on moved-from policy");
    }
}

void ARCEvictionPolicy::record_miss(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record miss on moved-from policy");
    }
}

void ARCEvictionPolicy::record_insert(const std::string& key, size_t size) {
    (void)key;
    (void)size;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record insert on moved-from policy");
    }
}

void ARCEvictionPolicy::record_delete(const std::string& key) {
    (void)key;
    if (is_moved_from_) {
        throw std::logic_error("Cannot record delete on moved-from policy");
    }
}

CacheEvictionPolicy::EvictionDecision ARCEvictionPolicy::choose_victim(
    const std::vector<CacheKeyDescriptor>& candidates) {
    
    if (is_moved_from_) {
        throw std::logic_error("Cannot choose victim on moved-from policy");
    }

    if (candidates.empty()) {
        return {false, "", "No candidates available"};
    }

    // Simplified ARC: mix of LRU and LFU heuristics
    auto victim = std::min_element(candidates.begin(), candidates.end(),
                                   [](const CacheKeyDescriptor& a, const CacheKeyDescriptor& b) {
                                       // Weight: recency (50%) + frequency (50%)
                                       double score_a = (a.last_access_ns * 0.5) + (a.access_count * 0.5);
                                       double score_b = (b.last_access_ns * 0.5) + (b.access_count * 0.5);
                                       return score_a < score_b;
                                   });

    if (victim != candidates.end()) {
        return {true, victim->key, "ARC victim: " + victim->key};
    }

    return {false, "", "Failed to find victim"};
}

std::unique_ptr<CacheEvictionPolicy> ARCEvictionPolicy::clone() const {
    return std::make_unique<ARCEvictionPolicy>();
}

// =============================================================================
// WeightedTieredLRUEvictionPolicy Implementation
// =============================================================================

WeightedTieredLRUEvictionPolicy::WeightedTieredLRUEvictionPolicy()
    : WeightedTieredLRUEvictionPolicy(Config{}) {}

WeightedTieredLRUEvictionPolicy::WeightedTieredLRUEvictionPolicy(Config config)
    : config_(std::move(config)),
      trigger_threshold_percent_(clamp_percent(config_.trigger_threshold_percent, 55, 90)),
      safe_threshold_percent_(clamp_percent(config_.safe_threshold_percent, 35, 80)) {
    // Phase 3: Validate L2 promotion threshold
    if (config_.l2_promotion_threshold == 0) {
        config_.l2_promotion_threshold = 1;
    }
    // Phase 3: Ensure L1 >= L2
    if (config_.l1_promotion_threshold < config_.l2_promotion_threshold) {
        config_.l1_promotion_threshold = config_.l2_promotion_threshold;
    }
    if (config_.frequency_weight < 0.0) {
        config_.frequency_weight = 0.0;
    }
    if (config_.recency_weight < 0.0) {
        config_.recency_weight = 0.0;
    }
    const double total_weight = config_.frequency_weight + config_.recency_weight;
    if (total_weight <= 0.0) {
        config_.frequency_weight = 0.3;
        config_.recency_weight = 0.7;
    } else {
        config_.frequency_weight /= total_weight;
        config_.recency_weight /= total_weight;
    }
    config_.frequency_decay_factor = std::clamp(config_.frequency_decay_factor, 0.0, 1.0);
    config_.severe_threshold_percent = clamp_percent(config_.severe_threshold_percent, 70, 95);
    if (trigger_threshold_percent_ >= config_.severe_threshold_percent) {
        trigger_threshold_percent_ = config_.severe_threshold_percent - 5;
    }
    if (safe_threshold_percent_ >= trigger_threshold_percent_) {
        safe_threshold_percent_ = trigger_threshold_percent_ > 10
            ? trigger_threshold_percent_ - 10
            : trigger_threshold_percent_;
    }
}

WeightedTieredLRUEvictionPolicy::WeightedTieredLRUEvictionPolicy(
    WeightedTieredLRUEvictionPolicy&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.mutex_);
    config_ = other.config_;
    states_ = std::move(other.states_);
    trigger_threshold_percent_ = other.trigger_threshold_percent_;
    safe_threshold_percent_ = other.safe_threshold_percent_;
    last_threshold_adjustment_ns_ = other.last_threshold_adjustment_ns_;
    is_moved_from_ = false;
    other.is_moved_from_ = true;
}

WeightedTieredLRUEvictionPolicy& WeightedTieredLRUEvictionPolicy::operator=(
    WeightedTieredLRUEvictionPolicy&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    std::scoped_lock lock(mutex_, other.mutex_);
    config_ = other.config_;
    states_ = std::move(other.states_);
    trigger_threshold_percent_ = other.trigger_threshold_percent_;
    safe_threshold_percent_ = other.safe_threshold_percent_;
    last_threshold_adjustment_ns_ = other.last_threshold_adjustment_ns_;
    is_moved_from_ = false;
    other.is_moved_from_ = true;
    return *this;
}

void WeightedTieredLRUEvictionPolicy::record_hit(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();
    auto& state = states_[key];
    const auto now = steady_now_ns();
    if (state.creation_time_ns == 0) {
        state.creation_time_ns = now;
    }
    state.last_access_ns = now;
    state.access_count += 1;
    state.decayed_frequency =
        (state.decayed_frequency * config_.frequency_decay_factor) + 1.0;
}

void WeightedTieredLRUEvictionPolicy::record_miss(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();
    auto it = states_.find(key);
    if (it != states_.end()) {
        it->second.decayed_frequency *= config_.frequency_decay_factor;
    }
}

void WeightedTieredLRUEvictionPolicy::record_insert(const std::string& key, size_t /*size*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();
    const auto now = steady_now_ns();
    auto& state = states_[key];
    state.creation_time_ns = now;
    state.last_access_ns = now;
    state.access_count = 0;
    state.decayed_frequency = 0.0;
}

void WeightedTieredLRUEvictionPolicy::record_delete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();
    states_.erase(key);
}

CacheEvictionPolicy::EvictionDecision
WeightedTieredLRUEvictionPolicy::choose_victim(
    const std::vector<CacheKeyDescriptor>& candidates) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();

    if (candidates.empty()) {
        return {false, "", "No candidates available"};
    }

    const auto now = steady_now_ns();
    const auto victim = std::min_element(
        candidates.begin(),
        candidates.end(),
        [this, now](const CacheKeyDescriptor& lhs, const CacheKeyDescriptor& rhs) {
            const auto lhs_state = states_.find(lhs.key);
            const auto rhs_state = states_.find(rhs.key);

            const size_t lhs_access_count =
                lhs_state != states_.end() ? lhs_state->second.access_count : lhs.access_count;
            const size_t rhs_access_count =
                rhs_state != states_.end() ? rhs_state->second.access_count : rhs.access_count;

            const auto lhs_tier = classify_locked(lhs_access_count);
            const auto rhs_tier = classify_locked(rhs_access_count);
            if (lhs_tier != rhs_tier) {
                return static_cast<int>(lhs_tier) < static_cast<int>(rhs_tier);
            }

            const EntryState lhs_entry{
                lhs_access_count,
                lhs_state != states_.end() ? lhs_state->second.last_access_ns : lhs.last_access_ns,
                lhs_state != states_.end() ? lhs_state->second.creation_time_ns : lhs.creation_time_ns,
                lhs_state != states_.end() ? lhs_state->second.decayed_frequency : static_cast<double>(lhs.access_count)
            };
            const EntryState rhs_entry{
                rhs_access_count,
                rhs_state != states_.end() ? rhs_state->second.last_access_ns : rhs.last_access_ns,
                rhs_state != states_.end() ? rhs_state->second.creation_time_ns : rhs.creation_time_ns,
                rhs_state != states_.end() ? rhs_state->second.decayed_frequency : static_cast<double>(rhs.access_count)
            };

            const double lhs_score = score_locked(lhs_entry, now);
            const double rhs_score = score_locked(rhs_entry, now);
            if (lhs_score != rhs_score) {
                return lhs_score < rhs_score;
            }

            if (lhs_entry.last_access_ns != rhs_entry.last_access_ns) {
                return lhs_entry.last_access_ns < rhs_entry.last_access_ns;
            }

            return lhs_entry.creation_time_ns < rhs_entry.creation_time_ns;
        });

    if (victim == candidates.end()) {
        return {false, "", "Failed to find victim"};
    }

    const auto tier = classify_locked(
        states_.count(victim->key) ? states_.at(victim->key).access_count : victim->access_count);
    std::string tier_name = "L3";
    if (tier == Tier::L2) {
        tier_name = "L2";
    } else if (tier == Tier::L1) {
        tier_name = "L1";
    }

    return {true, victim->key, "Tiered LRU victim (" + tier_name + "): " + victim->key};
}

std::unique_ptr<CacheEvictionPolicy> WeightedTieredLRUEvictionPolicy::clone() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto policy = std::make_unique<WeightedTieredLRUEvictionPolicy>(config_);
    policy->states_ = states_;
    policy->trigger_threshold_percent_ = trigger_threshold_percent_;
    policy->safe_threshold_percent_ = safe_threshold_percent_;
    policy->last_threshold_adjustment_ns_ = last_threshold_adjustment_ns_;
    return policy;
}

WeightedTieredLRUEvictionPolicy::Tier
WeightedTieredLRUEvictionPolicy::tier_for_key(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();
    const auto it = states_.find(key);
    if (it == states_.end()) {
        return Tier::L3;
    }
    return classify_locked(it->second.access_count);
}

double WeightedTieredLRUEvictionPolicy::score_for_key(const std::string& key,
                                                      int64_t now_ns) const {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();
    const auto it = states_.find(key);
    if (it == states_.end()) {
        return 0.0;
    }
    return score_locked(it->second, now_ns == 0 ? steady_now_ns() : now_ns);
}

double WeightedTieredLRUEvictionPolicy::score_for_descriptor(
    const CacheKeyDescriptor& descriptor, int64_t now_ns) const {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();
    const auto it = states_.find(descriptor.key);
    const EntryState state{
        it != states_.end() ? it->second.access_count : descriptor.access_count,
        it != states_.end() ? it->second.last_access_ns : descriptor.last_access_ns,
        it != states_.end() ? it->second.creation_time_ns : descriptor.creation_time_ns,
        it != states_.end() ? it->second.decayed_frequency : static_cast<double>(descriptor.access_count)
    };
    return score_locked(state, now_ns == 0 ? steady_now_ns() : now_ns);
}

void WeightedTieredLRUEvictionPolicy::observe_capacity(size_t current_capacity_percent,
                                                       int64_t now_ns) {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();

    if (!config_.adaptive_thresholds) {
        return;
    }

    const auto now = now_ns == 0 ? steady_now_ns() : now_ns;
    if (last_threshold_adjustment_ns_ != 0 &&
        now - last_threshold_adjustment_ns_ < config_.threshold_adjustment_interval_ns) {
        return;
    }

    const auto high_pressure = current_capacity_percent >= 90;
    const auto low_pressure = current_capacity_percent <= 75;

    if (!high_pressure && !low_pressure) {
        return;
    }

    if (high_pressure) {
        trigger_threshold_percent_ = clamp_percent(trigger_threshold_percent_ > 5
            ? trigger_threshold_percent_ - 5
            : trigger_threshold_percent_, 55, config_.severe_threshold_percent - 1);
        safe_threshold_percent_ = clamp_percent(
            std::min(safe_threshold_percent_, trigger_threshold_percent_ - 10), 35, 80);
    } else if (low_pressure) {
        trigger_threshold_percent_ = clamp_percent(
            std::min<size_t>(trigger_threshold_percent_ + 5,
                             config_.severe_threshold_percent - 1),
            55, config_.severe_threshold_percent - 1);
        safe_threshold_percent_ = clamp_percent(
            std::min<size_t>(trigger_threshold_percent_ - 10, safe_threshold_percent_ + 5),
            35, 80);
    }

    last_threshold_adjustment_ns_ = now;
}

size_t WeightedTieredLRUEvictionPolicy::recommended_batch_size(
    size_t current_capacity_percent, size_t candidate_count) const {
    if (current_capacity_percent < trigger_threshold_percent_ || candidate_count == 0) {
        return 0;
    }
    if (current_capacity_percent >= config_.severe_threshold_percent) {
        return std::min<size_t>(std::max<size_t>(10, candidate_count / 4), candidate_count);
    }
    return 1;
}

std::array<size_t, 3> WeightedTieredLRUEvictionPolicy::tier_distribution() const {
    std::lock_guard<std::mutex> lock(mutex_);
    ensure_operational();
    std::array<size_t, 3> distribution{0, 0, 0};
    for (const auto& [_, state] : states_) {
        distribution[static_cast<size_t>(classify_locked(state.access_count))] += 1;
    }
    return distribution;
}

int64_t WeightedTieredLRUEvictionPolicy::steady_now_ns() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

size_t WeightedTieredLRUEvictionPolicy::clamp_percent(size_t value,
                                                      size_t min_value,
                                                      size_t max_value) {
    return std::max(min_value, std::min(value, max_value));
}

void WeightedTieredLRUEvictionPolicy::ensure_operational() const {
    if (is_moved_from_) {
        throw std::logic_error("Cannot use moved-from policy");
    }
}

WeightedTieredLRUEvictionPolicy::Tier
WeightedTieredLRUEvictionPolicy::classify_locked([[maybe_unused]] size_t access_count) const {
    if (access_count >= config_.l1_promotion_threshold) {
        return Tier::L1;
    }
    if (access_count >= config_.l2_promotion_threshold) {
        return Tier::L2;
    }
    return Tier::L3;
}

double WeightedTieredLRUEvictionPolicy::score_locked(const EntryState& state,
                                                     int64_t now_ns) const {
    const auto age_ns = std::max<int64_t>(1, now_ns - state.last_access_ns);
    const auto age_seconds = static_cast<double>(age_ns) / 1000000000.0;
    const auto recency_score = 1.0 / (1.0 + age_seconds);

    const double frequency_basis =
        state.decayed_frequency > 0.0 ? state.decayed_frequency : static_cast<double>(state.access_count);
    const double max_frequency_basis =
        std::max<double>(1.0, static_cast<double>(config_.l1_promotion_threshold));
    const auto frequency_score =
        std::min(1.0, std::log1p(frequency_basis) / std::log1p(max_frequency_basis));

    return (config_.frequency_weight * frequency_score) +
           (config_.recency_weight * recency_score);
}

// =============================================================================
// EvictionPolicyFactory Implementation
// =============================================================================

std::unique_ptr<CacheEvictionPolicy> EvictionPolicyFactory::create(const std::string& policy_name) {
    if (policy_name == "LRU" || policy_name == "lru") {
        return std::make_unique<LRUEvictionPolicy>();
    } else if (policy_name == "LFU" || policy_name == "lfu") {
        return std::make_unique<LFUEvictionPolicy>();
    } else if (policy_name == "FIFO" || policy_name == "fifo") {
        return std::make_unique<FIFOEvictionPolicy>();
    } else if (policy_name == "ARC" || policy_name == "arc") {
        return std::make_unique<ARCEvictionPolicy>();
    } else if (policy_name == "TIERED_LRU" || policy_name == "tiered_lru" ||
               policy_name == "tiered-lru") {
        return std::make_unique<WeightedTieredLRUEvictionPolicy>();
    } else {
        throw std::invalid_argument("Unknown eviction policy: " + policy_name);
    }
}

} // namespace cache
} // namespace themis
