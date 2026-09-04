/**
 * @file sequence_packer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/sequence_packer.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>

namespace themis {
namespace llm {
namespace lora {

SequencePacker::SequencePacker(const Device& device)
    : device_(device) {
    spdlog::debug("SequencePacker initialized for device: {}", static_cast<int>(device.type));
}

SequencePacker::PackedBatch SequencePacker::packSequences(
    const std::vector<std::vector<int>>& sequences
) {
    PackedBatch batch;
    batch.num_sequences = sequences.size();
    
    if (sequences.empty()) {
        spdlog::warn("SequencePacker: Empty sequence list");
        return batch;
    }
    
    // Calculate total tokens
    size_t total_tokens = 0;
    for (const auto& seq : sequences) {
        batch.sequence_lengths.push_back(seq.size());
        batch.sequence_offsets.push_back(total_tokens);
        total_tokens += seq.size();
    }
    batch.total_tokens = total_tokens;
    
    spdlog::debug("Packing {} sequences with {} total tokens", 
                  batch.num_sequences, total_tokens);
    
    // Pack into single contiguous array
    std::vector<float> packed_data;
    packed_data.reserve(total_tokens);
    
    for (const auto& seq : sequences) {
        for (int token : seq) {
            packed_data.push_back(static_cast<float>(token));
        }
    }
    
    // Create GPU tensor with packed data
    batch.token_ids = GPUTensor({total_tokens}, device_);
    batch.token_ids.upload(packed_data);
    
    // Create attention mask (all 1s for real tokens)
    std::vector<float> mask_data(total_tokens, 1.0f);
    batch.attention_mask = GPUTensor({total_tokens}, device_);
    batch.attention_mask.upload(mask_data);
    
    // Calculate and log memory savings
    size_t max_len = 0;
    for (const auto& seq : sequences) {
        max_len = std::max(max_len, seq.size());
    }
    size_t padded_total = batch.num_sequences * max_len;
    float savings = 100.0f * (1.0f - static_cast<float>(total_tokens) / padded_total);
    
    spdlog::info("Sequence packing complete:");
    spdlog::info("  Packed tokens: {}", total_tokens);
    spdlog::info("  Padded would be: {}", padded_total);
    spdlog::info("  Memory savings: {:.1f}%", savings);
    
    return batch;
}

std::vector<GPUTensor> SequencePacker::unpackResults(
    const GPUTensor& packed_output,
    const PackedBatch& batch_info
) {
    if (!batch_info.is_valid()) {
        spdlog::error("Invalid batch info for unpacking");
        return {};
    }
    
    auto output_shape = packed_output.shape();
    if (static_cast<int>(output_shape.size()) != 2) {
        spdlog::error("Expected 2D packed output [total_tokens, hidden_dim], got {}D", 
                     output_shape.size());
        return {};
    }
    
    size_t total_tokens = output_shape[0];
    size_t hidden_dim = output_shape[1];
    
    if (total_tokens != batch_info.total_tokens) {
        spdlog::error("Token count mismatch: expected {}, got {}", 
                     batch_info.total_tokens, total_tokens);
        return {};
    }
    
    spdlog::debug("Unpacking {} sequences from packed tensor", batch_info.num_sequences);
    
    // Download packed output
    auto packed_data = packed_output.cpu_data();
    
    // Unpack into separate tensors
    std::vector<GPUTensor> unpacked;
    unpacked.reserve(batch_info.num_sequences);
    
    for (size_t i = 0; i < batch_info.num_sequences; ++i) {
        size_t offset = batch_info.sequence_offsets[i];
        size_t length = batch_info.sequence_lengths[i];
        
        // Extract this sequence's data
        std::vector<float> seq_data;
        seq_data.reserve(length * hidden_dim);
        
        for (size_t j = 0; j < length; ++j) {
            size_t token_idx = offset + j;
            for (size_t k = 0; k < hidden_dim; ++k) {
                seq_data.push_back(packed_data[token_idx * hidden_dim + k]);
            }
        }
        
        // Create tensor for this sequence
        GPUTensor seq_tensor({length, hidden_dim}, device_);
        seq_tensor.upload(seq_data);
        unpacked.push_back(std::move(seq_tensor));
    }
    
    return unpacked;
}

float SequencePacker::calculateMemorySavings(
    const std::vector<std::vector<int>>& sequences,
    size_t max_length
) {
    if (sequences.empty()) {
        return 0.0f;
    }
    
    // Calculate actual tokens
    size_t actual_tokens = 0;
    for (const auto& seq : sequences) {
        actual_tokens += seq.size();
    }
    
    // Calculate padded tokens
    size_t padded_tokens = sequences.size() * max_length;
    
    if (padded_tokens == 0) {
        return 0.0f;
    }
    
    // Memory savings percentage
    return 1.0f - (static_cast<float>(actual_tokens) / padded_tokens);
}

std::pair<std::vector<std::vector<int>>, std::vector<size_t>> 
SequencePacker::sortByLength(const std::vector<std::vector<int>>& sequences) const {
    // Create index array
    std::vector<size_t> indices(sequences.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    // Sort indices by sequence length (descending)
    std::sort(indices.begin(), indices.end(), 
              [&sequences](size_t a, size_t b) {
                  return static_cast<bool>(sequences[a].size()  < static_cast<int>(sequences[b].size()));
              });
    
    // Reorder sequences
    std::vector<std::vector<int>> sorted;
    sorted.reserve(sequences.size());
    for (size_t idx : indices) {
        sorted.push_back(sequences[idx]);
    }
    
    return {sorted, indices};
}

} // namespace lora
} // namespace llm
} // namespace themis

