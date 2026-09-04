/**
 * @file voice_stream_validator.h
 * @brief Voice stream chunk validation and malformed/oversized input rejection.
 * @version 1.0.0
 * @date 2026-08-16
 * 
 * Wave A-8 Voice hardening: enforces fail-closed validation for audio stream chunks,
 * rejecting malformed data, oversized payloads, and invalid session transitions.
 * 
 * All validation errors trigger immediate stream termination and CPU-safe fallback.
 * 
 * @see src/voice/ROADMAP.md § Wave A Scope for voice
 */

#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

namespace themis {
namespace voice {

// =============================================================================
// Error Types
// =============================================================================

/**
 * @brief Exception thrown when stream validation fails.
 * 
 * Captures validation failure reason, invalid data type, and diagnostics.
 */
class StreamValidationError : public std::runtime_error {
public:
    /**
     * @brief Construct validation error with reason and context.
     * 
     * @param reason Description of validation failure
     * @param chunk_size Size of invalid chunk
     * @param line Source line where validation failed
     */
    StreamValidationError(const std::string& reason, size_t chunk_size, int line)
        : std::runtime_error(build_message(reason, chunk_size, line)),
          chunk_size_(chunk_size), line_(line) {}

    [[nodiscard]] size_t chunk_size() const noexcept { return chunk_size_; }
    [[nodiscard]] int line() const noexcept { return line_; }

private:
    size_t chunk_size_;
    int line_;

    static std::string build_message(const std::string& reason, size_t chunk_size, int line) {
        return "Stream validation failed at line " + std::to_string(line) + 
               ": " + reason + " (chunk_size=" + std::to_string(chunk_size) + ")";
    }
};

// =============================================================================
// Stream Chunk Validation
// =============================================================================

/**
 * @brief Audio stream chunk validation policy.
 * 
 * Enforces bounds on chunk size, format, and other properties.
 * All validation failures are fail-closed.
 */
class StreamValidationPolicy {
public:
    /// Maximum chunk size in bytes (16 MB safety limit).
    static constexpr size_t MAX_CHUNK_SIZE_BYTES = 16 * 1024 * 1024;
    
    /// Minimum chunk size in bytes (must have at least 1 byte).
    static constexpr size_t MIN_CHUNK_SIZE_BYTES = 1;
    
    /// Maximum number of samples in a chunk (48 kHz * 10 seconds max).
    static constexpr uint32_t MAX_SAMPLES_PER_CHUNK = 48000 * 10;
    
    /// Valid audio sample rates (Hz).
    static constexpr uint32_t VALID_SAMPLE_RATES[] = {8000, 16000, 22050, 44100, 48000};
    
    /// Valid audio bit depths.
    static constexpr uint8_t VALID_BIT_DEPTHS[] = {8, 16, 24, 32};
    
    /// Maximum stream duration in seconds (1 hour).
    static constexpr uint32_t MAX_STREAM_DURATION_SECONDS = 3600;
};

/**
 * @brief Audio stream chunk with validation.
 * 
 * Encapsulates a validated audio chunk with metadata.
 */
struct ValidatedAudioChunk {
    /// Raw audio data (byte stream).
    std::vector<uint8_t> data;
    
    /// Sample rate in Hz.
    uint32_t sample_rate = 0;
    
    /// Number of audio channels.
    uint8_t num_channels = 0;
    
    /// Bit depth (8, 16, 24, or 32).
    uint8_t bit_depth = 0;
    
    /// Chunk sequence number (for ordering).
    uint32_t sequence_num = 0;
    
    /// Timestamp in milliseconds (relative to stream start).
    uint64_t timestamp_ms = 0;
    
    /// True if this is the final chunk in the stream.
    bool is_final_chunk = false;
};

/**
 * @brief Stream validator that enforces fail-closed chunk validation.
 * 
 * Validates incoming audio chunks for:
 * - Size bounds (MIN_CHUNK_SIZE_BYTES to MAX_CHUNK_SIZE_BYTES)
 * - Format correctness (sample rate, channels, bit depth)
 * - Sequence ordering
 * - Stream lifetime limits
 * - Malformed data detection
 * 
 * All validation failures throw StreamValidationError and trigger fallback.
 */
class VoiceStreamValidator {
public:
    /**
     * @brief Create a stream validator for a new session.
     * 
     * @param session_id Session identifier
     * @param expected_sample_rate Expected audio sample rate (Hz)
     * @param expected_channels Expected number of audio channels
     * @param expected_bit_depth Expected audio bit depth
     * 
     * @throws std::invalid_argument if parameters are invalid
     */
    explicit VoiceStreamValidator(const std::string& session_id,
                                  uint32_t expected_sample_rate,
                                  uint8_t expected_channels,
                                  uint8_t expected_bit_depth);

