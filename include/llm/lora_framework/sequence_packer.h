/**
 * @file sequence_packer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include <vector>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {

class SequencePacker {
public:
    /**
     * @brief Sequence Packer.
     * @return Return value.
     */
    virtual ~SequencePacker() = default;
    struct PackedBatch {
        GPUTensor token_ids;        // [total_tokens] - packed without padding
        GPUTensor attention_mask;   // [total_tokens] - attention mask
        std::vector<size_t> sequence_lengths;  // Length per sequence
        std::vector<size_t> sequence_offsets;  // Start offset per sequence
        size_t num_sequences = 0;
        size_t total_tokens = 0;
        
        bool is_valid() const {
            return num_sequences > 0 && total_tokens > 0 && 
                   sequence_lengths.size() == num_sequences &&
                   sequence_offsets.size() == num_sequences;
        }
    };
    
    explicit SequencePacker(const Device& device = Device::cuda());
    
    /**
     * @brief Pack Sequences.
     * @param[in] sequences Input parameter.
     * @return Return value.
     */
    PackedBatch packSequences(const std::vector<std::vector<int>>& sequences);
    
    /**
     * @brief Unpack Results.
     * @param[in] packed_output Input parameter.
     * @param[in] batch_info Input parameter.
     * @return Return value.
     */
    std::vector<GPUTensor> unpackResults(
        const GPUTensor& packed_output,
        const PackedBatch& batch_info
    );
    
    /**
     * @brief Calculate Memory Savings.
     * @param[in] sequences Input parameter.
     * @param[in] max_length Input parameter.
     * @return Return value.
     */
    static float calculateMemorySavings(
        const std::vector<std::vector<int>>& sequences,
        size_t max_length
    );
    
private:
    Device device_;
    
    std::pair<std::vector<std::vector<int>>, std::vector<size_t>> 
    sortByLength(const std::vector<std::vector<int>>& sequences) const;
};

} // namespace lora
} // namespace llm
} // namespace themis

