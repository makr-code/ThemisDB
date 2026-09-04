/**
 * @file hnsw_parameter_tuner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "index/hnsw_parameter_tuner.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

#ifdef __SSE__
#include <xmmintrin.h>
#endif

// Platform-specific cache-line size queries
#if defined(__linux__)
#   include <unistd.h>   // sysconf
#elif defined(__APPLE__)
#   include <sys/sysctl.h>
#elif defined(_WIN32)
#   include <windows.h>
#   include <sysinfoapi.h>
#endif

namespace themis {
namespace index {

HnswParameterTuner::HnswParameterTuner(const Config& config)
    : config_(config)
    , current_ef_search_(config.ef_search_default)
{
    recent_queries_.reserve(config.stats_window_size);
}

int HnswParameterTuner::getOptimalEfSearch(size_t k, size_t dataset_size) const {
    if (!config_.adaptive) {
        return config_.ef_search_default;
    }
    
    int ef = current_ef_search_.load();
    
    // Scale with k (efSearch should be >= k)
    ef = std::max(ef, static_cast<int>(k));
    
    // Scale with dataset size if enabled
    if (config_.scale_with_dataset) {
        ef = calculateEfSearch(k, dataset_size);
    }
    
    // Clamp to configured range
    ef = std::clamp(ef, config_.ef_search_min, config_.ef_search_max);
    
    return ef;
}

void HnswParameterTuner::recordQueryResult(size_t k, int ef_used, double latency_ms, double recall) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    QueryStats stats;
    stats.k = k;
    stats.ef_used = ef_used;
    stats.latency_ms = latency_ms;
    stats.recall = recall;
    
    recent_queries_.push_back(stats);
    
    // Keep only recent queries
    if (recent_queries_.size() > config_.stats_window_size) {
        recent_queries_.erase(recent_queries_.begin());
    }
    
    queries_processed_.fetch_add(1);
    total_latency_ += latency_ms;
    
    if (recall >= 0.0) {
        total_recall_ += recall;
        recall_count_++;
    }
    
    // Adapt if we have enough samples
    if (recent_queries_.size() >= 100) {
        adapt();
    }
}

void HnswParameterTuner::updateConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    current_ef_search_.store(config.ef_search_default);
}

HnswParameterTuner::Stats HnswParameterTuner::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.queries_processed = queries_processed_.load();
    stats.current_ef_search = current_ef_search_.load();
    stats.adaptations_count = adaptations_count_.load();
    
    size_t count = queries_processed_.load();
    if (count > 0) {
        stats.avg_latency_ms = total_latency_ / count;
        
        if (recall_count_ > 0) {
            stats.avg_recall = total_recall_ / recall_count_;
        }
    }
    
    return stats;
}

void HnswParameterTuner::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    recent_queries_.clear();
    queries_processed_.store(0);
    adaptations_count_.store(0);
    total_latency_ = 0.0;
    total_recall_ = 0.0;
    recall_count_ = 0;
}

int HnswParameterTuner::getRecommendedM(size_t dataset_size, WorkloadType workload) {
    // Based on HNSW paper recommendations and empirical results
    // Adjusted for workload-specific requirements per PERFORMANCE_TIPS.md
    
    int base_M = 16;
    
    // Base M selection based on dataset size
    if (dataset_size < 10000) {
        base_M = 8;
    } else if (dataset_size < 100000) {
        base_M = 16;
    } else if (dataset_size < 1000000) {
        base_M = 24;
    } else {
        base_M = 32;
    }
    
    // Workload-specific adjustments
    switch (workload) {
        case WorkloadType::OLTP:
            // OLTP: Prioritize write throughput and low latency
            // Reduce M for faster inserts, acceptable recall trade-off
            return std::max(8, base_M - 4);
            
        case WorkloadType::ANALYTICS:
            // Analytics: Maximize recall, tolerate higher memory/latency
            // Increase M for better graph connectivity
            return std::min(48, base_M + 8);
            
        case WorkloadType::RAG:
            // RAG: Balance recall and latency, medium-high M
            return std::min(40, base_M + 4);
            
        case WorkloadType::BATCH_INSERT:
            // Batch insert: Optimize for fast bulk loading
            return std::max(8, base_M - 6);
            
        case WorkloadType::MIXED:
        [[fallthrough]];\n        default:
            // Balanced configuration
            return base_M;
    }
}

int HnswParameterTuner::getRecommendedEfConstruction(size_t dataset_size, int M, WorkloadType workload) {
    // ef_construction should be roughly 10-20x M for good recall
    // Scale with dataset size and adjust for workload
    
    int base_ef = M * 12;
    
    // Dataset size scaling
    if (dataset_size < 10000) {
        // Small datasets: keep base value
    } else if (dataset_size < 100000) {
        base_ef = static_cast<int>(base_ef * 1.5);
    } else if (dataset_size < 1000000) {
        base_ef = static_cast<int>(base_ef * 2.0);
    } else {
        base_ef = static_cast<int>(base_ef * 3.0);
    }
    
    // Workload-specific adjustments
    switch (workload) {
        case WorkloadType::OLTP:
            // OLTP: Prioritize insert speed, reduce ef_construction
            return static_cast<int>(base_ef * 0.7);
            
        case WorkloadType::ANALYTICS:
            // Analytics: Maximize index quality for better query performance
            return static_cast<int>(base_ef * 1.3);
            
        case WorkloadType::RAG:
            // RAG: High-quality index for accurate retrieval
            return static_cast<int>(base_ef * 1.2);
            
        case WorkloadType::BATCH_INSERT:
            // Batch insert: Fast construction, can rebuild later if needed
            return static_cast<int>(base_ef * 0.6);
            
        case WorkloadType::MIXED:
        [[fallthrough]];\n        default:
            return base_ef;
    }
}

void HnswParameterTuner::adapt() {
    if (!config_.adaptive || recent_queries_.empty()) {
        return;
    }
     
    // Calculate average latency
    double avg_latency = 0.0;
    double avg_recall = 0.0;
    size_t recall_samples = 0;
     
    for (const auto& q : recent_queries_) {
        avg_latency += q.latency_ms;
        if (q.recall >= 0.0) {
            avg_recall += q.recall;
            recall_samples++;
        }
    }
     
    avg_latency /= recent_queries_.size();
    if (recall_samples > 0) {
        avg_recall /= recall_samples;
    }
    
    int current_ef = current_ef_search_.load();
    int new_ef = current_ef;
    
    // If latency is too high, decrease efSearch
    if (avg_latency > config_.target_latency.count()) {
        new_ef = std::max(config_.ef_search_min, 
                         static_cast<int>(current_ef * 0.9));
    }
    // If recall is too low (and we have recall data), increase efSearch
    else if (recall_samples > 0 && avg_recall < config_.target_recall) {
        new_ef = std::min(config_.ef_search_max, 
                         static_cast<int>(current_ef * 1.1));
    }
    // If both latency and recall are good, try to optimize (reduce efSearch slightly)
    else if (recall_samples > 0 && avg_recall > config_.target_recall + 0.02) {
        new_ef = std::max(config_.ef_search_min, 
                         static_cast<int>(current_ef * 0.95));
    }
    
    if (new_ef != current_ef) {
        current_ef_search_.store(new_ef);
        adaptations_count_.fetch_add(1);
    }
}

int HnswParameterTuner::calculateEfSearch(size_t k, size_t dataset_size) const {
    // Base efSearch calculation: scale with k and log(dataset_size)
    // Adjusted for workload-specific requirements
    
    double base_ef = k * 2.0; // Start with 2x k
    
    // Workload-specific multipliers
    switch (config_.workload) {
        case WorkloadType::OLTP:
            // OLTP: Minimize efSearch for lower latency
            base_ef = k * 1.5;
            break;
            
        case WorkloadType::ANALYTICS:
            // Analytics: Higher efSearch for better recall
            base_ef = k * 3.0;
            break;
            
        case WorkloadType::RAG:
            // RAG: Balance between speed and accuracy
            base_ef = k * 2.5;
            break;
            
        case WorkloadType::BATCH_INSERT:
            // Batch insert: Not applicable for search, use default
            base_ef = k * 2.0;
            break;
            
        case WorkloadType::MIXED:
        [[fallthrough]];\n        default:
            base_ef = k * 2.0;
            break;
    }
    
    // Scale with dataset size (logarithmically)
    if (dataset_size > 10000) {
        double scale_factor = 1.0 + std::log10(dataset_size / 10000.0);
        base_ef *= scale_factor;
    }
    
    // Clamp to configured range
    int ef = static_cast<int>(base_ef);
    return std::clamp(ef, config_.ef_search_min, config_.ef_search_max);
}

HnswParameterTuner::Config HnswParameterTuner::getWorkloadOptimizedConfig(
    size_t dataset_size,
    WorkloadType workload) {
    Config config;
    config.workload = workload;
    
    // Set M and ef_construction based on workload
    config.M = getRecommendedM(dataset_size, workload);
    config.ef_construction = getRecommendedEfConstruction(dataset_size, config.M, workload);
    
    // Workload-specific runtime parameters
    switch (workload) {
        case WorkloadType::OLTP:
            // OLTP: Low latency, acceptable recall
            config.ef_search_min = 16;
            config.ef_search_max = 128;
            config.ef_search_default = 32;
            config.target_recall = 0.90;  // Slightly lower for speed
            config.target_latency = std::chrono::milliseconds(5);  // Aggressive latency target
            config.adaptive = true;
            config.scale_with_dataset = true;
            break;
            
        case WorkloadType::ANALYTICS:
            // Analytics: High recall, tolerate higher latency
            config.ef_search_min = 64;
            config.ef_search_max = 512;
            config.ef_search_default = 128;
            config.target_recall = 0.98;  // High recall requirement
            config.target_latency = std::chrono::milliseconds(50);  // More relaxed latency
            config.adaptive = true;
            config.scale_with_dataset = true;
            break;
            
        case WorkloadType::RAG:
            // RAG: Balance speed and accuracy for LLM applications
            config.ef_search_min = 32;
            config.ef_search_max = 256;
            config.ef_search_default = 64;
            config.target_recall = 0.95;  // High but not maximum recall
            config.target_latency = std::chrono::milliseconds(15);
            config.adaptive = true;
            config.scale_with_dataset = true;
            break;
            
        case WorkloadType::BATCH_INSERT:
            // Batch insert: Optimize for throughput
            config.ef_search_min = 32;
            config.ef_search_max = 256;
            config.ef_search_default = 64;
            config.target_recall = 0.92;
            config.target_latency = std::chrono::milliseconds(10);
            config.adaptive = false;  // Don't adapt during bulk insert
            config.scale_with_dataset = false;
            break;
            
        case WorkloadType::MIXED:
        [[fallthrough]];\n        default:
            // Mixed: Balanced configuration
            config.ef_search_min = 32;
            config.ef_search_max = 512;
            config.ef_search_default = 64;
            config.target_recall = 0.95;
            config.target_latency = std::chrono::milliseconds(10);
            config.adaptive = true;
            config.scale_with_dataset = true;
            break;
    }
    
    // Common settings
    config.stats_window_size = 1000;
    
    return config;
}

HnswParameterTuner::ConstructionParams HnswParameterTuner::getAutoTunedConstructionParams(
    size_t dataset_size,
    WorkloadType workload_hint) const {

    ConstructionParams result;

    // Determine effective workload: use hint unless caller left it as MIXED,
    // in which case we infer from recently recorded query statistics.
    WorkloadType effective_workload = workload_hint;

    if (effective_workload == WorkloadType::MIXED) {
        // Infer from recorded query patterns.
        // Use average k as a simple heuristic:
        //   k <= 5         → likely OLTP (point-lookups, small neighborhoods)
        //   6 <= k <= 19   → likely RAG  (retrieval-augmented generation)
        //   k >= 20        → likely ANALYTICS (large result sets)
        std::lock_guard<std::mutex> lock(mutex_);
        if (!recent_queries_.empty()) {
            double avg_k = 0.0;
            for (const auto& q : recent_queries_) {
                avg_k += static_cast<double>(q.k);
            }
            avg_k /= static_cast<double>(recent_queries_.size());

            if (avg_k <= 5.0) {
                effective_workload = WorkloadType::OLTP;
            } else if (avg_k >= 20.0) {
                effective_workload = WorkloadType::ANALYTICS;
            } else {
                effective_workload = WorkloadType::RAG;
            }
        }
        // If no queries have been recorded yet, keep MIXED as the default.
    }

    result.detected_workload  = effective_workload;
    result.M                  = getRecommendedM(dataset_size, effective_workload);
    result.ef_construction    = getRecommendedEfConstruction(dataset_size, result.M, effective_workload);

    return result;
}

// WorkloadClassifier implementation

void WorkloadClassifier::recordInsert([[maybe_unused]] size_t batch_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    total_inserts_ += batch_size;
    insert_events_ += 1;
}

void WorkloadClassifier::recordQuery([[maybe_unused]] size_t k) {
    std::lock_guard<std::mutex> lock(mutex_);
    total_k_      += k;
    query_events_ += 1;
}

HnswParameterTuner::WorkloadType WorkloadClassifier::detectWorkload() const {
    std::lock_guard<std::mutex> lock(mutex_);

    const bool has_inserts = insert_events_ > 0;
    const bool has_queries = query_events_  > 0;

    if (!has_inserts && !has_queries) {
        return HnswParameterTuner::WorkloadType::MIXED;
    }

    double avg_batch = has_inserts
        ? static_cast<double>(total_inserts_) / static_cast<double>(insert_events_)
        : 0.0;
    double avg_k = has_queries
        ? static_cast<double>(total_k_) / static_cast<double>(query_events_)
        : 0.0;

    // Pure-insert or heavily insert-dominated traffic
    if (!has_queries) {
        return avg_batch >= 100.0
            ? HnswParameterTuner::WorkloadType::BATCH_INSERT
            : HnswParameterTuner::WorkloadType::OLTP;
    }

    double insert_query_ratio = has_inserts
        ? static_cast<double>(insert_events_) / static_cast<double>(query_events_)
        : 0.0;

    // Heavy batch insertions with very few queries
    if (insert_query_ratio >= 10.0 && avg_batch >= 100.0) {
        return HnswParameterTuner::WorkloadType::BATCH_INSERT;
    }

    // Insert-dominated with small batches → transactional (OLTP)
    if (insert_query_ratio >= 1.0) {
        return HnswParameterTuner::WorkloadType::OLTP;
    }

    // Query-dominated: differentiate by k
    if (avg_k >= 20.0) {
        return HnswParameterTuner::WorkloadType::ANALYTICS;
    }
    if (avg_k <= 5.0) {
        return HnswParameterTuner::WorkloadType::OLTP;
    }
    return HnswParameterTuner::WorkloadType::RAG;
}

void WorkloadClassifier::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    total_inserts_  = 0;
    insert_events_  = 0;
    total_k_        = 0;
    query_events_   = 0;
}

WorkloadClassifier::Stats WorkloadClassifier::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.total_inserts  = total_inserts_;
    s.total_queries  = query_events_;
    s.avg_batch_size = insert_events_ > 0
        ? static_cast<double>(total_inserts_) / static_cast<double>(insert_events_)
        : 0.0;
    s.avg_k = query_events_ > 0
        ? static_cast<double>(total_k_) / static_cast<double>(query_events_)
        : 0.0;
    return s;
}

// HnswMemoryOptimizer implementation

void HnswMemoryOptimizer::prefetchNodes([[maybe_unused]] const std::vector<size_t>& node_ids) {
#if !defined(__SSE__) && !defined(__GNUC__)
    (void)node_ids;
#endif
#ifdef __SSE__
    // Use SIMD prefetch instructions if available
    for (size_t id : node_ids) {
        // Prefetch to L1 cache
        _mm_prefetch(reinterpret_cast<const char*>(&id), _MM_HINT_T0);
    }
#else
    // Fallback: compiler builtin
    #ifdef __GNUC__
    for (size_t id : node_ids) {
        __builtin_prefetch(&id, 0, 3);
    }
    #endif
#endif
}

size_t HnswMemoryOptimizer::getCacheLineSize() {
#if defined(__linux__)
    // Use POSIX sysconf on Linux; returns -1 on unsupported systems.
    const long sz = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (sz > 0) {
        return static_cast<size_t>(sz);
    }
#elif defined(__APPLE__)
    // macOS exposes the cache line size via sysctl hw.cachelinesize.
    std::size_t line = 0;
    std::size_t len  = sizeof(line);
    if (sysctlbyname("hw.cachelinesize", &line, &len, nullptr, 0) == 0 && line > 0) {
        return line;
    }
#elif defined(_WIN32)
    // Windows: enumerate logical processor relationships to find the L1
    // data cache line size.
    DWORD bufSize = 0;
    GetLogicalProcessorInformation(nullptr, &bufSize);
    if (bufSize > 0) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buf(
            bufSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buf.data(), &bufSize)) {
            for (const auto& info : buf) {
                if (info.Relationship == RelationCache &&
                    info.Cache.Type   == CacheData &&
                    info.Cache.Level  == 1 &&
                    info.Cache.LineSize > 0) {
                    return static_cast<size_t>(info.Cache.LineSize);
                }
            }
        }
    }
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    // CPUID leaf 0x1: EBX[15:8] gives CLFLUSH line size * 8.
    // Supported on all x86/x64 CPUs that have CLFLUSH (CPUID.01h:EDX.CLFSH[bit 19]).
    uint32_t eax = 1, ebx = 0, ecx = 0, edx = 0; // eax/ecx used in GCC asm path only
#   if defined(_MSC_VER)
    int regs[4] = {};
    __cpuid(regs, 1);
    ebx = static_cast<uint32_t>(regs[1]);
    edx = static_cast<uint32_t>(regs[3]);
    (void)eax; (void)ecx;  // only used in GCC/Clang asm branch
#   else
    __asm__ volatile(
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(eax), "c"(ecx));
#   endif
    if (edx & (1u << 19)) {                     // CLFLUSH feature bit
        const size_t sz = ((ebx >> 8) & 0xFFu) * 8u;
        if (sz > 0) {
          return sz;
        }
    }
#endif

    // Conservative fallback valid for all modern x86/x64/ARM/RISC-V CPUs.
    return 64;
}

size_t HnswMemoryOptimizer::alignToCacheLine([[maybe_unused]] size_t size) {
    size_t cache_line = getCacheLineSize();
    return ((size + cache_line - 1) / cache_line) * cache_line;
}

bool HnswMemoryOptimizer::hasSIMDPrefetch() {
#ifdef __SSE__
    return true;
#else
    return false;
#endif
}

} // namespace index
} // namespace themis

