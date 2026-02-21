/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            splinterdb.h                                       ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
