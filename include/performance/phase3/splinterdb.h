/*
 * ThemisDB | File: splinterdb.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SplinterDB: Closing the Bandwidth Gap for NVMe Key-Value Stores
// Paper: "SplinterDB: Closing the Bandwidth Gap for NVMe Key-Value Stores" (OSDI'20)
// Authors: Alex Conway et al., Carnegie Mellon University
//
// Key idea: Concurrent compaction on NVMe SSDs without write stalls
// Expected gain: -70% P99 latency
// Reference: https://www.usenix.org/system/files/osdi20-conway.pdf

#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <functional>

namespace themis {
namespace performance {
namespace phase3 {

/// Concurrent compaction manager
class ConcurrentCompactor {
public:
    explicit ConcurrentCompactor(size_t num_threads = 4);
    ~ConcurrentCompactor();
    
    // Start background compaction
    void start();
    
    // Stop background compaction
    void stop();
    
    // Schedule compaction for a level
    void schedule_compaction(int level, std::function<void()> compaction_fn);
    
    // Get compaction statistics
    struct Stats {
        size_t compactions_completed;
        size_t compactions_in_progress;
        double avg_compaction_time_ms;
    };
    Stats get_stats() const;

private:
    size_t num_threads_;
    std::vector<std::thread> worker_threads_;
    std::atomic<bool> running_{false};
    
    // Worker function
    void worker_loop();
};

} // namespace phase3
} // namespace performance
} // namespace themis
