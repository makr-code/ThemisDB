/**
 * @file voice_liveness_detector.cpp
 * @brief VoiceLivenessDetector implementation
 */

#include "voice/voice_liveness_detector.h"
#include <chrono>
#include <algorithm>
#include <random>
#include <cctype>
#include <limits>

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

namespace {

[[nodiscard]] bool isPrintableTranscriptPayload(const std::string& payload) {
    if (payload.empty()) {
        return false;
    }

    size_t printable_count = 0;
    for (const unsigned char c : payload) {
        if (std::isprint(c) || std::isspace(c)) {
            ++printable_count;
        } else {
            return false;
        }
    }

    return printable_count == static_cast<int>(payload.size());
}

[[nodiscard]] std::string extractTranscriptCandidate(const std::string& payload) {
    if (!isPrintableTranscriptPayload(payload)) {
        return {};
    }

    constexpr const char* kTranscriptPrefix = "transcript:";
    if (payload.rfind(kTranscriptPrefix, 0) == 0) {
        return payload.substr(std::char_traits<char>::length(kTranscriptPrefix));
    }

    const auto transcript_key = payload.find("\"transcript\"");
    if (transcript_key != std::string::npos) {
        const auto colon = payload.find(':', transcript_key);
        const auto first_quote = payload.find('"', colon == std::string::npos ? transcript_key : colon + 1);
        if (colon != std::string::npos && first_quote != std::string::npos) {
            const auto second_quote = payload.find('"', first_quote + 1);
            if (second_quote != std::string::npos && second_quote > first_quote + 1) {
                return payload.substr(first_quote + 1, second_quote - first_quote - 1);
            }
        }
    }

    return payload;
}

size_t cleanupExpiredChallengesUnlocked(
    const int64_t now,
    const int64_t challenge_timeout_ms,
    const int64_t replay_memory_ms,
    std::map<uint64_t, Challenge>& active_challenges,
    std::set<uint64_t>& verified_challenges,
    std::map<uint64_t, int64_t>& verified_timestamps) {
    size_t cleaned = 0;
    const int64_t cutoff = now - challenge_timeout_ms;

    for (auto it = active_challenges.begin(); it != active_challenges.end();) {
        if (it->second.issued_at_ms < cutoff) {
            it = active_challenges.erase(it);
            ++cleaned;
        } else {
            ++it;
        }
    }

    const int64_t replay_cutoff = now - replay_memory_ms;
    for (auto it = verified_timestamps.begin(); it != verified_timestamps.end();) {
        if (it->second < replay_cutoff) {
            verified_challenges.erase(it->first);
            it = verified_timestamps.erase(it);
        } else {
            ++it;
        }
    }

    return cleaned;
}

} // namespace

VoiceLivenessDetector::VoiceLivenessDetector(const Config& config)
    : config_(config) {
}

std::optional<Challenge> VoiceLivenessDetector::issueChallenge(const std::string& user_id) {
    if (user_id.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    cleanupExpiredChallengesUnlocked(
        nowMs(),
        config_.challenge_timeout_ms,
        config_.replay_memory_ms,
        active_challenges_,
        verified_challenges_,
        verified_timestamps_);

    Challenge challenge;
    challenge.id = next_challenge_id_++;
    challenge.text = generateRandomChallenge();
    challenge.issued_at_ms = nowMs();
    challenge.verified = false;
    challenge.user_id = user_id;

    active_challenges_[challenge.id] = challenge;
    return challenge;
}

VoiceLivenessDetector::VerificationResult VoiceLivenessDetector::verifyResponse(
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
    if (audio_response.empty()) {
        result.reason = "Empty response payload";
        return result;
    }
    if (static_cast<int>(audio_response.size()) > config_.max_response_bytes) {
        result.reason = "Response payload too large";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    cleanupExpiredChallengesUnlocked(
        nowMs(),
        config_.challenge_timeout_ms,
        config_.replay_memory_ms,
        active_challenges_,
        verified_challenges_,
        verified_timestamps_);

    // Check if challenge exists and is active
    auto it = active_challenges_.find(challenge.id);
    if (it == active_challenges_.end()) {
        const int64_t challenge_age_ms = nowMs() - challenge.issued_at_ms;
        if (challenge_age_ms > config_.challenge_timeout_ms) {
            result.reason = "Challenge expired (stale)";
        } else {
            result.reason = (verified_challenges_.find(challenge.id) != verified_challenges_.end())
                ? "Replay attack detected (challenge already consumed)"
                : "Challenge not found";
        }
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
    if (config_.enable_replay_detection &&
        verified_challenges_.find(challenge.id) != verified_challenges_.end()) {
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
        active_challenges_.erase(it);
        result.reason = "Speech-to-text conversion failed";
        return result;
    }

    // Normalize both texts for comparison
    std::string normalized_expected = normalizeText(stored.text);
    std::string normalized_response = normalizeText(response_text);

    // Check if response matches challenge
    if (normalized_response.find(normalized_expected) == std::string::npos &&
        normalized_expected.find(normalized_response) == std::string::npos) {
        active_challenges_.erase(it);
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
    return cleanupExpiredChallengesUnlocked(
        nowMs(),
        config_.challenge_timeout_ms,
        config_.replay_memory_ms,
        active_challenges_,
        verified_challenges_,
        verified_timestamps_);
}

bool VoiceLivenessDetector::isReplayedChallenge(uint64_t challenge_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return verified_challenges_.find(challenge_id) != verified_challenges_.end();
}

size_t VoiceLivenessDetector::getActiveChallengeCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(active_challenges_.size());
}

std::string VoiceLivenessDetector::speechToText(const std::string& audio) {
    if (audio.empty() || static_cast<int>(audio.size()) > config_.max_response_bytes) {
        return {};
    }

    auto transcript = normalizeText(extractTranscriptCandidate(audio));
    size_t alpha_count = 0;
    for (const unsigned char c : transcript) {
        if (std::isalpha(c)) {
            ++alpha_count;
        }
    }

    return alpha_count >= 3 ? transcript : std::string{};
}

std::string VoiceLivenessDetector::generateRandomChallenge() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<uint64_t> dis(0, static_cast<int>(CHALLENGE_PHRASES.size()) - 1);
    return CHALLENGE_PHRASES[static_cast<size_t>(dis(gen))];
}

int64_t VoiceLivenessDetector::nowMs() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::string VoiceLivenessDetector::normalizeText(const std::string& text) {
    std::string normalized = {};
    
    for (const unsigned char c : text) {
        if (std::isalnum(c)) {
            normalized += static_cast<char>(std::tolower(c));
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
