/**
 * @file cache_eviction_policy.cpp
 * @brief Eviction policy implementations with move semantics
 * @version 0.1.0
 * @note Gap Fix: CWE-672, CWE-457
 */

#include "cache/cache_eviction_policy.h"
#include <algorithm>
#include <utility>
#include <stdexcept>

namespace themis {
namespace cache {

// =============================================================================
// LRUEvictionPolicy Implementation
// =============================================================================

void LRUEvictionPolicy::record_hit(const std::string& key) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record hit on moved-from policy");
    }
    // Update access tracking
}

void LRUEvictionPolicy::record_miss(const std::string& key) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record miss on moved-from policy");
    }
}

void LRUEvictionPolicy::record_insert(const std::string& key, size_t size) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record insert on moved-from policy");
    }
}

void LRUEvictionPolicy::record_delete(const std::string& key) {
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
        return {false, "No candidates available"};
    }

    // Find least recently used (minimum timestamp)
    auto lru = std::min_element(candidates.begin(), candidates.end(),
                                [](const CacheKeyDescriptor& a, const CacheKeyDescriptor& b) {
                                    return a.last_access_ns < b.last_access_ns;
                                });

    if (lru != candidates.end()) {
        return {true, "LRU victim: " + lru->key};
    }

    return {false, "Failed to find victim"};
}

std::unique_ptr<CacheEvictionPolicy> LRUEvictionPolicy::clone() const {
    return std::make_unique<LRUEvictionPolicy>();
}

// =============================================================================
// LFUEvictionPolicy Implementation
// =============================================================================

LFUEvictionPolicy::LFUEvictionPolicy(double aging_factor)
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
    if (is_moved_from_) {
        throw std::logic_error("Cannot record hit on moved-from policy");
    }
}

void LFUEvictionPolicy::record_miss(const std::string& key) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record miss on moved-from policy");
    }
}

void LFUEvictionPolicy::record_insert(const std::string& key, size_t size) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record insert on moved-from policy");
    }
}

void LFUEvictionPolicy::record_delete(const std::string& key) {
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
        return {false, "No candidates available"};
    }

    // Find least frequently used (minimum access_count)
    auto lfu = std::min_element(candidates.begin(), candidates.end(),
                                [](const CacheKeyDescriptor& a, const CacheKeyDescriptor& b) {
                                    return a.access_count < b.access_count;
                                });

    if (lfu != candidates.end()) {
        return {true, "LFU victim: " + lfu->key};
    }

    return {false, "Failed to find victim"};
}

std::unique_ptr<CacheEvictionPolicy> LFUEvictionPolicy::clone() const {
    return std::make_unique<LFUEvictionPolicy>(aging_factor_);
}

// =============================================================================
// FIFOEvictionPolicy Implementation
// =============================================================================

void FIFOEvictionPolicy::record_hit(const std::string& key) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record hit on moved-from policy");
    }
}

void FIFOEvictionPolicy::record_miss(const std::string& key) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record miss on moved-from policy");
    }
}

void FIFOEvictionPolicy::record_insert(const std::string& key, size_t size) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record insert on moved-from policy");
    }
}

void FIFOEvictionPolicy::record_delete(const std::string& key) {
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
        return {false, "No candidates available"};
    }

    // Find oldest by creation time
    auto fifo = std::min_element(candidates.begin(), candidates.end(),
                                 [](const CacheKeyDescriptor& a, const CacheKeyDescriptor& b) {
                                     return a.creation_time_ns < b.creation_time_ns;
                                 });

    if (fifo != candidates.end()) {
        return {true, "FIFO victim: " + fifo->key};
    }

    return {false, "Failed to find victim"};
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
    if (is_moved_from_) {
        throw std::logic_error("Cannot record hit on moved-from policy");
    }
}

void ARCEvictionPolicy::record_miss(const std::string& key) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record miss on moved-from policy");
    }
}

void ARCEvictionPolicy::record_insert(const std::string& key, size_t size) {
    if (is_moved_from_) {
        throw std::logic_error("Cannot record insert on moved-from policy");
    }
}

void ARCEvictionPolicy::record_delete(const std::string& key) {
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
        return {false, "No candidates available"};
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
        return {true, "ARC victim: " + victim->key};
    }

    return {false, "Failed to find victim"};
}

std::unique_ptr<CacheEvictionPolicy> ARCEvictionPolicy::clone() const {
    return std::make_unique<ARCEvictionPolicy>();
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
    } else {
        throw std::invalid_argument("Unknown eviction policy: " + policy_name);
    }
}

} // namespace cache
} // namespace themis
