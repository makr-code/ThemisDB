/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            sequence_packer.h                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:11:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include <vector>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Sequence packer for reducing padding waste
 * 
 * Problem: Padding short sequences to max_length wastes memory and compute.
 * Solution: Pack sequences to reduce padding from ~50% to <10%.
 * 
 * Benefits:
 * - 2-3x reduction in memory usage for variable-length sequences
 * - Proportional reduction in compute time
 * - Enables larger effective batch sizes
 * 
 * Example:
 * ```
 * Original (padded to max_len=8):
 *   Seq 1: [1, 2, 3, PAD, PAD, PAD, PAD, PAD]  (3 tokens, 5 padding)
 *   Seq 2: [4, 5, 6, 7, PAD, PAD, PAD, PAD]    (4 tokens, 4 padding)
 *   Total: 8 real tokens + 10 padding = 18 tokens (56% waste)
 * 
 * Packed (no padding):
 *   [1, 2, 3, 4, 5, 6, 7]  (7 tokens, 0 padding, 0% waste)
 *   Offsets: [0, 3, 7]
 *   Lengths: [3, 4]
 * ```
 */
class SequencePacker {
public:
    /**
     * @brief Packed batch representation
     */
    struct PackedBatch {
        GPUTensor token_ids;        // [total_tokens] - packed without padding
        GPUTensor attention_mask;   // [total_tokens] - attention mask
        std::vector<size_t> sequence_lengths;  // Length per sequence
        std::vector<size_t> sequence_offsets;  // Start offset per sequence
        size_t num_sequences;
        size_t total_tokens;
        
        bool is_valid() const {
            return num_sequences > 0 && total_tokens > 0 && 
                   sequence_lengths.size() == num_sequences &&
                   sequence_offsets.size() == num_sequences;
        }
    };
    
    /**
     * @brief Construct sequence packer
     * @param device Target GPU device
     */
    explicit SequencePacker(const Device& device = Device::cuda());
    
    /**
     * @brief Pack variable-length sequences to minimize padding
     * 
     * Strategy: Group sequences by similar lengths and pack contiguously
     * 
     * @param sequences Vector of token sequences (variable length)
     * @return Packed batch with metadata
     */
    PackedBatch packSequences(const std::vector<std::vector<int>>& sequences);
    
    /**
     * @brief Unpack results back to separate sequences
     * @param packed_output Packed output tensor [total_tokens, hidden_dim]
     * @param batch_info Batch metadata from packSequences
     * @return Vector of unpacked tensors, one per sequence
     */
    std::vector<GPUTensor> unpackResults(
        const GPUTensor& packed_output,
        const PackedBatch& batch_info
    );
    
    /**
     * @brief Calculate memory savings from packing
     * @param sequences Input sequences
     * @param max_length Maximum sequence length (for padding calculation)
     * @return Memory savings percentage (0.0-1.0)
     */
    static float calculateMemorySavings(
        const std::vector<std::vector<int>>& sequences,
        size_t max_length
    );
    
private:
    Device device_;
    
    /**
     * @brief Sort sequences by length for better packing
     * Returns sorted sequences and original indices
     */
    std::pair<std::vector<std::vector<int>>, std::vector<size_t>> 
    sortByLength(const std::vector<std::vector<int>>& sequences) const;
};

} // namespace lora
} // namespace llm
} // namespace themis
