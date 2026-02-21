/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            block_table.h                                      ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:57:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     92                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 4b4dd1ff7  2025-12-29  Add performance benchmark results and update benchmarking... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/paged_block_manager.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace themis {
namespace llm {

/**
 * BlockTable manages the mapping between logical sequence positions and physical blocks
 * for a single inference sequence. Supports Copy-on-Write (CoW) for prefix sharing.
 */
class BlockTable {
public:
    struct Config {
        size_t block_size = 16;  // Tokens per block
        bool enable_cow = true;   // Enable Copy-on-Write for prefix sharing
    };

    BlockTable(std::shared_ptr<PagedBlockManager> block_manager, 
               uint64_t sequence_id, 
               const Config& config);
    
    ~BlockTable();

    // Allocate blocks for this sequence
    std::vector<int> allocateBlocks(size_t num_blocks);
    
    // Release all blocks for this sequence
    void releaseBlocks();
    
    // Share prefix blocks from parent sequence (Copy-on-Write)
    void sharePrefix(uint64_t parent_sequence_id, size_t prefix_length);
    
    // Get block mapping for attention computation
    std::vector<int> getBlockMapping() const;
    
    // Get number of tokens stored
    size_t getNumTokens() const;
    
    // Get statistics
    struct Stats {
        size_t num_blocks;
        size_t num_shared_blocks;
        size_t num_cow_blocks;
        double sharing_ratio;
    };
    Stats getStats() const;

private:
    std::shared_ptr<PagedBlockManager> block_manager_;
    uint64_t sequence_id_;
    Config config_;
    
    std::vector<int> block_ids_;          // Physical block IDs
    std::vector<bool> is_shared_;         // Which blocks are shared via CoW
    std::unordered_map<int, int> ref_counts_;  // Reference counts for shared blocks
    
    mutable std::mutex mutex_;
};

} // namespace llm
} // namespace themis
