/**
 * @file voice_liveness_checker.cpp
 * @brief Implementation of voice liveness detection and anti-spoof verification.
 * @version 1.0.0
 * @date 2026-08-16
 */

#include "voice/voice_liveness_checker.h"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <iomanip>
#include <sstream>

namespace themis {
namespace voice {

VoiceLivenessChecker::VoiceLivenessChecker(const std::string& session_id,
                                         const std::string& speaker_profile)
    : session_id_(session_id),
      speaker_profile_(speaker_profile),
      chunks_checked_(0),
      chunks_rejected_(0),
      last_chunk_time_(std::chrono::high_resolution_clock::now()) {
    
    if (session_id.empty()) {
        throw std::invalid_argument("session_id must not be empty");
    }
}

uint8_t VoiceLivenessChecker::detect_spoof_indicators(
    const uint8_t* audio_data,
    size_t audio_size,
    uint32_t sample_rate) const noexcept {
    
    if (!audio_data || audio_size == 0) {
        return 100;  // Null/empty is definitely spoofed.
    }
    
    uint8_t spoof_confidence = 0;
    
    // Check for all-zero or all-one patterns (synthetic/noise).
    uint32_t zero_count = 0;
    uint32_t max_byte = 0;
    for (size_t i = 0; i < std::min(audio_size, size_t(4096)); ++i) {
        if (audio_data[i] == 0) {
          zero_count++;
        }
        max_byte = std::max(max_byte, static_cast<uint32_t>(audio_data[i]));
    }
    
    if (zero_count > audio_size / 2) {
        spoof_confidence += 30;  // Too many zeros.
    }
    
    // Check for extreme uniformity (TTS-like).
    if (max_byte < 10) {
        spoof_confidence += 20;  // Very low amplitude variation.
    }
    
    return std::min(spoof_confidence, uint8_t(100));
}

uint8_t VoiceLivenessChecker::estimate_liveness_confidence(
    const uint8_t* audio_data,
    size_t audio_size,
    uint32_t sample_rate) const noexcept {
    
    if (!audio_data || audio_size == 0) {
        return 0;  // No data = not live.
    }
    
    uint8_t confidence = 50;  // Start with neutral.
    
    // If audio has variation and isn't uniform, increase confidence.
    uint32_t sample_count = audio_size / 2;  // Assume 16-bit samples.
    if (sample_count > sample_rate / 100) {  // At least 10ms of audio.
        confidence += 10;
    }
    
    // If not empty/silence, increase confidence.
    uint32_t nonzero_count = 0;
    for (size_t i = 0; i < std::min(audio_size, size_t(4096)); ++i) {
        if (audio_data[i] != 0) {
          nonzero_count++;
        }
    }
    if (nonzero_count > audio_size / 4) {
        confidence += 20;
    }
    
    return std::min(confidence, uint8_t(100));
}

LivenessCheckResult VoiceLivenessChecker::check_audio_chunk(
    const uint8_t* audio_data,
    size_t audio_size,
    uint32_t sample_rate) {
    
    if (!audio_data) {
        throw std::invalid_argument("audio_data must not be null");
    }
    if (audio_size == 0) {
        throw std::invalid_argument("audio_size must be > 0");
    }
    if (sample_rate == 0) {
        throw std::invalid_argument("sample_rate must be > 0");
    }
    
    LivenessCheckResult result;
    chunks_checked_++;
    
    // Check for silence/noise.
    if (is_silence_or_noise_only(audio_data, audio_size, sample_rate)) {
        result.is_live = false;
        result.liveness_confidence = 0;
        result.spoof_confidence = 50;
        result.reason = "Silence or noise detected";
        chunks_rejected_++;
        return result;
    }
    
    // Estimate liveness confidence.
    result.liveness_confidence = estimate_liveness_confidence(audio_data, audio_size, sample_rate);
    
    // Detect spoof indicators.
    result.spoof_confidence = detect_spoof_indicators(audio_data, audio_size, sample_rate);
    
    // Check for replay.
    std::string audio_hash = compute_audio_hash(audio_data, audio_size);
    if (is_replay_detected(audio_hash)) {
        result.is_live = false;
        result.spoof_confidence = std::max(result.spoof_confidence, uint8_t(70));
        result.reason = "Replay attack detected";
        chunks_rejected_++;
        return result;
    }
    
    // Final decision.
    if (result.liveness_confidence >= LivenessPolicy::MIN_LIVENESS_CONFIDENCE_THRESHOLD &&
        result.spoof_confidence <= LivenessPolicy::MAX_SPOOF_CONFIDENCE_THRESHOLD) {
        result.is_live = true;
        result.reason = "Audio appears to be live speech";
    } else {
        result.is_live = false;
        result.reason = "Liveness threshold not met";
        chunks_rejected_++;
    }
    
    return result;
}

bool VoiceLivenessChecker::is_replay_detected(const std::string& audio_hash) noexcept {
    // Check if this hash has been seen before.
    for (const auto& prev_hash : audio_hash_history_) {
        if (prev_hash == audio_hash) {
            return true;  // Exact replay detected.
        }
    }
    
    // Add to history (keep bounded to avoid memory growth).
    audio_hash_history_.push_back(audio_hash);
    if (audio_hash_history_.size() > LivenessPolicy::MAX_SESSION_HISTORY_FOR_REPLAY_CHECK) {
        audio_hash_history_.erase(audio_hash_history_.begin());
    }
    
    return false;
}

bool VoiceLivenessChecker::is_silence_or_noise_only(
    const uint8_t* audio_data,
    size_t audio_size,
    uint32_t sample_rate) const noexcept {
    
    if (!audio_data || audio_size < 2) {
        return true;
    }
    
    // Simple check: if average byte value is very low, likely silence.
    uint32_t sum = 0;
    for (size_t i = 0; i < std::min(audio_size, size_t(4096)); ++i) {
        sum += audio_data[i];
    }
    uint8_t avg = sum / std::min(audio_size, size_t(4096));
    
    return avg < 20;  // Very low average indicates silence/noise.
}

std::string VoiceLivenessChecker::compute_audio_hash(
    const uint8_t* audio_data,
    size_t audio_size) const noexcept {
    
    if (!audio_data || audio_size == 0) {
        return "empty";
    }
    
    // Simple hash: checksum of first 1KB.
    uint32_t hash = 0;
    for (size_t i = 0; i < std::min(audio_size, size_t(1024)); ++i) {
        hash = hash * 31 + audio_data[i];
    }
    
    // Convert to hex string.
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0') << std::setw(8) << hash;
    return oss.str();
}

void VoiceLivenessChecker::reset() noexcept {
    chunks_checked_ = 0;
    chunks_rejected_ = 0;
    audio_hash_history_.clear();
    last_chunk_time_ = std::chrono::high_resolution_clock::now();
}

}  // namespace voice
}  // namespace themis
