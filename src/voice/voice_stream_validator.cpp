/**
 * @file voice_stream_validator.cpp
 * @brief Implementation of voice stream chunk validation.
 * @version 1.0.0
 * @date 2026-08-16
 */

#include "voice/voice_stream_validator.h"
#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>

namespace themis {
namespace voice {

VoiceStreamValidator::VoiceStreamValidator(const std::string& session_id,
                                         uint32_t expected_sample_rate,
                                         uint8_t expected_channels,
                                         uint8_t expected_bit_depth)
    : session_id_(session_id),
      expected_sample_rate_(expected_sample_rate),
      expected_channels_(expected_channels),
      expected_bit_depth_(expected_bit_depth),
      total_bytes_(0),
      chunks_validated_(0),
      last_sequence_num_(0),
      stream_complete_(false) {
    
    if (session_id.empty()) {
        throw std::invalid_argument("session_id must not be empty");
    }
    if (!is_valid_sample_rate(expected_sample_rate)) {
        throw std::invalid_argument("Invalid sample rate: " + std::to_string(expected_sample_rate));
    }
    if (expected_channels == 0 || expected_channels > 8) {
        throw std::invalid_argument("Channels must be 1-8");
    }
    if (!is_valid_bit_depth(expected_bit_depth)) {
        throw std::invalid_argument("Invalid bit depth: " + std::to_string(expected_bit_depth));
    }
}

void VoiceStreamValidator::validate_size([[maybe_unused]] size_t chunk_size) {
    if (chunk_size == 0) {
        throw StreamValidationError("Chunk size cannot be zero", chunk_size, __LINE__);
    }
    if (chunk_size > StreamValidationPolicy::MAX_CHUNK_SIZE_BYTES) {
        throw StreamValidationError("Chunk exceeds maximum size limit", chunk_size, __LINE__);
    }
}

void VoiceStreamValidator::validate_sequence([[maybe_unused]] uint32_t sequence_num) {
    if (chunks_validated_ > 0 && sequence_num != last_sequence_num_ + 1) {
        throw StreamValidationError(
            "Non-sequential chunk: expected " + std::to_string(last_sequence_num_ + 1) +
            " got " + std::to_string(sequence_num),
            0, __LINE__);
    }
}

void VoiceStreamValidator::validate_duration([[maybe_unused]] uint64_t timestamp_ms) {
    if (timestamp_ms / 1000 > StreamValidationPolicy::MAX_STREAM_DURATION_SECONDS) {
        throw StreamValidationError(
            "Stream duration exceeds maximum",
            0, __LINE__);
    }
}

bool VoiceStreamValidator::is_chunk_malformed(const uint8_t* chunk, size_t chunk_size) const noexcept {
    if (!chunk || chunk_size == 0) {
        return true;
    }
    
    // Check for all-zero chunks (likely corruption).
    bool all_zero = true;
    for (size_t i = 0; i < std::min(chunk_size, size_t(1024)); ++i) {
        if (chunk[i] != 0) {
            all_zero = false;
            break;
        }
    }
    if (all_zero && chunk_size > 100) {
        return true;  // Suspiciously all-zero for large chunk.
    }
    
    // Additional malformation checks can be added here.
    return false;
}

ValidatedAudioChunk VoiceStreamValidator::validate_chunk(
    const uint8_t* chunk,
    size_t chunk_size,
    uint32_t sequence_num,
    uint64_t timestamp_ms,
    bool is_final) {
    
    if (stream_complete_) {
        throw StreamValidationError("Cannot add chunk after stream is complete", chunk_size, __LINE__);
    }
    
    // Validate size.
    validate_size(chunk_size);
    
    // Validate sequence.
    validate_sequence(sequence_num);
    
    // Validate duration.
    validate_duration(timestamp_ms);
    
    // Check for malformation.
    if (is_chunk_malformed(chunk, chunk_size)) {
        throw StreamValidationError("Chunk data appears malformed", chunk_size, __LINE__);
    }
    
    // Build validated chunk.
    ValidatedAudioChunk result;
    result.data.assign(chunk, chunk + chunk_size);
    result.sample_rate = expected_sample_rate_;
    result.num_channels = expected_channels_;
    result.bit_depth = expected_bit_depth_;
    result.sequence_num = sequence_num;
    result.timestamp_ms = timestamp_ms;
    result.is_final_chunk = is_final;
    
    // Update state.
    total_bytes_ += chunk_size;
    chunks_validated_++;
    last_sequence_num_ = sequence_num;
    if (is_final) {
        stream_complete_ = true;
    }
    
    return result;
}

void VoiceStreamValidator::reset() noexcept {
    total_bytes_ = 0;
    chunks_validated_ = 0;
    last_sequence_num_ = 0;
    stream_complete_ = false;
}

}  // namespace voice
}  // namespace themis
