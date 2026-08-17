/**
 * @file voice_liveness_detector.h
 * @brief Voice Liveness Detection — Challenge-Response Validation for Wave A Batch A-8
 *
 * @version v1.0
 * @note Maturity: 🟡 BETA
 * @note Status: Wave A Batch A-8 Implementation
 *
 * Provides challenge-response-based liveness detection to prevent replay and
 * playback attacks. Server issues random challenges; client must echo them
 * within a time window to prove live audio capture.
 *
 * ## Security Model
 * 1. Server generates random challenge text + timestamp
 * 2. Client captures live audio containing challenge phrase
 * 3. A deterministic local transcript extractor normalizes the response payload
 * 4. Verification checks: text match, timestamp freshness, no replay
 * 
 * ## Challenge Lifecycle
 * ```
 * [Issued] → [Active] → [Verified or Expired]
 * 
 * Active window: 5 seconds (reject if >5s old)
 * Verified challenges: tracked to prevent replay
 * ```
 *
 * @error 7100: Challenge generation failed
 * @error 7101: Challenge verification failed
 * @error 7102: Stale challenge (>5s old)
 * @error 7103: Replay attack detected (duplicate challenge)
 * @error 7104: Invalid challenge response (text mismatch)
 */

#pragma once

#include <string>
#include <map>
#include <set>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

namespace themis {
namespace voice {

/**
 * @brief Voice liveness challenge descriptor
 */
struct Challenge {
    uint64_t id;                ///< Unique challenge ID
    std::string text;           ///< Random phrase to echo (e.g., "echo seven thousand two hundred")
    int64_t issued_at_ms;       ///< Timestamp when challenge was issued
    bool verified = false;      ///< Whether this challenge passed verification
    std::string user_id;        ///< User this challenge was issued for
};

/**
 * @class VoiceLivenessDetector
 * @brief Challenge-response based liveness detection
 *
 * Prevents replay and playback attacks by requiring clients to:
 * 1. Receive a random challenge phrase
 * 2. Capture live audio containing that phrase
 * 3. Return the audio within a time window
 * 4. Prove the response matches the challenge (via speech-to-text)
 *
 * Thread-safe: all methods acquire internal mutex.
 *
 * The implementation is intentionally fail-closed: opaque or malformed binary
 * payloads are rejected unless they contain a locally verifiable transcript
 * representation. This avoids granting access when upstream capture or speech
 * extraction data is incomplete.
 */
class VoiceLivenessDetector {
public:
    /// @brief Configuration for liveness detection
    struct Config {
        int64_t challenge_timeout_ms = 5000;  ///< Time window for response (default: 5 sec)
        int64_t replay_memory_ms = 60000;     ///< How long to remember verified challenges (default: 60 sec)
        bool enable_replay_detection = true;  ///< Track verified challenges to prevent replay
        size_t max_response_bytes = 64 * 1024; ///< Reject oversized response payloads fail-closed
    };

    /// @brief Result of liveness verification
    struct VerificationResult {
        bool passed = false;                  ///< Whether verification passed
        std::string reason;                   ///< Details if failed
        double confidence = 0.0;              ///< Confidence score [0, 1]
        int64_t response_time_ms = 0;         ///< Time from challenge to response
    };

    /// @brief Construct detector with default config
    VoiceLivenessDetector() = default;

    /// @brief Construct detector with custom config
    /// @param config Liveness detection configuration
    explicit VoiceLivenessDetector(const Config& config);

    /// @brief Destructor
    ~VoiceLivenessDetector() = default;

    /// Delete copy, allow move
    VoiceLivenessDetector(const VoiceLivenessDetector&) = delete;
    VoiceLivenessDetector& operator=(const VoiceLivenessDetector&) = delete;
    VoiceLivenessDetector(VoiceLivenessDetector&&) noexcept = default;
    VoiceLivenessDetector& operator=(VoiceLivenessDetector&&) noexcept = default;

    /// @brief Issue a new liveness challenge
    /// @param user_id User identifier
    /// @return Challenge with unique ID and random phrase; nullopt if generation failed
    /// @error 7100: Challenge generation failed
    [[nodiscard]] std::optional<Challenge> issueChallenge(const std::string& user_id);

    /// @brief Verify a challenge response
    /// @param user_id User identifier
    /// @param challenge Original challenge issued
    /// @param audio_response Raw audio data containing user's response
    /// @return VerificationResult with pass/fail verdict
    /// @error 7101: Challenge verification failed
    /// @error 7102: Stale challenge (>5s old)
    /// @error 7103: Replay attack detected
    /// @error 7104: Invalid challenge response (text mismatch)
    [[nodiscard]] VerificationResult verifyResponse(
        const std::string& user_id,
        const Challenge& challenge,
        const std::string& audio_response
    );

    /// @brief Get an active challenge by ID
    /// @param challenge_id Challenge identifier
    /// @return Challenge if found and not expired; nullopt otherwise
    [[nodiscard]] std::optional<Challenge> getChallenge(uint64_t challenge_id) const;

    /// @brief Cleanup expired challenges
    /// @return Number of challenges cleaned up
    [[nodiscard]] size_t cleanupExpiredChallenges();

    /// @brief Check if a challenge has been verified before (replay detection)
    /// @param challenge_id Challenge identifier
    /// @return true if challenge was previously verified (replay); false otherwise
    [[nodiscard]] bool isReplayedChallenge(uint64_t challenge_id) const;

    /// @brief Get current challenge count
    /// @return Number of active challenges
    [[nodiscard]] size_t getActiveChallengeCount() const;

private:
    Config config_;
    mutable std::mutex mutex_;

    uint64_t next_challenge_id_ = 1;
    std::map<uint64_t, Challenge> active_challenges_;       ///< Pending challenges
    std::set<uint64_t> verified_challenges_;               ///< Previously verified (replay detection)
    std::map<uint64_t, int64_t> verified_timestamps_;      ///< When each challenge was verified

    /// @brief Extract a deterministic transcript candidate from the response payload.
    /// @param audio Raw response payload; opaque/binary data is rejected fail-closed.
    /// @return Recognized text; empty string if extraction failed
    std::string speechToText(const std::string& audio);

    /// @brief Generate random challenge phrase
    /// @return Random phrase suitable for voice challenge
    std::string generateRandomChallenge();

    /// @brief Get current time in milliseconds
    /// @return Milliseconds since epoch
    int64_t nowMs() const;

    /// @brief Normalize text for comparison (lowercase, remove punctuation)
    /// @param text Raw text
    /// @return Normalized text
    std::string normalizeText(const std::string& text);
};

}} // namespace themis::voice
