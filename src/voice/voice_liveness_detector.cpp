/**
 * @file voice_liveness_detector.cpp
 * @brief VoiceLivenessDetector implementation
 */

#include "voice/voice_liveness_detector.h"
#include <chrono>
#include <algorithm>
#include <random>
#include <cctype>
#include <iostream>

namespace themis {
namespace voice {

// Random challenge phrases for liveness detection
static const std::vector<std::string> CHALLENGE_PHRASES = {
    "echo seven thousand two hundred",
    "verify three hundred and forty five",
    "confirm nine hundred ninety nine",
    "repeat one thousand and one",
    "voice six hundred seventy eight",
    "authenticate five thousand two",
    "unlock eight hundred and twelve",
    "respond four hundred thirty three",
    "verify two thousand nineteen",
    "confirm six hundred six"
};

VoiceLivenessDetector::VoiceLivenessDetector(const Config& config)
    : config_(config) {
}

std::optional<Challenge> VoiceLivenessDetector::issueChallenge(const std::string& user_id) {
    if (user_id.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    Challenge challenge;
    challenge.id = next_challenge_id_++;
    challenge.text = generateRandomChallenge();
    challenge.issued_at_ms = nowMs();
    challenge.verified = false;
    challenge.user_id = user_id;

    active_challenges_[challenge.id] = challenge;
    return challenge;
}

VerificationResult VoiceLivenessDetector::verifyResponse(
    const std::string& user_id,
    const Challenge& challenge,
    const std::string& audio_response) {
    
    VerificationResult result;
    result.passed = false;
    result.confidence = 0.0;
    result.response_time_ms = nowMs() - challenge.issued_at_ms;

    if (user_id.empty()) {
        result.reason = "Invalid user ID";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if challenge exists and is active
    auto it = active_challenges_.find(challenge.id);
    if (it == active_challenges_.end()) {
        result.reason = "Challenge not found";
        return result;
    }

    const Challenge& stored = it->second;

    // Check if challenge is fresh (within timeout)
    int64_t age_ms = nowMs() - stored.issued_at_ms;
    if (age_ms > config_.challenge_timeout_ms) {
        result.reason = "Challenge expired (stale)";
        active_challenges_.erase(it);  // Clean up expired challenge
        return result;
    }

    // Check for replay attack
    if (config_.enable_replay_detection && isReplayedChallenge(challenge.id)) {
        result.reason = "Replay attack detected (challenge already verified)";
        return result;
    }

    // Verify user match
    if (stored.user_id != user_id) {
        result.reason = "User ID mismatch";
        return result;
    }

    // Convert audio response to text
    std::string response_text = speechToText(audio_response);
    if (response_text.empty()) {
        result.reason = "Speech-to-text conversion failed";
        return result;
    }

    // Normalize both texts for comparison
    std::string normalized_expected = normalizeText(stored.text);
    std::string normalized_response = normalizeText(response_text);

    // Check if response matches challenge
    if (normalized_response.find(normalized_expected) == std::string::npos &&
        normalized_expected.find(normalized_response) == std::string::npos) {
        result.reason = "Challenge response text mismatch";
        return result;
    }

    // Verification passed!
    result.passed = true;
    result.reason = "Liveness verified";
    result.confidence = 0.95;  // High confidence for exact match

    // Record this challenge as verified (for replay detection)
    if (config_.enable_replay_detection) {
        verified_challenges_.insert(challenge.id);
        verified_timestamps_[challenge.id] = nowMs();
    }

    // Remove from active challenges
    active_challenges_.erase(it);

    return result;
}

std::optional<Challenge> VoiceLivenessDetector::getChallenge(uint64_t challenge_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = active_challenges_.find(challenge_id);
    if (it != active_challenges_.end()) {
        // Check if expired
        if (nowMs() - it->second.issued_at_ms <= config_.challenge_timeout_ms) {
            return it->second;
        }
    }
    return std::nullopt;
}

size_t VoiceLivenessDetector::cleanupExpiredChallenges() {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t cleaned = 0;
    int64_t now = nowMs();
    int64_t cutoff = now - config_.challenge_timeout_ms;

    // Remove expired active challenges
    for (auto it = active_challenges_.begin(); it != active_challenges_.end(); ) {
        if (it->second.issued_at_ms < cutoff) {
            it = active_challenges_.erase(it);
            cleaned++;
        } else {
            ++it;
        }
    }

    // Remove old verified challenge records (beyond replay memory window)
    int64_t replay_cutoff = now - config_.replay_memory_ms;
    for (auto it = verified_timestamps_.begin(); it != verified_timestamps_.end(); ) {
        if (it->second < replay_cutoff) {
            verified_challenges_.erase(it->first);
            it = verified_timestamps_.erase(it);
        } else {
            ++it;
        }
    }

    return cleaned;
}

bool VoiceLivenessDetector::isReplayedChallenge(uint64_t challenge_id) const {
    // Note: not taking lock here since it's called from within verifyResponse which already holds it
    return verified_challenges_.find(challenge_id) != verified_challenges_.end();
}

size_t VoiceLivenessDetector::getActiveChallengeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_challenges_.size();
}

std::string VoiceLivenessDetector::speechToText(const std::string& audio) {
    // NOTE: This is a stub implementation. In production, this would call:
    // - Google Cloud Speech-to-Text API
    // - AWS Transcribe
    // - Azure Cognitive Services
    // - Local Whisper model
    //
    // For now, we return empty string to indicate stub (production would not do this)
    // In actual testing, this would be mocked or replaced with a real service
    
    if (audio.empty()) {
        return "";
    }

    // Stub: In production, call real speech-to-text service
    // This is a placeholder that allows the detector to work in testing
    return "";  // Actual implementation would contact speech-to-text service
}

std::string VoiceLivenessDetector::generateRandomChallenge() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> dis(0, CHALLENGE_PHRASES.size() - 1);
    return CHALLENGE_PHRASES[dis(gen)];
}

int64_t VoiceLivenessDetector::nowMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::string VoiceLivenessDetector::normalizeText(const std::string& text) {
    std::string normalized;
    
    for (char c : text) {
        if (std::isalnum(c)) {
            normalized += std::tolower(c);
        } else if (std::isspace(c) && !normalized.empty() && normalized.back() != ' ') {
            normalized += ' ';
        }
    }
    
    // Trim trailing space
    while (!normalized.empty() && normalized.back() == ' ') {
        normalized.pop_back();
    }
    
    return normalized;
}

}} // namespace themis::voice