    /**
     * @brief Validate and accept a new audio chunk.
     * 
     * Checks:
     * 1. Chunk size is within bounds
     * 2. Chunk format matches session expectations
     * 3. Sequence number is in order
     * 4. No buffer overflow or malformed data
     * 5. Stream lifetime hasn't been exceeded
     * 
     * @param chunk Raw audio chunk data (byte array)
     * @param chunk_size Size of chunk in bytes
     * @param sequence_num Sequence number of this chunk
     * @param timestamp_ms Timestamp in milliseconds (optional)
     * @param is_final True if this is the final chunk
     * 
     * @return ValidatedAudioChunk with accepted data
     * 
     * @throws StreamValidationError if chunk fails validation
     */
    [[nodiscard]] ValidatedAudioChunk validate_chunk(
        const uint8_t* chunk,
        size_t chunk_size,
        uint32_t sequence_num,
        uint64_t timestamp_ms = 0,
        bool is_final = false);

    /**
     * @brief Check if chunk data contains obvious malformation.
     * 
     * Detects:
     * - All-zero chunks (likely corruption)
     * - Extreme sample values (likely overflow)
     * - Inconsistent headers (if format detected)
     * 
     * @param chunk Chunk data
     * @param chunk_size Chunk size
     * 
     * @return true if chunk appears malformed; false if valid
     */
    [[nodiscard]] bool is_chunk_malformed(const uint8_t* chunk, size_t chunk_size) const noexcept;

    /**
     * @brief Get total bytes validated so far.
     */
    [[nodiscard]] uint64_t total_bytes_validated() const noexcept { return total_bytes_; }

    /**
     * @brief Get number of chunks validated so far.
     */
    [[nodiscard]] uint32_t chunks_validated() const noexcept { return chunks_validated_; }

    /**
     * @brief Get last sequence number seen.
     */
    [[nodiscard]] uint32_t last_sequence_num() const noexcept { return last_sequence_num_; }

    /**
     * @brief Get session ID.
     */
    [[nodiscard]] const std::string& session_id() const noexcept { return session_id_; }

    /**
     * @brief Check if validation has completed (final chunk was seen).
     */
    [[nodiscard]] bool is_complete() const noexcept { return stream_complete_; }

    /**
     * @brief Reset validator state (for testing or stream restart).
     */
    void reset() noexcept;

private:
    std::string session_id_;
    uint32_t expected_sample_rate_;
    uint8_t expected_channels_;
    uint8_t expected_bit_depth_;
    
    uint64_t total_bytes_;
    uint32_t chunks_validated_;
    uint32_t last_sequence_num_;
    bool stream_complete_;
    
    // Helpers
    void validate_size(size_t chunk_size);
    void validate_sequence(uint32_t sequence_num);
    void validate_duration(uint64_t timestamp_ms);
};

/**
 * @brief Check if sample rate is valid.
 * 
 * @param sample_rate Sample rate in Hz
 * @return true if valid; false otherwise
 */
[[nodiscard]] inline bool is_valid_sample_rate(uint32_t sample_rate) noexcept {
    for (auto valid_rate : StreamValidationPolicy::VALID_SAMPLE_RATES) {
        if (sample_rate == valid_rate) {
          return true;
        }
    }
    return false;
}

/**
 * @brief Check if bit depth is valid.
 * 
 * @param bit_depth Audio bit depth
 * @return true if valid; false otherwise
 */
[[nodiscard]] inline bool is_valid_bit_depth(uint8_t bit_depth) noexcept {
    for (auto valid_depth : StreamValidationPolicy::VALID_BIT_DEPTHS) {
        if (bit_depth == valid_depth) {
          return true;
        }
    }
    return false;
}

}  // namespace voice
}  // namespace themis
